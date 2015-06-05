
#ifndef UTTP_WORKER_H
#define UTTP_WORKER_H

#include "defs.h"


typedef struct {
    unsigned int id;
    uttp_server_t* server;
} uttp_worker_config_t;


int uttp_worker_start(uttp_worker_t* worker, uttp_worker_config_t* config);

void uttp_worker_stop(uttp_worker_t* worker);

int uttp_worker_join(uttp_worker_t* worker);

#endif
