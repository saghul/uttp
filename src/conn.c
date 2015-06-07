
#include <string.h>
#include "uv.h"

#include "log.h"
#include "conn.h"
#include "queue.h"


int uttp_conn_init(uttp_worker_t* worker, uttp_conn_t* conn) {
    int r;
    memset(conn, 0, sizeof(*conn));
    conn->worker = worker;
    QUEUE_INIT(&conn->queue);
    r = uv_tcp_init(&worker->loop, &conn->tcp_handle);
    return r;
}


static void uttp__conn_tcp_close(uv_handle_t* handle) {
    uttp_conn_t* conn = container_of(handle, uttp_conn_t, tcp_handle);
    free(conn);
}


void uttp_conn_destroy(uttp_conn_t* conn) {
    if (!QUEUE_EMPTY(&conn->queue)) {
        QUEUE_REMOVE(&conn->queue);
    }
    uv_close((uv_handle_t*) &conn->tcp_handle, uttp__conn_tcp_close);
}


static void uttp__conn_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = malloc(suggested_size);
    ASSERT(buf->base != NULL);
    buf->len = suggested_size;
}


static void uttp__conn_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    uttp_conn_t* conn = container_of(stream, uttp_conn_t, tcp_handle);

    if (buf->base != NULL) {
        free(buf->base);
    }

    if (nread < 0) {
        if (nread == UV_EOF) {
            log_debug("[%s][conn %p] closed", conn->worker->name, conn);
        } else {
            log_debug("[%s][conn %p] read error: %s - %s", conn->worker->name, conn, uv_err_name(nread), uv_strerror(nread));
        }
        uttp_conn_destroy(conn);
    }
}


void uttp_conn_run(uttp_conn_t* conn) {
    int r;
    uttp_worker_t* worker = conn->worker;
    QUEUE_INSERT_TAIL(&worker->conn_queue, &conn->queue);

    r = uv_read_start((uv_stream_t*) &conn->tcp_handle, uttp__conn_alloc_cb, uttp__conn_read_cb);
    ASSERT(r == 0);

    log_debug("[%s][conn %p] running", conn->worker->name, conn);
}
