#ifndef __REPSHEET_H
#define __REPSHEET_H

#define TRUE 1
#define FALSE 0

#define WHITELISTED 2
#define MARKED 3
#define BLACKLISTED 4
#define UNSUPPORTED 5
#define LIBREPSHEET_OK 6

#define IP 7
#define USER 8
#define BLOCK 9

#define DISCONNECTED -1
#define NIL -2

#define MAX_REASON_LENGTH 1024

#include "hiredis/hiredis.h"

redisContext *get_redis_context(const char *host, int port, int timeout);
int check_connection(redisContext *context);

int country_status(redisContext *context, const char *country_code);
int actor_status(redisContext *context, const char *actor, int type, char *reason);

int remote_address(char *connected_address, char *xff_header, char *address);

int blacklist_actor(redisContext *context, const char *actor, int type, const char *reason);
int mark_actor(redisContext *context, const char *actor, int type, const char *reason);

#endif
