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
redisContext *repsheet_connect(const char *host, int port, int connect_timeout, int read_timeout);
int repsheet_reconnect(redisContext *context);
int check_connection(redisContext *context);

int actor_status(redisContext *context, const char *actor, int type, char *reason);

int remote_address(char *connected_address, char *xff_header, char *address);

int blacklist(redisContext *context, const char *actor, int type, const char *reason);
int whitelist(redisContext *context, const char *actor, int type, const char *reason);
int mark(redisContext *context, const char *actor, int type, const char *reason);

void set_initial_whitelist_size(int new_size);
void set_initial_blacklist_size(int new_size);

#endif
