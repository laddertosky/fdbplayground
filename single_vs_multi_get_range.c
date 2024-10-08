#include "common.h"

#define KEY_COUNT 10000
#define BATCH_SIZE 1000
#define KEY_SIZE 12
#define VALUE_SIZE 128

uint8_t** keys = NULL;
uint8_t** values = NULL;

fdb_error_t get_range_impl(FDBTransaction* tr, FDBStreamingMode mode, const char* operation, int skipped) {
    fdb_error_t err = set_default_transaction_option(tr);
    if (err)
        return err;

    // will be ignored when mode is not FDB_STREAMING_MODE_ITERATOR
    int iteration = 0;

    // exact mode requires limit parameters
    int limit = BATCH_SIZE;

    // printf("[DEBUG] get_range_impl, skipped: %d, first key: %s\n", skipped, keys[skipped]);
    FDBFuture* future = fdb_transaction_get_range(tr, 
                                                  keys[skipped] /* begin_key_name */, 
                                                  KEY_SIZE /* begin_key_name_length */, 
                                                  1 /* begin_or_equal */,
                                                  0 /* begin_offset */,
                                                  keys[skipped+limit-1] /* end_key_name */, 
                                                  KEY_SIZE /* end_key_name_length */, 
                                                  1 /* end_or_equal */, 
                                                  1 /* end_offset */, 
                                                  limit /* limit */, 
                                                  0 /* target_bytes */, 
                                                  mode /* mode */, 
                                                  ++iteration /* iteration */, 
                                                  0 /* snapshot */, 
                                                  0 /* reverse */);

    const FDBKeyValue* outputs;
    fdb_bool_t more = 1;
    int total_count = 0;
    int count;

    while (more) {
        err = block_and_wait(future, operation, (const char*) keys[skipped]);
        if (err)
            return err;

        err = fdb_future_get_keyvalue_array(future, &outputs, &count, &more);
        if (err) {
            fdb_future_destroy(future);
            return err;
        } 

        if (count > 0) {
            // printf("[DEBUG] Get %d kv pairs within %s - %s, more: %d\n", count, outputs[0].key, outputs[count-1].key, more);
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
                                                           keys[skipped+limit-1], 
                                                           KEY_SIZE /* end_key_name_length */, 
                                                           1 /* end_or_equal */, 
                                                           1 /* end_offset */, 
                                                           limit /* limit */, 
                                                           0 /* target_bytes */, 
                                                           mode /* mode */, 
                                                           ++iteration /* iteration */, 
                                                           0 /* snapshot */, 
                                                           0 /* reverse */);

            fdb_future_destroy(future);
            future = future2;
        }
    }

    if (total_count != BATCH_SIZE) {
        printf("[WARN] Obtained pairs count not match, expected: %d, actual: %d\n", BATCH_SIZE, total_count);
    }

    fdb_future_destroy(future);
    return err;
}

fdb_error_t get_range_want_all_impl(FDBTransaction* tr) {
    static int count = -1;
    count++;
    count %= KEY_COUNT / BATCH_SIZE;
    return get_range_impl(tr, FDB_STREAMING_MODE_WANT_ALL, "get_range_want_all", count*BATCH_SIZE);
}

fdb_error_t get_range_iterator_impl(FDBTransaction* tr) {
    static int count = -1;
    count++;
    count %= KEY_COUNT / BATCH_SIZE;
    return get_range_impl(tr, FDB_STREAMING_MODE_ITERATOR, "get_range_iterator", count*BATCH_SIZE);
}

fdb_error_t get_range_exact_impl(FDBTransaction* tr) {
    static int count = -1;
    count++;
    count %= KEY_COUNT / BATCH_SIZE;
    return get_range_impl(tr, FDB_STREAMING_MODE_EXACT, "get_range_exact", count*BATCH_SIZE);
}

fdb_error_t get_range_small_impl(FDBTransaction* tr) {
    static int count = -1;
    count++;
    count %= KEY_COUNT / BATCH_SIZE;
    return get_range_impl(tr, FDB_STREAMING_MODE_SMALL, "get_range_small", count*BATCH_SIZE);
}

fdb_error_t get_range_medium_impl(FDBTransaction* tr) {
    static int count = -1;
    count++;
    count %= KEY_COUNT / BATCH_SIZE;
    return get_range_impl(tr, FDB_STREAMING_MODE_MEDIUM, "get_range_medium", count*BATCH_SIZE);
}

fdb_error_t get_range_large_impl(FDBTransaction* tr) {
    static int count = -1;
    count++;
    count %= KEY_COUNT / BATCH_SIZE;
    return get_range_impl(tr, FDB_STREAMING_MODE_LARGE, "get_range_large", count*BATCH_SIZE);
}

fdb_error_t get_range_serial_impl(FDBTransaction* tr) {
    static int count = -1;
    count++;
    count %= KEY_COUNT / BATCH_SIZE;
    return get_range_impl(tr, FDB_STREAMING_MODE_SERIAL, "get_range_serial", count*BATCH_SIZE);
}

void do_benchmark(FDBDatabase* db) {
    BenchmarkRecord r1 = benchmark(db, KEY_COUNT, BATCH_SIZE, get_range_want_all_impl, "sync_get_range_mode_want_all_impl_10k_10txn");
    BenchmarkRecord r2 = benchmark(db, KEY_COUNT, BATCH_SIZE, get_range_iterator_impl, "sync_get_range_mode_iterator_impl_10k_10txn");
    BenchmarkRecord r3 = benchmark(db, KEY_COUNT, BATCH_SIZE, get_range_exact_impl, "sync_get_range_mode_exact_impl_10k_10txn");
    BenchmarkRecord r4 = benchmark(db, KEY_COUNT, BATCH_SIZE, get_range_small_impl, "sync_get_range_mode_small_impl_10k_10txn");
    BenchmarkRecord r5 = benchmark(db, KEY_COUNT, BATCH_SIZE, get_range_medium_impl, "sync_get_range_mode_medium_impl_10k_10txn");
    BenchmarkRecord r6 = benchmark(db, KEY_COUNT, BATCH_SIZE, get_range_large_impl, "sync_get_range_mode_large_impl_10k_10txn");
    BenchmarkRecord r7 = benchmark(db, KEY_COUNT, BATCH_SIZE, get_range_serial_impl, "sync_get_range_mode_serial_impl_10k_10txn");

    printRecord(r1);
    printRecord(r2);
    printRecord(r3);
    printRecord(r4);
    printRecord(r5);
    printRecord(r6);
    printRecord(r7);
}

void do_benchmark_async(FDBDatabase* db) {
    BenchmarkRecord r1 = benchmark_async(db, KEY_COUNT, BATCH_SIZE, get_range_want_all_impl, "async_get_range_mode_want_all_impl_10k_10txn");
    BenchmarkRecord r2 = benchmark_async(db, KEY_COUNT, BATCH_SIZE, get_range_iterator_impl, "async_get_range_mode_iterator_impl_10k_10txn");
    BenchmarkRecord r3 = benchmark_async(db, KEY_COUNT, BATCH_SIZE, get_range_exact_impl, "async_get_range_mode_exact_impl_10k_10txn");
    BenchmarkRecord r4 = benchmark_async(db, KEY_COUNT, BATCH_SIZE, get_range_small_impl, "async_get_range_mode_small_impl_10k_10txn");
    BenchmarkRecord r5 = benchmark_async(db, KEY_COUNT, BATCH_SIZE, get_range_medium_impl, "async_get_range_mode_medium_impl_10k_10txn");
    BenchmarkRecord r6 = benchmark_async(db, KEY_COUNT, BATCH_SIZE, get_range_large_impl, "async_get_range_mode_large_impl_10k_10txn");
    BenchmarkRecord r7 = benchmark_async(db, KEY_COUNT, BATCH_SIZE, get_range_serial_impl, "async_get_range_mode_serial_impl_10k_10txn");

    printRecord(r1);
    printRecord(r2);
    printRecord(r3);
    printRecord(r4);
    printRecord(r5);
    printRecord(r6);
    printRecord(r7);
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
    do_benchmark_async(db);

    run_transaction(db, delete_range_impl, "cleanup");
    destroy_key_value(keys, values, KEY_COUNT);
    teardown(&network_thread, db);
}

