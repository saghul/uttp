
#ifndef UTTP_DEFS_H
#define UTTP_DEFS_H

#include <stddef.h>
#include <stdlib.h>

#include "uv.h"


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
    uttp_worker_t* workers;
};

struct uttp_worker_s {
    char name[32];
    uv_loop_t loop;
    uv_async_t stop_async;
    uv_thread_t thread;
    uttp_server_t* server;
};


#endif
