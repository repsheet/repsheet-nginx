#ifndef __LIBREPSHEET_H
#define __LIBREPSHEET_H

#include "hiredis/hiredis.h"

redisContext *get_redis_context(char *host, int port, int timeout);

#endif
