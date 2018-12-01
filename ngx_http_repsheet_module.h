#ifndef __NGX_HTTP_REPSHEET_MODULE_H__
#define __NGX_HTTP_REPSHEET_MODULE_H__

#define CONNECTED 0
#define DISCONNECTED -1

#define OK 0
#define WHITELISTED 1
#define BLACKLISTED 2
#define MARKED 3
#define INVALID 4

#define MAX_REASON_LENGTH 1024

#include <ngx_core.h>
#include <hiredis/hiredis.h>

typedef int Status;

typedef struct {
  ngx_str_t  host;
  ngx_uint_t port;
  ngx_uint_t connection_timeout;
  ngx_uint_t read_timeout;
  ngx_uint_t max_length;
  ngx_uint_t expiry;
  redisContext *connection;
} repsheet_redis_t;

typedef struct {
  repsheet_redis_t redis;
  ngx_flag_t user_lookup;
  ngx_flag_t ip_lookup;
  ngx_flag_t proxy_headers;
  ngx_str_t cookie;
  ngx_int_t whitelist_CIDR_cache_initial_size;
  ngx_int_t blacklist_CIDR_cache_initial_size;
  ngx_uint_t cache_expiry;
} repsheet_main_conf_t;

typedef struct {
  ngx_flag_t enabled;
  ngx_flag_t auto_blacklist;
  ngx_flag_t auto_mark;
  ngx_str_t proxy_headers_header;
  ngx_flag_t proxy_headers_fallback;
} repsheet_loc_conf_t;

typedef struct {
  ngx_flag_t flagged;
  ngx_str_t reason;
} repsheet_ctx_t;

ngx_module_t ngx_http_repsheet_module;

#endif
