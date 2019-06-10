#ifndef __NGX_HTTP_REPSHEET_CACHE_H__
#define __NGX_HTTP_REPSHEET_CACHE_H__

#include "ngx_http_repsheet_module.h"

ngx_int_t evaluate_cache_connection(ngx_http_request_t *r, repsheet_main_conf_t *main_conf);
repsheet_cache_status_t *status(redisContext *context, char *actor);
void free_cache_status(repsheet_cache_status_t *cache_status);

#endif
