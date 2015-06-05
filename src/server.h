
#ifndef UTTP_SERVER_H
#define UTTP_SERVER_H

#include "uv.h"

#include "defs.h"


typedef struct {
    char* interface;
    unsigned short port;
    unsigned int nworkers;
} uttp_server_config_t;


int uttp_server_run(uttp_server_t* server, uttp_server_config_t* config, uv_loop_t* loop);

void uttp_server_close(uttp_server_t* server);

#endif
