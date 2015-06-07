
#include <string.h>
#include <stdio.h>

#include "log.h"
#include "defs.h"
#include "util.h"
#include "worker.h"
#include "conn.h"


static void uttp__worker_stop(uv_async_t* handle) {
    uv_stop(handle->loop);
}


static void uttp__worker_handle_connection(uv_stream_t* server, int status) {
    int r;
    uttp_conn_t* conn;
    uttp_worker_t* worker = container_of(server, uttp_worker_t, tcp_listener);
    if (status != 0) {
        log_warn("[%s] Error processing incoming connection: %s - %s", worker->name, uv_err_name(status), uv_strerror(status));
        return;
    }

    log_debug("[%s] Processng incoming connection...", worker->name);

    conn = malloc(sizeof(*conn));
    ASSERT(conn != NULL);

    r = uttp_conn_init(worker, conn);
    ASSERT(r == 0);

    r = uv_accept(server, (uv_stream_t*) &conn->tcp_handle);
    if (r != 0) {
        log_warn("[%s] Error accepting incoming connection: %s - %s", worker->name, uv_err_name(r), uv_strerror(r));
        uttp_conn_destroy(conn);
        return;
    }
    uttp_conn_run(conn);
    worker->statistics.connections++;
}


static void uttp__worker_close(uttp_worker_t* worker) {
    int r;
    QUEUE* q;
    uttp_conn_t* conn;

    uv_close((uv_handle_t*) &worker->tcp_listener, NULL);
    uv_close((uv_handle_t*) &worker->stop_async, NULL);

    while (!QUEUE_EMPTY(&worker->conn_queue)) {
        q = QUEUE_HEAD(&worker->conn_queue);
        conn = QUEUE_DATA(q, uttp_conn_t, queue);
        uttp_conn_destroy(conn);
    }
    ASSERT(QUEUE_EMPTY(&worker->conn_queue));

    r = uv_run(&worker->loop, UV_RUN_DEFAULT);
    ASSERT(r == 0);

    r = uv_loop_close(&worker->loop);
    ASSERT(r == 0);
}


static void uttp__worker_run(void* arg) {
    int r;
    uttp_worker_t* worker = arg;
    uttp_server_t* server = worker->server;

    /* start listening for connections */
    r = uv_listen((uv_stream_t*) &worker->tcp_listener, 511, uttp__worker_handle_connection);
    ASSERT(r == 0);

    /* synchronize the start */
    uv_barrier_wait(&server->start_barrier);

    log_info("[%s] started", worker->name);
    uv_run(&worker->loop, UV_RUN_DEFAULT);
    log_info("[%s] stopped", worker->name);

    uttp__worker_close(worker);
}


int uttp_worker_start(uttp_worker_t* worker, uttp_worker_config_t* config) {
    int r;
    int fd;
    uttp_server_t* server = config->server;

    memset(worker, 0, sizeof(*worker));

    snprintf(worker->name, sizeof(worker->name), "Worker #%d", config->id);
    worker->server = server;
    QUEUE_INIT(&worker->conn_queue);

    r = uv_loop_init(&worker->loop);
    ASSERT(r == 0);

    r = uv_async_init(&worker->loop, &worker->stop_async, uttp__worker_stop);
    ASSERT(r == 0);

    fd = uttp_tcp_socket_create(server->address.ss_family);
    ASSERT(fd != -1);

    r = uv_tcp_init(&worker->loop, &worker->tcp_listener);
    ASSERT(r == 0);

    r = uv_tcp_open(&worker->tcp_listener, fd);
    ASSERT(r == 0);

    r = uv_tcp_bind(&worker->tcp_listener, (const struct sockaddr*) &server->address, 0);
    ASSERT(r == 0);

    r = uv_thread_create(&worker->thread, uttp__worker_run, worker);
    ASSERT(r == 0);

    return 0;
}


void uttp_worker_stop(uttp_worker_t* worker) {
    int r;
    r = uv_async_send(&worker->stop_async);
    ASSERT(r == 0);
}


int uttp_worker_join(uttp_worker_t* worker) {
    return uv_thread_join(&worker->thread);
}
