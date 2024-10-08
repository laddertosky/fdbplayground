#include "common.h"

#define KEY_COUNT 100
#define KEY_SIZE 12
#define VALUE_SIZE 128

uint8_t* keys[KEY_COUNT];
uint8_t* values[KEY_COUNT];

#define ASYNC_GET_COUNT 10
fdb_error_t async_get_impl(FDBTransaction* tr) {
    fdb_error_t err = set_default_transaction_option(tr);
    if (err)
        return err;

    FDBFuture* futures[ASYNC_GET_COUNT];
    FDBKeyValue* expecteds[ASYNC_GET_COUNT];

    for (int i = 0; i < ASYNC_GET_COUNT; i++) {
        int index = rand() % KEY_COUNT; 
        futures[i] = fdb_transaction_get(tr, keys[index], KEY_SIZE, 0);

        FDBKeyValue* expected = (FDBKeyValue*) malloc(sizeof(FDBKeyValue));
        expected->key = keys[index];
        expected->key_length = KEY_SIZE;
        expected->value = values[index];
        expected->value_length = VALUE_SIZE;

        err = fdb_future_set_callback(futures[i], &check_obtained_value, expected);
        expecteds[i] = expected;
    }

    int completed_count = 0;
    int completed[ASYNC_GET_COUNT] = {0};

    struct timespec to_wait;
    to_wait.tv_sec = 0;
    to_wait.tv_nsec = 10000;
    struct timespec remains;

    while (completed_count < ASYNC_GET_COUNT) {
        for (int i = 0; i < ASYNC_GET_COUNT; i++) {
            if (completed[i] == 0 && fdb_future_is_ready(futures[i])) {
                completed[i] = 1;
                completed_count++;
            }
        }

        nanosleep(&to_wait, &remains);
    }

    for (int i = 0; i < ASYNC_GET_COUNT; i++) {
        fdb_future_destroy(futures[i]);
        free(expecteds[i]);
    }

    return 0;
}

const int SYNC_GET_COUNT = 10;
fdb_error_t sync_get_impl(FDBTransaction* tr) {
    fdb_error_t err = set_default_transaction_option(tr);
    if (err)
        return err;

    for (int i = 0; i < SYNC_GET_COUNT; i++) {
        int index = rand() % KEY_COUNT; 
        uint8_t* key = keys[index];
        FDBFuture* future = fdb_transaction_get(tr, key, KEY_SIZE, 0);
        err = block_and_wait(future, "sync_get_impl", (const char*) key);

        FDBKeyValue expected;
        expected.key = keys[index];
        expected.key_length = KEY_SIZE;
        expected.value = values[index];
        expected.value_length = VALUE_SIZE;

        check_obtained_value(future, &expected);
        fdb_future_destroy(future);
    }

    return 0;
}

fdb_error_t set_impl(FDBTransaction* tr) {
    fdb_error_t err = set_default_transaction_option(tr);
    if (err)
        return err;

    for (int i = 0; i < KEY_COUNT; i++) {
        fdb_transaction_set(tr, keys[i], KEY_SIZE, values[i], VALUE_SIZE);
    }
    return 0;
}

fdb_error_t getrange_impl(FDBTransaction* tr) {
    fdb_error_t err = set_default_transaction_option(tr);
    if (err)
        return err;


    uint8_t begin_key[2] = "\x00";

    // the server will not accept end_key with \xff, even with 0 end_or_equal, is that a bug?
    uint8_t end_key[2] = "\xfe";
    int limit = 0;

    FDBFuture* future = fdb_transaction_get_range(tr, begin_key, 
                                                  2 /* begin_key_name_length */, 
                                                  0 /* begin_or_equal */,
                                                  0 /* begin_offset */,
                                                  end_key, 
                                                  2 /* end_key_name_length */, 
                                                  0 /* end_or_equal */, 
                                                  1 /* end_offset */, 
                                                  limit, 
                                                  0 /* target_bytes */, 
                                                  FDB_STREAMING_MODE_WANT_ALL, 
                                                  0 /* iteration */, 
                                                  0 /* snapshot */, 
                                                  0 /* reverse */);

    err = block_and_wait(future, "getrange_impl", "all");
    if (err)
        return err;

    const FDBKeyValue* outputs;
    fdb_bool_t more;
    int count;
    err = fdb_future_get_keyvalue_array(future, &outputs, &count, &more);
    printf("[INFO] Get %d kv pairs within %s - %s, more: %d\n", count, begin_key, end_key, more);

    if (count != KEY_COUNT) {
        printf("[WARN] Obtained key count does not match, expected %d, but got %d\n", KEY_COUNT, count);
    }

    fdb_future_destroy(future);

    return err;
}

fdb_error_t delete_impl(FDBTransaction* tr) {
    fdb_error_t err = set_default_transaction_option(tr);
    if (err)
        return err;

    for (int i = 0; i < KEY_COUNT; i++) {
        fdb_transaction_clear(tr, keys[i], KEY_SIZE);
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

void test_get(FDBDatabase* db) {
    run_transaction(db, sync_get_impl, "sync get");
    run_transaction(db, async_get_impl, "async get");
}

void test_set(FDBDatabase* db) {
    run_transaction(db, set_impl, "set");
}

void test_getrange(FDBDatabase* db) {
    run_transaction(db, getrange_impl, "getrange");
}

void test_delete(FDBDatabase* db) {
    run_transaction(db, delete_impl, "delete");
}

void test_delete_range(FDBDatabase* db) {
    run_transaction(db, set_impl, "set_for_delete");
    run_transaction(db, delete_range_impl, "delete_range");
}

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
    test_set(db);
    test_get(db);
    test_getrange(db);
    test_delete(db);
    test_delete_range(db);

    destroy_key_value(keys, values, KEY_COUNT);
    teardown(&network_thread, db);
}
