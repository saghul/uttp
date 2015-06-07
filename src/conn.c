
#include <string.h>

#include "uv.h"
#include "http_parser.h"

#include "log.h"
#include "conn.h"
#include "queue.h"


#define UTTP_TEST_RESPONSE       \
  "HTTP/1.1 200 OK\r\n"          \
  "Content-Type: text/plain\r\n" \
  "Content-Length: 12\r\n"       \
  "\r\n"                         \
  "hello world\n"


static void uttp__conn_write_cb(uv_write_t* req, int status) {
    uttp_conn_t* conn = req->data;
    if (status < 0) {
        log_warn("[%s][conn %p] write error: %s - %s", conn->worker->name, conn, uv_err_name(status), uv_strerror(status));
    }
    uttp_conn_destroy(conn);
}


static int http__message_complete(http_parser* parser) {
    int r;
    uv_buf_t buf;
    uv_write_t* req;
    uttp_conn_t* conn = container_of(parser, uttp_conn_t, parser);

    log_debug("[%s][conn %p] HTTP message complete!", conn->worker->name, conn);

    req = malloc(sizeof(*req));
    req->data = conn;
    buf.base = UTTP_TEST_RESPONSE;
    buf.len = sizeof(UTTP_TEST_RESPONSE);
    r = uv_write(req, (uv_stream_t*) &conn->tcp_handle, &buf, 1, uttp__conn_write_cb);
    ASSERT(r == 0);

    return 0;
}


int uttp_conn_init(uttp_worker_t* worker, uttp_conn_t* conn) {
    int r;
    memset(conn, 0, sizeof(*conn));
    conn->worker = worker;
    QUEUE_INIT(&conn->queue);

    http_parser_init(&conn->parser, HTTP_REQUEST);
    http_parser_settings_init(&conn->parser_settings);
    conn->parser_settings.on_message_complete = http__message_complete;
    /* TODO: set HTTP parser callbacks */

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
    size_t nparsed;
    uttp_conn_t* conn = container_of(stream, uttp_conn_t, tcp_handle);

    if (nread < 0) {
        if (nread == UV_EOF) {
            log_debug("[%s][conn %p] closed", conn->worker->name, conn);
        } else {
            log_warn("[%s][conn %p] read error: %s - %s", conn->worker->name, conn, uv_err_name(nread), uv_strerror(nread));
        }
        uttp_conn_destroy(conn);
        goto end;
    }

    nparsed = http_parser_execute(&conn->parser, &conn->parser_settings, buf->base, nread);
    if (conn->parser.upgrade) {
        /* TODO */
        uttp_conn_destroy(conn);
    } else if (conn->parser.http_errno != HPE_OK) {
        log_warn("[%s][conn %p] parsing error: %s - %s", conn->worker->name,
                                                         conn,
                                                         http_errno_name(conn->parser.http_errno),
                                                         http_errno_description(conn->parser.http_errno));
        log_debug("%.*s", (int) nread, buf->base);
        uttp_conn_destroy(conn);
    } else {
        ASSERT(nparsed == nread);
    }

end:
    if (buf->base != NULL) {
        free(buf->base);
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
