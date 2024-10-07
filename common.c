#include "common.h"
#include <sys/time.h>
#include <time.h>

// FDBCallback
void check_obtained_value(FDBFuture* future, void* expected_raw) {
    FDBKeyValue* expected = (FDBKeyValue*) expected_raw;

    const uint8_t* obtained;
    fdb_bool_t presented;
    int value_size;
    fdb_error_t err = fdb_future_get_value(future, &presented, &obtained, &value_size);

    if (err) {
        printf("[WARN] Could not get value for key: %s. Description: %s\n", expected->key, fdb_get_error(err));   
    } else if (presented == 0) {
        printf("[WARN] Could not get value for key: %s\n", expected->key);
    } else if (value_size != expected->value_length) {
        printf("[WARN] Obtained different value size, key: %s, obtained_value_size: %d, expected_value_size: %d\n", expected->key, value_size, expected->value_length);
    } else if (memcmp(obtained, expected->value, expected->value_length) != 0) {
        printf("[WARN] Obtained different value, key: %s, obtained_value: %s, expected_value: %s\n", expected->key, obtained, expected->value);
    } else {
        printf("[DEBUG] Obtained kv pair is matched: %s:%s\n", expected->key, obtained);
    }
}

void prepare_key_value(uint8_t** keys, int key_size, uint8_t** values, int value_size, int key_count) {
    const int KEY_PREFIX_SIZE = 4;
    const char* KEY_FORMAT = "key_%0*d";
    const int VALUE_PREFIX_SIZE = 6;
    const char* VALUE_FORMAT = "value_%0*d";

    for (int i = 0; i < key_count; i++) {
        uint8_t* key = (uint8_t*) malloc(sizeof(uint8_t) * (key_size));
        sprintf((char*) key, KEY_FORMAT, key_size-KEY_PREFIX_SIZE-1, i);
        keys[i] = key;

        uint8_t* value = (uint8_t*) malloc(sizeof(uint8_t) * (value_size));
        sprintf((char*) value, VALUE_FORMAT, value_size-VALUE_PREFIX_SIZE-1, i);
        values[i] = value;
    }
}

void destroy_key_value(uint8_t** keys, uint8_t** values, int key_count) {
    for (int i = 0; i < key_count; i++) {
        free(keys[i]);
        free(values[i]);
    }
}

void exit_when_err(fdb_error_t err, const char* method) {
    if (!err) return;

    printf("[FATAL] During %s. Description: %s\n", method, fdb_get_error(err));
    exit(1);
}

// Start routine to run the network in auxiliary thread.
void* run_network() {
    exit_when_err(fdb_run_network(), "fdb_run_network");
    return NULL;
}

FDBDatabase* setup(const char* cluster_file_path, pthread_t* network_thread) {
    exit_when_err(fdb_select_api_version(FDB_API_VERSION), "fdb_select_api_version");

    printf("This program uses client version: %s\n\n", fdb_get_client_version());
    exit_when_err(fdb_setup_network(), "fdb_setup_network");

    int err_pthread = pthread_create(network_thread, NULL, (void*) &run_network, NULL);
    if (err_pthread) {
        printf("[FATAL] During creating network thread. Description: %s\n", strerror(err_pthread));
        exit(2);
    }

    printf("Network thread started.\n");
    FDBDatabase* db;
    exit_when_err(fdb_create_database(cluster_file_path, &db), "fdb_create_database");

    printf("Database create successfully.\n");
    return db;
}

void teardown(pthread_t* network_thread) {
    exit_when_err(fdb_stop_network(), "fdb_stop_network");

    int err_pthread = pthread_join(*network_thread, NULL);
    if (err_pthread) {
        printf("[FATAL] During joining the network thread. err: %s\n", strerror(err_pthread));
        exit(2);
    }
    printf("Network thread stopped.\n");
}

uint8_t retry_limit = 5;
fdb_error_t set_default_transaction_option(FDBTransaction* tr) {
    return fdb_transaction_set_option(tr, FDB_TR_OPTION_RETRY_LIMIT, (const uint8_t*)&retry_limit, sizeof(uint64_t));
}

fdb_error_t* async_run_multiple_transactions(FDBDatabase* db, TxnTask* run_impls, const char** task_descriptions, int transaction_count) {
    FDBTransaction** trs = (FDBTransaction**) malloc(sizeof(FDBTransaction*) * transaction_count);
    FDBFuture** futures = (FDBFuture**) malloc(sizeof(FDBFuture*) * transaction_count);
    int* completed = (int*) malloc(sizeof(int) * transaction_count);
    fdb_error_t* errs = (fdb_error_t*) malloc(sizeof(fdb_error_t) * transaction_count);

    for (int i = 0; i < transaction_count; i++) {
        exit_when_err(fdb_database_create_transaction(db, &trs[i]), "fdb_database_create_transaction");
        completed[i] = 0;

        errs[i] = run_impls[i](trs[i]);
        if (errs[i]) {
            futures[i] = fdb_transaction_on_error(trs[i], errs[i]);
            printf("[ERROR] Something wrong in transaction: %s, to rollback... Description: %s\n", task_descriptions[i], fdb_get_error(errs[i]));
        } else {
            futures[i] = fdb_transaction_commit(trs[i]);
            printf("[INFO] transaction: %s accepted, to commit\n", task_descriptions[i]);
        }
    }

    int completed_count = 0;

    struct timespec to_wait;
    to_wait.tv_sec = 0;
    to_wait.tv_nsec = 10000;
    struct timespec remains;

    while (completed_count < transaction_count) {
        for (int i = 0; i < transaction_count; i++) {
            if (completed[i] == 0 && fdb_future_is_ready(futures[i])) {
                completed[i] = 1;
                completed_count++;
            }
        }

        nanosleep(&to_wait, &remains);
    }

    for (int i = 0; i < transaction_count; i++) {
        fdb_future_destroy(futures[i]);
        fdb_transaction_destroy(trs[i]);
    }

    free(trs);    
    free(futures);
    free(completed);
    return errs;
}

fdb_error_t run_transaction(FDBDatabase* db, TxnTask run_impl, const char* task_description) {
    FDBTransaction* tr = NULL;
    exit_when_err(fdb_database_create_transaction(db, &tr), "fdb_database_create_transaction");

    fdb_error_t err = run_impl(tr);
    if (!err) {
        FDBFuture* future = fdb_transaction_commit(tr);
        err = fdb_future_block_until_ready(future);
        if (err) {
            printf("[ERROR] During %s. From blocking operation, description: %s\n", task_description, fdb_get_error(err));
        } else {
            err = fdb_future_get_error(future);
        }

        if (!err) {
            printf("[INFO] transaction: %s committed\n", task_description);
        }
        fdb_future_destroy(future);
    }

    // Error might come from committing process, so don't merge with the above branch.
    if (err) {
        printf("[ERROR] Something wrong in transaction: %s, to rollback... Description: %s\n", task_description, fdb_get_error(err));

        FDBFuture* future = fdb_transaction_on_error(tr, err);

        fdb_error_t err2 = block_and_wait(future, "transaction", task_description);
        if (!err2) {
            fdb_future_destroy(future);
        }
    }

    fdb_transaction_destroy(tr);
    return err;
}

double getTimeMilliSec() {
    static struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec / 1000.0 + tv.tv_sec * 1000.0;
}

BenchmarkRecord benchmark(FDBDatabase* db, int key_count, int batch_size, TxnTask task_impl, const char* task_description) {
    BenchmarkRecord record;
    record.err_count = 0;
    record.item_count = 0;
    record.transaction_count = 0;
    record.description = task_description;

    double start = getTimeMilliSec();
    for (int i = 0; i < key_count / batch_size; i++) {
        char description[64];
        sprintf(description, "%s, batch %d", task_description, i+1);
        fdb_error_t err = run_transaction(db, task_impl, description);
        record.last_err = err;
        if (err) {
            record.err_count++;
        }
        record.transaction_count++;
        record.item_count += batch_size;
    }
    double end = getTimeMilliSec();

    record.total_response_time_msec = end - start;
    return record;
}

BenchmarkRecord benchmark_async(FDBDatabase* db, int key_count, int batch_size, TxnTask task_impl, const char* task_description) {
    BenchmarkRecord record;
    record.err_count = 0;
    record.item_count = 0;
    record.transaction_count = 0;
    record.description = task_description;

    double start = getTimeMilliSec();
    int txn_count = (key_count + batch_size - 1) / batch_size;
    const char** task_descriptions = (const char**) malloc(sizeof(char*) * txn_count);
    TxnTask* tasks = (TxnTask*) malloc(sizeof(TxnTask*) * txn_count);

    for (int i = 0; i < txn_count; i++) {
        char description[64];
        sprintf(description, "%s, batch %d", task_description, i+1);
        task_descriptions[i] = description;
        tasks[i] = task_impl;
    }

    fdb_error_t* errs = async_run_multiple_transactions(db, tasks, task_descriptions, txn_count);
    record.last_err = errs[txn_count-1];
    record.transaction_count = txn_count;
    record.item_count = key_count;

    for (int i = 0; i < txn_count; i++) {
        if (errs[i]) {
            record.err_count++;
        }
    }

    double end = getTimeMilliSec();

    record.total_response_time_msec = end - start;
    free(task_descriptions);
    free(tasks);
    free(errs);
    return record;
}

void printRecord(BenchmarkRecord record) {
    printf("\n[Benchmark] Task: %s ------------------\n", record.description);
    printf("Last transaction status: %s\n", fdb_get_error(record.last_err));
    printf("Item processed: %d\n", record.item_count);
    printf("Transaction processed: %d\n", record.transaction_count);
    printf("Total response time: %lf (ms)\n", record.total_response_time_msec);
    printf("Average response time per 1k item: %lf (ms)\n", record.total_response_time_msec / record.item_count * 1000);
    printf("-------------------End of Record-------------------\n\n");
}

fdb_error_t block_and_wait(FDBFuture *future, const char* operation, const char* key) {
    fdb_error_t err = fdb_future_block_until_ready(future);

    if (err) {
        printf("[ERROR] During %s: %s. From blocking operation, description: %s\n", operation, key, fdb_get_error(err));
        fdb_future_destroy(future);
        return err;
    } else {
        err = fdb_future_get_error(future);

        if (err && !fdb_error_predicate(FDB_ERROR_PREDICATE_RETRYABLE, err))  {
            printf("[ERROR] During %s: %s. From future operation, Description: %s\n", operation, key, fdb_get_error(err));
            fdb_future_destroy(future);
            return err;
        }
    }
    return err;
}
