
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "uv.h"

#include "defs.h"
#include "log.h"
#include "server.h"
#include "worker.h"


static void uttp__server_quit(uv_signal_t* handle, int signum) {
    switch (signum) {
        case SIGINT:
            log_info("Received SIGINT, stopping...");
            break;
        case SIGTERM:
            log_info("Received SIGTERM, stopping...");
            break;
        default:
            abort();
    }

    uv_stop(handle->loop);
}


int uttp_server_run(uttp_server_t* server, uttp_server_config_t* config, uv_loop_t* loop) {
    int r;
    int i;

    if (strchr(config->interface, ':')) {
        if (uv_ip6_addr(config->interface, config->port, (struct sockaddr_in6*) &server->address) != 0) {
            log_warn("Couldn't process IP: port -> [%s]:%d", config->interface, config->port);
            return 1;
        }
    } else {
        if (uv_ip4_addr(config->interface, config->port, (struct sockaddr_in*) &server->address) != 0) {
            log_warn("Couldn't process IP: port -> %s:%d", config->interface, config->port);
            return 1;
        }
    }

    log_debug("interface: %s, port: %d, workers: %d", config->interface,
                                                      config->port,
                                                      config->nworkers);

    /* initialize loop */
    if (loop != NULL) {
        server->loop = loop;
        server->own_loop = 0;
    } else {
        server->loop = malloc(sizeof(uv_loop_t));
        r = uv_loop_init(server->loop);
        ASSERT(r == 0);
        server->own_loop = 1;
    }

    /* initialize signal handles */
    r = uv_signal_init(server->loop, &server->sigint_h);
    ASSERT(r == 0);
    r = uv_signal_start(&server->sigint_h, uttp__server_quit, SIGINT);
    ASSERT(r == 0);

    r = uv_signal_init(server->loop, &server->sigterm_h);
    ASSERT(r == 0);
    r = uv_signal_start(&server->sigterm_h, uttp__server_quit, SIGTERM);
    ASSERT(r == 0);

    /* compute number or workers */
    if (config->nworkers == 0) {
        uv_cpu_info_t* infos;
        int count;
        int r;
        r = uv_cpu_info(&infos, &count);
        if (r == 0) {
            uv_free_cpu_info(infos, count);
            config->nworkers = count;
        } else {
            log_warn("Couldn't determine CPU information, using a single worker");
            config->nworkers = 1;
        }
    }
    ASSERT(config->nworkers > 0);

    /* initialize start barrier */
    r = uv_barrier_init(&server->start_barrier, config->nworkers + 1);
    ASSERT(r == 0);

    /* start workers */
    log_info("Launching %d workers", config->nworkers);
    server->workers = malloc(sizeof(uttp_worker_t) * config->nworkers);
    ASSERT(server->workers != NULL);
    for (i = 0; i < config->nworkers; i++) {
        uttp_worker_t* worker = server->workers + i;
        uttp_worker_config_t worker_config = {.id = i, .server = server};
        r = uttp_worker_start(worker, &worker_config);
        ASSERT(r == 0);
    }

    /* wait for all workers to start */
    uv_barrier_wait(&server->start_barrier);

    uv_run(server->loop, UV_RUN_DEFAULT);

    log_info("Stopping workers...");
    for (i = 0; i < config->nworkers; i++) {
        uttp_worker_t* worker = server->workers + i;
        uttp_worker_stop(worker);
    }
    for (i = 0; i < config->nworkers; i++) {
        uttp_worker_t* worker = server->workers + i;
        r = uttp_worker_join(worker);
        ASSERT(r == 0);
    }

    log_info("Server stopped");
    return 0;
}


void uttp_server_close(uttp_server_t* server) {
    /* TODO clean resources */
}
