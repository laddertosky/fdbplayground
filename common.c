#include "common.h"

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
    }
}

void prepare_key_value(uint8_t** keys, int key_size, uint8_t** values, int value_size, int key_count) {
    const int KEY_PREFIX_SIZE = 4;
    const char* KEY_FORMAT = "key_%0*d";
    const int VALUE_PREFIX_SIZE = 6;
    const char* VALUE_FORMAT = "value_%0*d";

    for (int i = 0; i < key_count; i++) {
        uint8_t* key = (uint8_t*) malloc(sizeof(uint8_t) * (1+key_size));
        sprintf((char*) key, KEY_FORMAT, key_size-KEY_PREFIX_SIZE, i);
        keys[i] = key;

        uint8_t* value = (uint8_t*) malloc(sizeof(uint8_t) * (1+value_size));
        sprintf((char*) value, VALUE_FORMAT, value_size-VALUE_PREFIX_SIZE, i);
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
