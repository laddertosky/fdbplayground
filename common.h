#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include <foundationdb/fdb_c_types.h>

#define FDB_API_VERSION 730
#include <foundationdb/fdb_c.h>

void prepare_key_value(uint8_t** keys, int key_size, uint8_t** values, int value_size, int key_count);
void destroy_key_value(uint8_t** keys, uint8_t** values, int key_count);
fdb_error_t block_and_wait(FDBFuture* future, const char* operation, const char* key);

// expected_raw should be FDBKeyValue*
void check_obtained_value(FDBFuture* future, void* expected_raw);

void exit_when_err(fdb_error_t err, const char* method);
void* run_network();
FDBDatabase* setup(const char* cluster_file_path, pthread_t* network_thread);
void teardown(pthread_t* network_thread, FDBDatabase* db);

fdb_error_t set_default_transaction_option(FDBTransaction* tr);

typedef fdb_error_t(*TxnTask)(FDBTransaction*);
fdb_error_t run_transaction(FDBDatabase* db, TxnTask run_impl, const char* task_description);
fdb_error_t* async_run_multiple_transactions(FDBDatabase* db, TxnTask* run_impls, const char** task_descriptions, int transaction_count);

double getTimeMilliSec();

typedef struct {
    const char* description;
    fdb_error_t last_err;
    int err_count;
    int item_count;
    int transaction_count;
    double total_response_time_msec;
} BenchmarkRecord;

BenchmarkRecord benchmark(FDBDatabase* db, int key_count, int batch_size, fdb_error_t (*task_impl)(FDBTransaction*), const char* task_description);
BenchmarkRecord benchmark_async(FDBDatabase* db, int key_count, int batch_size, TxnTask task_impl, const char* task_description);
void printRecord(BenchmarkRecord record);
