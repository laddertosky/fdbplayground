#include "common.h"
#include <stdint.h>

#define KEY_COUNT 10000
#define KEY_SIZE 12
#define VALUE_SIZE 128

uint8_t** keys = NULL;
uint8_t** values = NULL;

fdb_error_t get_range_want_all_impl(FDBTransaction* tr) {
    fdb_error_t err = set_default_transaction_option(tr);
    if (err)
        return err;

    uint8_t begin_key[2] = "\x00";

    // the server will not accept end_key with \xff, even with 0 end_or_equal, is that a bug?
    uint8_t end_key[2] = "\xfe";

    FDBFuture* future = fdb_transaction_get_range(tr, begin_key, 
                                                  2 /* begin_key_name_length */, 
                                                  0 /* begin_or_equal */,
                                                  0 /* begin_offset */,
                                                  end_key, 
                                                  2 /* end_key_name_length */, 
                                                  0 /* end_or_equal */, 
                                                  1 /* end_offset */, 
                                                  0 /* limit */, 
                                                  0 /* target_bytes */, 
                                                  FDB_STREAMING_MODE_WANT_ALL, 
                                                  0 /* iteration */, 
                                                  0 /* snapshot */, 
                                                  0 /* reverse */);

    const FDBKeyValue* outputs;
    fdb_bool_t more = 1;
    int total_count = 0;
    int count;

    while (more) {
        err = block_and_wait(future, "get_range_want_all_impl", "all");
        if (err)
            return err;

        err = fdb_future_get_keyvalue_array(future, &outputs, &count, &more);
        printf("[DEBUG] Get %d kv pairs within %s - %s, more: %d\n", count, begin_key, end_key, more);
        total_count += count;

        if (more) {
            FDBFuture* future2 = fdb_transaction_get_range(tr, 
                                                           outputs[count-1].key, 
                                                           outputs[count-1].key_length /* begin_key_name_length */, 
                                                           1 /* begin_or_equal */,
                                                           1 /* begin_offset */,
                                                           end_key, 
                                                           2 /* end_key_name_length */, 
                                                           0 /* end_or_equal */, 
                                                           1 /* end_offset */, 
                                                           0 /* limit */, 
                                                           0 /* target_bytes */, 
                                                           FDB_STREAMING_MODE_WANT_ALL, 
                                                           0 /* iteration */, 
                                                           0 /* snapshot */, 
                                                           0 /* reverse */);

            fdb_future_destroy(future);
            future = future2;
        }
    }

    if (total_count != KEY_COUNT) {
        printf("[WARN] Obtained pairs count not match, expected: %d, actural: %d\n", KEY_COUNT, total_count);
    }

    fdb_future_destroy(future);

    return 0;
}

void do_benchmark(FDBDatabase* db) {
    BenchmarkRecord r1 = benchmark(db, KEY_COUNT, KEY_COUNT, get_range_want_all_impl, "get_range_mode_want_all_impl_10k");
    printRecord(r1);
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
    keys = (uint8_t**) malloc(sizeof(uint8_t*) * KEY_COUNT);
    values = (uint8_t**) malloc(sizeof(uint8_t*) * KEY_COUNT); 

    prepare_key_value(keys, KEY_SIZE, values, VALUE_SIZE, KEY_COUNT);
    run_transaction(db, set_impl, "set all 10k pairs");

    do_benchmark(db);

    run_transaction(db, delete_range_impl, "cleanup");
    destroy_key_value(keys, values, KEY_COUNT);
    teardown(&network_thread);
}

