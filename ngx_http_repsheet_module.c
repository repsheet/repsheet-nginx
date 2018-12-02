#include <nginx.h>
#include <ngx_config.h>
#include <ngx_http.h>

#include "ngx_http_repsheet_module.h"
#include "ngx_http_repsheet_cache.h"
#include "ngx_http_repsheet_xff.h"


static int
flag_request(ngx_http_request_t *r, char *reason)
{
  repsheet_ctx_t *ctx;
  ctx = ngx_pcalloc(r->pool, sizeof(repsheet_ctx_t));
  if (ctx == NULL) {
    return NGX_ERROR;
  }

  ngx_http_set_ctx(r, ctx, ngx_http_repsheet_module);

  ctx->flagged = 1;

  int len = strlen(reason);
  ctx->reason.len = len;
  ctx->reason.data = ngx_palloc(r->pool, len);
  if (ctx->reason.data == NULL) {
    return NGX_ERROR;
  }

  memcpy(ctx->reason.data, reason, len);

  return NGX_OK;
}


static ngx_int_t
lookup_ip(ngx_http_request_t *r, repsheet_main_conf_t *main_conf, repsheet_loc_conf_t *loc_conf)
{
  char address[INET6_ADDRSTRLEN];
  int address_code = derive_actor_address(r, loc_conf, address);

  if (address_code == NGX_DECLINED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Request was blocked by repsheet. Reason: Invalid X-Forwarded-For", address);
    return NGX_HTTP_FORBIDDEN;
  }

  Status lookup_result = status(main_conf->redis.connection, address);

  if (lookup_result == DISCONNECTED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - The Redis request failed, bypassing further operations");
    return NGX_DECLINED;
  }

  char reason[MAX_REASON_LENGTH];
  memset(reason, '\0', MAX_REASON_LENGTH);
  int reason_result = get_reason(main_conf->redis.connection, address, lookup_result, reason);

  if (lookup_result != OK) {
    if (reason_result == DISCONNECTED || reason_result == INVALID) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Could not retrieve reason");
      ngx_memcpy(reason, "UNKNOWN", 7);
    }

    if (is_ip_marked(lookup_result)) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - IP %s was found on repsheet. Reason: %s", address, reason);
      if (flag_request(r, reason) != NGX_OK) {
	ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - failed to flag request");
	return NGX_DECLINED;
      }
    }

    if (lookup_result == WHITELISTED) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - IP %s is whitelisted by repsheet. Reason: %s", address, reason);
      return NGX_DECLINED;
    } else if (lookup_result == BLACKLISTED) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - IP %s was blocked by repsheet. Reason: %s", address, reason);
      return NGX_HTTP_FORBIDDEN;
    }
  }

  return NGX_DECLINED;
}


static ngx_int_t
ngx_http_repsheet_handler(ngx_http_request_t *r)
{
  repsheet_main_conf_t *main_conf = ngx_http_get_module_main_conf(r, ngx_http_repsheet_module);
  repsheet_loc_conf_t *loc_conf = ngx_http_get_module_loc_conf(r, ngx_http_repsheet_module);

  if (!loc_conf->enabled || loc_conf->enabled == NGX_CONF_UNSET) {
    return NGX_DECLINED;
  }

  evaluate_connection(r, main_conf);

  return lookup_ip(r, main_conf, loc_conf);
}


static ngx_int_t
ngx_http_repsheet_init(ngx_conf_t *cf)
{
  ngx_http_handler_pt *h;
  ngx_http_core_main_conf_t *cmcf;

  cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

  h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
  if (h == NULL) {
    return NGX_ERROR;
  }

  *h = ngx_http_repsheet_handler;

  return NGX_OK;
}


static ngx_command_t ngx_http_repsheet_commands[] = {
  {
    ngx_string("repsheet"),
    NGX_HTTP_MAIN_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_flag_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(repsheet_loc_conf_t, enabled),
    NULL
  },
  {
    ngx_string("repsheet_proxy_headers"),
    NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_flag_slot,
    NGX_HTTP_MAIN_CONF_OFFSET,
    offsetof(repsheet_main_conf_t, proxy_headers),
    NULL
  },
  {
    ngx_string("repsheet_proxy_headers_header"),
    NGX_HTTP_MAIN_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_str_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(repsheet_loc_conf_t, proxy_headers_header),
    NULL
  },
  {
    ngx_string("repsheet_proxy_headers_fallback"),
    NGX_HTTP_MAIN_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_flag_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(repsheet_loc_conf_t, proxy_headers_fallback),
    NULL
  },
  {
    ngx_string("repsheet_redis_host"),
    NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_str_slot,
    NGX_HTTP_MAIN_CONF_OFFSET,
    offsetof(repsheet_main_conf_t, redis.host),
    NULL
  },
  {
    ngx_string("repsheet_redis_port"),
    NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_num_slot,
    NGX_HTTP_MAIN_CONF_OFFSET,
    offsetof(repsheet_main_conf_t, redis.port),
    NULL
  },
  {
    ngx_string("repsheet_redis_connection_timeout"),
    NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_num_slot,
    NGX_HTTP_MAIN_CONF_OFFSET,
    offsetof(repsheet_main_conf_t, redis.connection_timeout),
    NULL
  },
  {
    ngx_string("repsheet_redis_read_timeout"),
    NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_num_slot,
    NGX_HTTP_MAIN_CONF_OFFSET,
    offsetof(repsheet_main_conf_t, redis.read_timeout),
    NULL
  },
  ngx_null_command
};


static void*
ngx_http_repsheet_create_main_conf(ngx_conf_t *cf)
{
  repsheet_main_conf_t *conf;

  conf = ngx_pcalloc(cf->pool, sizeof(repsheet_main_conf_t));
  if (conf == NULL) {
    return NULL;
  }

  conf->redis.port = NGX_CONF_UNSET_UINT;
  conf->redis.connection_timeout = NGX_CONF_UNSET_UINT;
  conf->redis.read_timeout = NGX_CONF_UNSET_UINT;
  conf->redis.max_length = NGX_CONF_UNSET_UINT;
  conf->redis.expiry = NGX_CONF_UNSET_UINT;
  conf->redis.connection = NULL;
  conf->proxy_headers = NGX_CONF_UNSET;

  return conf;
}


static void*
ngx_http_repsheet_create_loc_conf(ngx_conf_t *cf)
{
  repsheet_loc_conf_t *conf;

  conf = ngx_pcalloc(cf->pool, sizeof(repsheet_loc_conf_t));
  if (conf == NULL) {
    return NULL;
  }

  conf->enabled = NGX_CONF_UNSET;
  conf->proxy_headers_fallback = NGX_CONF_UNSET;

  return conf;
}


static char*
ngx_http_repsheet_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
  repsheet_loc_conf_t *prev = (repsheet_loc_conf_t *) parent;
  repsheet_loc_conf_t *conf = (repsheet_loc_conf_t *) child;

  ngx_conf_merge_value(conf->enabled, prev->enabled, 0);
  ngx_conf_merge_str_value(conf->proxy_headers_header, prev->proxy_headers_header, "X-Forwarded-For");
  ngx_conf_merge_value(conf->proxy_headers_fallback, prev->proxy_headers_fallback, 0);

  return NGX_CONF_OK;
}


static ngx_http_module_t ngx_http_repsheet_module_ctx = {
  NULL,                               /* preconfiguration */
  ngx_http_repsheet_init,             /* postconfiguration */
  ngx_http_repsheet_create_main_conf, /* create main configuration */
  NULL,                               /* init main configuration */
  NULL,                               /* create server configuration */
  NULL,                               /* merge server configuration */
  ngx_http_repsheet_create_loc_conf,  /* create location configuration */
  ngx_http_repsheet_merge_loc_conf    /* merge location configuration */
};


ngx_module_t ngx_http_repsheet_module = {
  NGX_MODULE_V1,
  &ngx_http_repsheet_module_ctx, /* module context */
  ngx_http_repsheet_commands,    /* module directives */
  NGX_HTTP_MODULE,               /* module type */
  NULL,                          /* init master */
  NULL,                          /* init module */
  NULL,                          /* init process */
  NULL,                          /* init thread */
  NULL,                          /* exit thread */
  NULL,                          /* exit process */
  NULL,                          /* exit master */
  NGX_MODULE_V1_PADDING
};
