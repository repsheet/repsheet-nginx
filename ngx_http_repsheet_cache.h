#ifndef __NGX_HTTP_REPSHEET_CACHE_H__
#define __NGX_HTTP_REPSHEET_CACHE_H__

#include "ngx_http_repsheet_module.h"

ngx_int_t evaluate_cache_connection(ngx_http_request_t *r, repsheet_main_conf_t *main_conf);
Status status(redisContext *context, char *actor);
ngx_int_t is_ip_whitelisted(Status status);
ngx_int_t is_ip_blacklisted(Status status);
ngx_int_t is_ip_marked(Status status);
ngx_int_t get_reason(redisContext *context, char *actor, Status status, char *reason);

#endif
