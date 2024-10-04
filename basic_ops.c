#include <foundationdb/fdb_c_types.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define FDB_API_VERSION 730
#include <foundationdb/fdb_c.h>

// Start routine to run the network in auxiliary thread.
void* run_network() {
    fdb_error_t err = fdb_run_network();
    if (err) {
        printf("[ERROR] During running network. err: %s\n", fdb_get_error(err));
    }
    return NULL;
}

FDBDatabase* setup(const char* cluster_file_path, pthread_t* network_thread) {
    fdb_error_t err = fdb_select_api_version(FDB_API_VERSION);
    if (err) {
        printf("[ERROR] During select API version. err: %s\n", fdb_get_error(err));
        return NULL;
    }

    printf("This program uses client version: %s\n\n", fdb_get_client_version());
    err = fdb_setup_network();
    if (err) {
        printf("[ERROR] During seting up network. err: %s\n", fdb_get_error(err));
        return NULL;
    }

    int err_pthread = pthread_create(network_thread, NULL, (void*) &run_network, NULL);
    if (err_pthread) {
        printf("[ERROR] During creating network thread. err: %s\n", strerror(err_pthread));
        return NULL;
    }

    printf("Network thread started.\n");
    FDBDatabase* db;
    err = fdb_create_database(cluster_file_path, &db);
    if (err) {
        printf("[ERROR] During creating database. err: %s\n", fdb_get_error(err));
        return NULL;
    }

    printf("Database create successfully.\n");
    return db;
}

void teardown(pthread_t* network_thread) {
    fdb_error_t err = fdb_stop_network();
    if (err) {
        printf("[ERROR] During stopping database. err: %s\n", fdb_get_error(err));
    }

    int err_pthread = pthread_join(*network_thread, NULL);
    if (err_pthread) {
        printf("[ERROR] During joining the network thread. err: %s\n", strerror(err_pthread));
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
