#ifndef __LIBREPSHEET_H
#define __LIBREPSHEET_H

#include "hiredis/hiredis.h"

redisContext *get_redis_context(char *host, int port, int timeout);

void increment_rule_count(redisContext *context, char *actor, char *rule);
void mark_actor(redisContext *context, char *actor);

#endif
