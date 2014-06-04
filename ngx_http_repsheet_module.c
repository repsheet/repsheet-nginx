#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "repsheet.h"

typedef struct {
  ngx_str_t  host;
  ngx_uint_t port;
  ngx_uint_t timeout;
  ngx_uint_t max_length;
  ngx_uint_t expiry;

} repsheet_redis_t;

typedef struct {
  repsheet_redis_t redis;

} repsheet_main_conf_t;

typedef struct {
  ngx_flag_t enabled;
  ngx_flag_t record;
  ngx_flag_t proxy_headers;

} repsheet_loc_conf_t;

ngx_module_t ngx_http_repsheet_module;

static ngx_int_t
ngx_http_repsheet_handler(ngx_http_request_t *r)
{
  repsheet_loc_conf_t *conf;
  repsheet_main_conf_t *mconf;
  char address[INET_ADDRSTRLEN];
  int length;

  conf = ngx_http_get_module_loc_conf(r, ngx_http_repsheet_module);
  mconf = ngx_http_get_module_main_conf(r,ngx_http_repsheet_module);

  if (!conf->enabled) {
    return NGX_DECLINED;
  }

  if (r->main->internal) {
    return NGX_DECLINED;
  }

  r->main->internal = 1;

  redisContext *context = get_redis_context((const char*)mconf->redis.host.data, mconf->redis.port, mconf->redis.timeout);

  if (context == NULL) {
    return NGX_DECLINED;
  }

  ngx_table_elt_t *xfwd = NULL;

#if (nginx_version >= 1004000)
  ngx_array_t *ngx_array = &r->headers_in.x_forwarded_for;
  if (ngx_array != NULL && ngx_array->nelts > 0) {
     ngx_table_elt_t **first_elt = ngx_array->elts;
     xfwd = first_elt[0];
  }
#else
  xfwd = r->headers_in.x_forwarded_for;
#endif

  if (xfwd != NULL && xfwd->value.data != NULL) {
    in_addr_t addr;
    u_char *p;

    /* Get the first value from the XFF list.
     * Note that xfwd->value.data already has any extra whitespace removed.
     */
    for (p=xfwd->value.data; p < (xfwd->value.data + xfwd->value.len); p++) {
      if (*p == ' ' || *p == ',')
        break;
    }

    /* Validate it is a valid IP */
    length = p - xfwd->value.data;
    addr = ngx_inet_addr(xfwd->value.data, length);
    if (addr != INADDR_NONE && length <= INET_ADDRSTRLEN) {
      strncpy(address, (char *)xfwd->value.data, length);
      address[length] = '\0';
    }
  }

  if (address == NULL) {
    length = r->connection->addr_text.len;
    strncpy(address, (char *)r->connection->addr_text.data, length);
    address[length] = '\0';
  }

  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "%s Address is %s", "[repsheet]", address);

  if (is_whitelisted(context, address)) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "%s %s was allowed by the repsheet whitelist", "[repsheet]", address);
    redisFree(context);
    return NGX_DECLINED;
  }

  if (is_blacklisted(context, address)) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "%s %s was blocked by the repsheet blacklist", "[repsheet]", address);
    redisFree(context);
    return NGX_HTTP_FORBIDDEN;
  }

  if (is_on_repsheet(context, address)) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "%s %s was found on the repsheet", "[repsheet]", address);
    ngx_table_elt_t *h;
    ngx_str_t label = ngx_string("X-Repsheet");
    ngx_str_t val = ngx_string("true");
    h = ngx_list_push(&r->headers_in.headers);
    h->hash = 1;
    h->key = label;
    h->value = val;
  }

  redisFree(context);

  return NGX_DECLINED;
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
    NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_flag_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(repsheet_loc_conf_t, enabled),
    NULL
  },
  {
    ngx_string("repsheet_proxy_headers"),
    NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_flag_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(repsheet_loc_conf_t, proxy_headers),
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
    ngx_string("repsheet_redis_timeout"),
    NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_num_slot,
    NGX_HTTP_MAIN_CONF_OFFSET,
    offsetof(repsheet_main_conf_t, redis.timeout),
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
  conf->redis.timeout = NGX_CONF_UNSET_UINT;
  conf->redis.max_length = NGX_CONF_UNSET_UINT;
  conf->redis.expiry = NGX_CONF_UNSET_UINT;

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
  conf->record = NGX_CONF_UNSET;
  conf->proxy_headers = NGX_CONF_UNSET;

  return conf;
}


static char*
ngx_http_repsheet_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
  repsheet_loc_conf_t *prev = (repsheet_loc_conf_t *)parent;
  repsheet_loc_conf_t *conf = (repsheet_loc_conf_t *)child;

  ngx_conf_merge_value(conf->enabled, prev->enabled, 0);
  ngx_conf_merge_value(conf->record, prev->record, 0);
  ngx_conf_merge_value(conf->proxy_headers, prev->proxy_headers, 0);

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
