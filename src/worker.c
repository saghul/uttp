
#include <string.h>
#include <stdio.h>

#include "log.h"
#include "defs.h"
#include "worker.h"


static void uttp__worker_stop(uv_async_t* handle) {
    uv_stop(handle->loop);
}


static void uttp__worker_run(void* arg) {
    uttp_worker_t* worker = arg;
    uttp_server_t* server = worker->server;

    /* synchronize the start */
    uv_barrier_wait(&server->start_barrier);

    log_info("[%s] started", worker->name);
    uv_run(&worker->loop, UV_RUN_DEFAULT);
    log_info("[%s] stopped", worker->name);

    /* TODO: cleanup loop */
}


int uttp_worker_start(uttp_worker_t* worker, uttp_worker_config_t* config) {
    int r;

    memset(worker, 0, sizeof(*worker));

    snprintf(worker->name, sizeof(worker->name), "Worker #%d", config->id);
    worker->server = config->server;

    r = uv_loop_init(&worker->loop);
    ASSERT(r == 0);

    r = uv_async_init(&worker->loop, &worker->stop_async, uttp__worker_stop);
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
