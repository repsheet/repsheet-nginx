#ifndef __REPSHEET_H
#define __REPSHEET_H

#include <string.h>
#include "hiredis/hiredis.h"

#define TRUE 1
#define FALSE 0

redisContext *get_redis_context(char *host, int port, int timeout);

void increment_rule_count(redisContext *context, char *actor, char *rule);

void mark_actor(redisContext *context, char *actor);
void blacklist_actor(redisContext *context, char *actor);
void whitelist_actor(redisContext *context, char *actor);

int is_on_repsheet(redisContext *context, char *actor);
int is_blacklisted(redisContext *context, char *actor);
int is_whitelisted(redisContext *context, char *actor);

void expire(redisContext *context, char *actor, char *label, int expiry);
void blacklist_and_expire(redisContext *context, char *actor, int expiry, char *reason);

#endif
