#include <nginx.h>
#include <ngx_config.h>
#include <ngx_http.h>

#include "ngx_http_repsheet_module.h"
#include "ngx_http_repsheet_cache.h"
#include "ngx_http_repsheet_xff.h"

ngx_str_t ngx_http_repsheet_variable_name = ngx_string("repsheet");
ngx_str_t ngx_http_repsheet_flagged = ngx_string("true");
ngx_str_t ngx_http_repsheet_default = ngx_string("");
ngx_str_t ngx_http_repsheet_reason_variable_name = ngx_string("repsheet_reason");


static ngx_int_t
ngx_http_repsheet_reason_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
  repsheet_ctx_t *ctx = ngx_http_get_module_ctx(r, ngx_http_repsheet_module);

  if (ctx && ctx->reason.data) {
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->len = ctx->reason.len;
    v->data = ctx->reason.data;
  } else {
    v->not_found = 1;
  }

  return NGX_OK;
}


static ngx_int_t
ngx_http_repsheet_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
  repsheet_ctx_t *ctx = ngx_http_get_module_ctx(r, ngx_http_repsheet_module);

  if (ctx && ctx->flagged) {
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->len = ngx_http_repsheet_flagged.len;
    v->data = ngx_http_repsheet_flagged.data;
  } else {
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->len = ngx_http_repsheet_default.len;
    v->data = ngx_http_repsheet_default.data;
  }

  return NGX_OK;
}


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
ngx_http_repsheet_preconf_add_variables(ngx_conf_t *cf)
{
  ngx_http_variable_t *var;
  var = ngx_http_add_variable(cf, &ngx_http_repsheet_variable_name, NGX_HTTP_VAR_NOCACHEABLE);
  if (var == NULL) {
    return NGX_ERROR;
  }
  var->get_handler = ngx_http_repsheet_variable;
  var->data = 0;

  var = ngx_http_add_variable(cf, &ngx_http_repsheet_reason_variable_name, NGX_HTTP_VAR_NOCACHEABLE);
  if (var == NULL) {
    return NGX_ERROR;
  }
  var->get_handler = ngx_http_repsheet_reason_variable;
  var->data = 0;

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

  if (is_ip_marked(lookup_result)) {
    // TODO: get reason
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - IP %s was found on repsheet. Reason: TODO", address);
    if (flag_request(r, "TODO") != NGX_OK) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - failed to flag request");
      // TODO: should this really return here?
      return NGX_DECLINED;
    }
  }

  if (lookup_result == DISCONNECTED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - The Redis request failed, bypassing further operations");
    return NGX_DECLINED;
  } else if (lookup_result == WHITELISTED) {
    // TODO: get reason
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - IP %s is whitelisted by repsheet. Reason: TODO", address);
    return NGX_DECLINED;
  } else if (lookup_result == BLACKLISTED) {
    // TODO: get reason
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - IP %s was blocked by repsheet. Reason: TODO", address);
    return NGX_HTTP_FORBIDDEN;
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

  int connection_status = check_connection(main_conf->redis.connection);

  if (connection_status == DISCONNECTED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - No Redis connection found, creating a new connection");

    if (main_conf->redis.connection != NULL) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Redis Context Error: %s", main_conf->redis.connection->errstr);
    }

    connection_status = reset_connection(main_conf);

    if (connection_status == DISCONNECTED) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Unable to establish a connection to Redis, bypassing Repsheet operations");

      if (main_conf->redis.connection != NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Redis Context Error: %s", main_conf->redis.connection->errstr);
        cleanup_connection(main_conf);
      }

      return NGX_DECLINED;
    }
  }

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
  ngx_http_repsheet_preconf_add_variables, /* preconfiguration */
  ngx_http_repsheet_init,                  /* postconfiguration */
  ngx_http_repsheet_create_main_conf,      /* create main configuration */
  NULL,                                    /* init main configuration */
  NULL,                                    /* create server configuration */
  NULL,                                    /* merge server configuration */
  ngx_http_repsheet_create_loc_conf,       /* create location configuration */
  ngx_http_repsheet_merge_loc_conf         /* merge location configuration */
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
