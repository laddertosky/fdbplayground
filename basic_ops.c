#include <stdlib.h>
#include <foundationdb/fdb_c_types.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define FDB_API_VERSION 730
#include <foundationdb/fdb_c.h>

#define KEY_COUNT 100

const int KEY_SIZE = 8;
const int KEY_PREFIX_SIZE = 4;
const char* KEY_FORMAT = "key_%0*d";
const int VALUE_SIZE = 128;
const int VALUE_PREFIX_SIZE = 6;
const char* VALUE_FORMAT = "value_%0*d";

uint8_t* keys[KEY_COUNT];
uint8_t* values[KEY_COUNT];

void prepare_key_value() {
    for (int i = 0; i < KEY_COUNT; i++) {
        uint8_t* key = (uint8_t*) malloc(sizeof(uint8_t) * KEY_SIZE);
        sprintf((char*) key, KEY_FORMAT, KEY_SIZE-KEY_PREFIX_SIZE, i);
        keys[i] = key;

        uint8_t* value = (uint8_t*) malloc(sizeof(uint8_t) * VALUE_SIZE);
        sprintf((char*) value, VALUE_FORMAT, VALUE_SIZE-VALUE_PREFIX_SIZE, i);
        values[i] = value;
    }
}

void destroy_key_value() {
    for (int i = 0; i < KEY_COUNT; i++) {
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

void run_transaction(FDBDatabase* db, fdb_error_t (*run_impl)(FDBTransaction*), const char* task_description) {
    FDBTransaction* tr = NULL;
    exit_when_err(fdb_database_create_transaction(db, &tr), "fdb_database_create_transaction");

    fdb_error_t err = run_impl(tr);
    if (!err) {
        FDBFuture* future = fdb_transaction_commit(tr);
        err = fdb_future_block_until_ready(future);
        fdb_future_destroy(future);
        printf("[INFO] transaction: %s committed\n", task_description);
    }

    // Error might come from commiting process, so don't merge with the above branch.
    if (err) {
        printf("[ERROR] Something wrong in transaction: %s, to rollback... Descrition: %s\n", task_description, fdb_get_error(err));

        FDBFuture* future = fdb_transaction_on_error(tr, err);

        fdb_error_t block_err = fdb_future_block_until_ready(future);
        if (block_err) {
            printf("[ERROR] During rolling back for transaction: %s. From blocking operation, description: %s\n", task_description, fdb_get_error(block_err));
        } else {
            fdb_error_t future_err = fdb_future_get_error(future);
            printf("[ERROR] During rolling back for transaction: %s. From future operation, Description: %s\n", task_description, fdb_get_error(future_err));
        }
        fdb_future_destroy(future);
    }

    fdb_transaction_destroy(tr);
}

fdb_error_t get_impl(FDBTransaction* tr) {
    return NULL;
}

fdb_error_t set_impl(FDBTransaction* tr) {
    fdb_error_t err = set_default_transaction_option(tr);
    if (err)
        return err;

    for (int i = 0; i < KEY_COUNT; i++) {
        fdb_transaction_set(tr, keys[i], KEY_SIZE, values[i], VALUE_SIZE);
    }
    return NULL;
}

fdb_error_t getrange_impl(FDBTransaction* tr) {
    return NULL;
}

fdb_error_t delete_impl(FDBTransaction* tr) {
    return NULL;
}

void test_get(FDBDatabase* db) {
    run_transaction(db, get_impl, "get");
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

    prepare_key_value();
    test_set(db);
    test_get(db);
    test_getrange(db);
    test_delete(db);

    destroy_key_value();
    teardown(&network_thread);
}
