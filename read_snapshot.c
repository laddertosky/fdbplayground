#include "common.h"
#define KEY_COUNT 4
#define KEY_SIZE 6
#define VALUE_SIZE 100

uint8_t* keys[KEY_COUNT];
uint8_t* values[KEY_COUNT];

fdb_error_t set_impl(FDBTransaction* tr) {
    fdb_error_t err = set_default_transaction_option(tr);
    if (err)
        return err;

    for (int i = 0; i < KEY_COUNT; i++) {
        fdb_transaction_set(tr, keys[i], KEY_SIZE, values[i], VALUE_SIZE);
    }
    return 0;
}

fdb_error_t delete_range_impl(FDBTransaction* tr) {
    fdb_error_t err = set_default_transaction_option(tr);
    if (err)
        return err;

    uint8_t begin_key[2] = "\x00";

    // the server will not accept end_key with \xff, even with 0 end_or_equal, is that a bug?
    uint8_t end_key[2] = "\xfe";
    fdb_transaction_clear_range(tr, begin_key, 2, end_key, 2);
    return 0;
}

fdb_error_t update_impl(FDBTransaction* tr) {
    fdb_error_t err = set_default_transaction_option(tr);
    if (err)
        return err;

    // ensure the read operation is executed (but not yet committed) first
    struct timespec to_wait;
    to_wait.tv_sec = 0;
    to_wait.tv_nsec = 1000 * 1000 * 100;
    nanosleep(&to_wait, NULL);

    const char* UPDATE_FORMAT = "updated_%0*d";

    uint8_t updatedV2[VALUE_SIZE];
    sprintf((char*) updatedV2, UPDATE_FORMAT, VALUE_SIZE-8-1, 1);

    uint8_t updatedV4[VALUE_SIZE];
    sprintf((char*) updatedV4, UPDATE_FORMAT, VALUE_SIZE-8-1, 3);

    fdb_transaction_set(tr, keys[1], KEY_SIZE, updatedV2, VALUE_SIZE);
    fdb_transaction_set(tr, keys[3], KEY_SIZE, updatedV4, VALUE_SIZE);

    printf("[DEBUG] %s updated\n", keys[1]);
    printf("[DEBUG] %s updated\n", keys[3]);

    to_wait.tv_sec = 0;
    to_wait.tv_nsec = 1000 * 1000 * 400;
    nanosleep(&to_wait, NULL);
    return err;
}

fdb_error_t read_impl(FDBTransaction* tr) {
    fdb_error_t err = set_default_transaction_option(tr);
    if (err)
        return err;

    for (int index = 0; index < 3; index++) {
        FDBKeyValue expected;
        expected.key = keys[index];
        expected.key_length = KEY_SIZE;
        expected.value = values[index];
        expected.value_length = VALUE_SIZE;

        FDBFuture* future = fdb_transaction_get(tr, keys[index], KEY_SIZE, 0);
        err = block_and_wait(future, "read", (const char*) keys[index]);

        check_obtained_value(future, &expected);
        fdb_future_destroy(future);
    }

    // create larger window for update to interrupt
    struct timespec to_wait;
    to_wait.tv_sec = 1;
    to_wait.tv_nsec = 0;
    nanosleep(&to_wait, NULL);

    return err;
}

void* aux_run(void* db_raw) {
    FDBDatabase* db = (FDBDatabase*) db_raw;

    printf("[INFO] Auxiliary thread is started.\n");
    run_transaction(db, read_impl, "read_only_without_snapshot_will_not_be_aborted");
    return NULL;
}

fdb_error_t get_range_impl(FDBTransaction* tr) {
    fdb_error_t err = set_default_transaction_option(tr);
    if (err)
        return err;

    uint8_t begin_key[2] = "\x00";

    // the server will not accept end_key with \xff, even with 0 end_or_equal, is that a bug?
    uint8_t end_key[2] = "\xfe";

    // will be ignored when mode is not FDB_STREAMING_MODE_ITERATOR
    int iteration = 0;

    int limit = 0;

    FDBFuture* future = fdb_transaction_get_range(tr, 
                                                  begin_key /* begin_key_name */, 
                                                  2 /* begin_key_name_length */, 
                                                  1 /* begin_or_equal */,
                                                  0 /* begin_offset */,
                                                  end_key /* end_key_name */, 
                                                  2 /* end_key_name_length */, 
                                                  1 /* end_or_equal */, 
                                                  1 /* end_offset */, 
                                                  limit /* limit */, 
                                                  0 /* target_bytes */, 
                                                  FDB_STREAMING_MODE_ITERATOR /* mode */, 
                                                  ++iteration /* iteration */, 
                                                  0 /* snapshot */, 
                                                  0 /* reverse */);

    const FDBKeyValue* outputs;
    fdb_bool_t more = 1;
    int total_count = 0;
    int count;

    while (more) {
        err = block_and_wait(future, "get_range_all", "all");
        if (err)
            return err;

        err = fdb_future_get_keyvalue_array(future, &outputs, &count, &more);
        if (err) {
            fdb_future_destroy(future);
            return err;
        } 

        if (count > 0) {
            for (int i = 0; i < count; i++) {
                // why the first three values contains \n in the output?
                printf("[DEBUG] Obtained %s:%s\n", outputs[i].key, outputs[i].value);
            }
        } else {
            printf("[WARN] Get 0 pairs even with more indicator from previous run.\n");
        }
        total_count += count;

        if (more) {
            FDBFuture* future2 = fdb_transaction_get_range(tr, 
                                                           outputs[count-1].key /* begin_key_name */, 
                                                           outputs[count-1].key_length /* begin_key_name_length */, 
                                                           1 /* begin_or_equal */,
                                                           1 /* begin_offset */,
                                                           end_key /* end_key_name */,
                                                           2 /* end_key_name_length */, 
                                                           1 /* end_or_equal */, 
                                                           1 /* end_offset */, 
                                                           limit /* limit */, 
                                                           0 /* target_bytes */, 
                                                           FDB_STREAMING_MODE_ITERATOR /* mode */, 
                                                           ++iteration /* iteration */, 
                                                           0 /* snapshot */, 
                                                           0 /* reverse */);

            fdb_future_destroy(future);
            future = future2;
        }
    }

    if (total_count != KEY_COUNT) {
        printf("[WARN] Obtained pairs count not match, expected: %d, actual: %d\n", KEY_COUNT, total_count);
    }

    fdb_future_destroy(future);
    return err;
}

/*
    * Timeline ---main----------100ms------500ms-------1sec-------
    * T1:Read     Begin         send                  Commit 
    * T2:Update   Begin send              Commit
    Both transaction should not be aborted.
*   T1: Readonly transaction will never be aborted.
*   T2: There is no other conflicting transaction try to write the same keys.
*/
int main(int argc, char** argv) {
    pthread_t network_thread;
    char* cluster_file_path = NULL;
    if (argc > 1) {
        cluster_file_path = argv[1];
        printf("Use cluster file: %s\n", cluster_file_path);
    } else {
        printf("Use default cluster file\n");
    }

    FDBDatabase* db = setup(cluster_file_path, &network_thread);
    prepare_key_value(keys, KEY_SIZE, values, VALUE_SIZE, KEY_COUNT);
    run_transaction(db, set_impl, "set_all_kv_pairs");

    pthread_t auxiliary_thread;
    int err_pthread = pthread_create(&auxiliary_thread, NULL, (void*) &aux_run, db);
    if (err_pthread) {
        printf("[FATAL] During creating auxiliary thread. Description: %s\n", strerror(err_pthread));
        exit(2);
    }

    run_transaction(db, update_impl, "update_try_to_cause_conflict");
    
    err_pthread = pthread_join(auxiliary_thread, NULL);
    if (err_pthread) {
        printf("[FATAL] During joining the auxiliary thread. err: %s\n", strerror(err_pthread));
        exit(2);
    }

    run_transaction(db, get_range_impl, "read_all_for_checking_committed");
    run_transaction(db, delete_range_impl, "cleanup");
    destroy_key_value(keys, values, KEY_COUNT);
    teardown(&network_thread);
}
