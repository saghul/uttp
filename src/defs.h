
#ifndef UTTP_DEFS_H
#define UTTP_DEFS_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "uv.h"
#include "http_parser.h"

#include "queue.h"


#define container_of(ptr, type, member) \
    ((type *) ((char *) (ptr) - offsetof(type, member)))

#define ASSERT(x)                                                           \
    do {                                                                    \
        if (!(x)) {                                                         \
            fprintf(stderr, "%s:%u: Assertion `" #x "' failed.\n",          \
                            __FILE__, __LINE__);                            \
            abort();                                                        \
        }                                                                   \
    } while(0)                                                              \

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))


typedef struct uttp_server_s uttp_server_t;
typedef struct uttp_worker_s uttp_worker_t;

struct uttp_server_s {
    uv_loop_t* loop;
    unsigned int own_loop:1;
    struct sockaddr_storage address;
    uv_signal_t sigint_h;
    uv_signal_t sigterm_h;
    uv_barrier_t start_barrier;
    uttp_worker_t* workers;
};

struct uttp_worker_s {
    char name[32];
    uv_loop_t loop;
    uv_async_t stop_async;
    uv_thread_t thread;
    uttp_server_t* server;
    uv_tcp_t tcp_listener;
    QUEUE conn_queue;
    struct {
        unsigned int connections;
    } statistics;
};


typedef struct {
    uv_tcp_t tcp_handle;
    QUEUE queue;
    uttp_worker_t* worker;
    http_parser parser;
    http_parser_settings parser_settings;
} uttp_conn_t;

#endif
