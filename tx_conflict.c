#include "common.h"
#define KEY_COUNT 2
#define KEY_SIZE 12
#define VALUE_SIZE 40

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

fdb_error_t readK2_updateK1(FDBTransaction* tr) {
    fdb_error_t err = set_default_transaction_option(tr);
    if (err)
        return err;

    struct timespec to_wait;
    to_wait.tv_sec = 0;
    to_wait.tv_nsec = 1000 * 1000 * 100;
    nanosleep(&to_wait, NULL);

    FDBFuture* future = fdb_transaction_get(tr, keys[1], KEY_SIZE, 0);
    err = block_and_wait(future, "read_K2", (const char*) keys[1]);
    fdb_future_destroy(future);
    if (err)
        return err;

    const char* UPDATE_FORMAT = "updated_%0*d";
    uint8_t updatedV1[VALUE_SIZE];
    sprintf((char*) updatedV1, UPDATE_FORMAT, VALUE_SIZE-8-1, 0);
    fdb_transaction_set(tr, keys[0], KEY_SIZE, updatedV1, VALUE_SIZE);

    to_wait.tv_sec = 0;
    to_wait.tv_nsec = 1000 * 1000 * 400;
    nanosleep(&to_wait, NULL);
    return err;
}

fdb_error_t readK1_updateK2(FDBTransaction* tr) {
    fdb_error_t err = set_default_transaction_option(tr);
    if (err)
        return err;

    FDBFuture* future = fdb_transaction_get(tr, keys[0], KEY_SIZE, 0);
    err = block_and_wait(future, "read_K1", (const char*) keys[0]);
    fdb_future_destroy(future);
    if (err)
        return err;

    const char* UPDATE_FORMAT = "updated_%0*d";
    uint8_t updatedV2[VALUE_SIZE];
    sprintf((char*) updatedV2, UPDATE_FORMAT, VALUE_SIZE-8-1, 1);
    fdb_transaction_set(tr, keys[1], KEY_SIZE, updatedV2, VALUE_SIZE);

    // create larger window for another txn to interrupt
    struct timespec to_wait;
    to_wait.tv_sec = 1;
    to_wait.tv_nsec = 0;
    nanosleep(&to_wait, NULL);

    return err;
}

fdb_error_t read_both(FDBTransaction* tr) {
    fdb_error_t err;

    FDBFuture* future = fdb_transaction_get(tr, keys[0], KEY_SIZE, 0);
    err = block_and_wait(future, "read K1", (const char*) keys[0]);
    if (err) {
        fdb_future_destroy(future);
        return err;
    }

    const uint8_t* obtained;
    fdb_bool_t presented;
    int value_size;
    err = fdb_future_get_value(future, &presented, &obtained, &value_size);
    if (err) {
        fdb_future_destroy(future);
        return err;
    }

    if (presented) {
        printf("[DEBUG] obtained V1: %s\n", obtained);
    }
    fdb_future_destroy(future);

    FDBFuture* future2 = fdb_transaction_get(tr, keys[1], KEY_SIZE, 0);
    err = block_and_wait(future2, "read K2", (const char*) keys[1]);

    if (err) {
        fdb_future_destroy(future2);
        return err;
    }

    err = fdb_future_get_value(future2, &presented, &obtained, &value_size);

    if (err) {
        fdb_future_destroy(future2);
        return err;
    }

    if (presented) {
        printf("[DEBUG] obtained V2: %s\n", obtained);
    }
    fdb_future_destroy(future2);
    return err;
}

void* aux_run(void* db_raw) {
    FDBDatabase* db = (FDBDatabase*) db_raw;

    printf("[INFO] Try to send txn from auxiliary thread.\n");
    run_transaction(db, readK2_updateK1, "read_K2_update_K1");
    return NULL;
}

/*
    * Timeline ---main----------100ms------500ms-------1sec-------
    * R K1 W K2   Begin send                          Commit(aborted)
    * R K2 W K1   Begin         send      Commit
    
    Txn1 (Read K1 Write K2) should be aborted because Txn2 commented.
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
    run_transaction(db, readK1_updateK2, "read_K1_update_K2_should_be_aborted");
    
    err_pthread = pthread_join(auxiliary_thread, NULL);
    if (err_pthread) {
        printf("[FATAL] During joining the auxiliary thread. err: %s\n", strerror(err_pthread));
        exit(2);
    }

    run_transaction(db, read_both, "read_both_for_check");
    run_transaction(db, delete_range_impl, "cleanup");
    destroy_key_value(keys, values, KEY_COUNT);
    teardown(&network_thread);
}
