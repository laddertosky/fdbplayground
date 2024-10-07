#include "common.h"
#include "time.h"
#include <foundationdb/fdb_c_types.h>
#define KEY_COUNT 4
#define KEY_SIZE 12
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

    to_wait.tv_sec = 0;
    to_wait.tv_nsec = 1000 * 1000 * 900;
    nanosleep(&to_wait, NULL);
    return err;
}

void* try_update(void* db_raw) {
    FDBDatabase* db = (FDBDatabase*) db_raw;

    printf("[INFO] Try to update from auxilary thread.\n");
    run_transaction(db, update_impl, "update concurrently");
    return NULL;
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

    // create larger window for update to interupt
    struct timespec to_wait;
    to_wait.tv_sec = 0;
    to_wait.tv_nsec = 1000 * 1000 * 500;
    nanosleep(&to_wait, NULL);

    return err;
}

/*
    * Timeline ---main----------100ms------500ms-------1sec-------
    * Read        Begin send              Commit 
    * Update      Begin         send                  Commit
    Both transaction should not be aborted.
    Because fdb applies MVCC and there is a previous version of keys for read txn.
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
    run_transaction(db, set_impl, "set all kv pairs");


    pthread_t write_thread;
    int err_pthread = pthread_create(&write_thread, NULL, (void*) &try_update, db);
    if (err_pthread) {
        printf("[FATAL] During creating auxilary thread. Description: %s\n", strerror(err_pthread));
        exit(2);
    }
    run_transaction(db, read_impl, "slow read txn");
    
    err_pthread = pthread_join(write_thread, NULL);
    if (err_pthread) {
        printf("[FATAL] During joining the auxilary thread. err: %s\n", strerror(err_pthread));
        exit(2);
    }

    run_transaction(db, delete_range_impl, "cleanup");
    destroy_key_value(keys, values, KEY_COUNT);
    teardown(&network_thread);
}
