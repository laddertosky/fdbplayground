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

// expected_raw should be FDBKeyValue*
void check_obtained_value(FDBFuture* future, void* expected_raw);

void exit_when_err(fdb_error_t err, const char* method);
void* run_network();
FDBDatabase* setup(const char* cluster_file_path, pthread_t* network_thread);
void teardown(pthread_t* network_thread);

fdb_error_t set_default_transaction_option(FDBTransaction* tr);
void run_transaction(FDBDatabase* db, fdb_error_t (*run_impl)(FDBTransaction*), const char* task_description);
