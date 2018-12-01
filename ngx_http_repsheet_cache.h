#ifndef __NGX_HTTP_REPSHEET_CACHE_H__
#define __NGX_HTTP_REPSHEET_CACHE_H__

#include "ngx_http_repsheet_module.h"

int check_connection(redisContext *context);
int reset_connection(repsheet_main_conf_t *main_conf);
void cleanup_connection(repsheet_main_conf_t *main_conf);
int blacklist(redisContext *context, char *actor, char *reason);
int whitelist(redisContext *context, char *actor, char *reason);
int mark(redisContext *context, char *actor, char *reason);
int status(redisContext *context, char *actor);
int is_ip_whitelisted(Status status);
int is_ip_blacklisted(Status status);
int is_ip_marked(Status status);
int get_reason(redisContext *context, char *actor, Status status, char *reason);

#endif
