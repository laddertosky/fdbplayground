#include <stdlib.h>
#include <foundationdb/fdb_c_types.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define FDB_API_VERSION 730
#include <foundationdb/fdb_c.h>

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

void test_get(FDBDatabase* db) {

}

void test_set(FDBDatabase* db) {

}

void test_getrange(FDBDatabase* db) {

}

void test_delete(FDBDatabase* db) {

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
    test_get(db);
    test_set(db);
    test_getrange(db);
    test_delete(db);

    teardown(&network_thread);
}
