
#ifndef UTTP_CONN_H
#define UTTP_CONN_H

#include "defs.h"


int uttp_conn_init(uttp_worker_t* worker, uttp_conn_t* conn);

void uttp_conn_destroy(uttp_conn_t* conn);

void uttp_conn_run(uttp_conn_t* conn);

#endif
