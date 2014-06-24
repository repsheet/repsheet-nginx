#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "hiredis/hiredis.h"

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
  ngx_flag_t enabled;
  ngx_flag_t record;
  ngx_flag_t proxy_headers;

} repsheet_main_conf_t;

typedef struct {

} repsheet_loc_conf_t;

ngx_module_t ngx_http_repsheet_module;

static ngx_table_elt_t*
x_forwarded_for(ngx_http_request_t *r)
{
  ngx_table_elt_t *xff = NULL;

#if (nginx_version >= 1004000)
  ngx_array_t *ngx_array = &r->headers_in.x_forwarded_for;
  if (ngx_array != NULL && ngx_array->nelts > 0) {
    ngx_table_elt_t **first_elt = ngx_array->elts;
    xff = first_elt[0];
  }
#else
  xff = r->headers_in.x_forwarded_for;
#endif

  return xff;
}


static void
derive_actor_address(ngx_http_request_t *r, char address[])
{
  int length;
  ngx_table_elt_t *xff = x_forwarded_for(r);

  if (xff != NULL && xff->value.data != NULL) {
    in_addr_t addr;
    u_char *p;

    for (p = xff->value.data; p < (xff->value.data + xff->value.len); p++) {
      if (*p == ' ' || *p == ',') {
        break;
      }
    }

    // Validate the address
    length = p - xff->value.data;
    addr = ngx_inet_addr(xff->value.data, length);
    if (addr != INADDR_NONE && length <= INET_ADDRSTRLEN) {
      strncpy(address, (char *)xff->value.data, length);
      address[length] = '\0';
    } else {
      // Address was invalid, clear the array to signal the error
      memset(address, '\0', INET_ADDRSTRLEN);
    }
  } else {
    length = r->connection->addr_text.len;
    strncpy(address, (char *)r->connection->addr_text.data, length);
    address[length] = '\0';
  }
}


static ngx_int_t
ngx_http_repsheet_handler(ngx_http_request_t *r)
{
  repsheet_main_conf_t *cmcf = ngx_http_get_module_main_conf(r,ngx_http_repsheet_module);

  if (!cmcf->enabled || r->main->internal) {
    return NGX_DECLINED;
  }

  r->main->internal = 1;

  redisContext *context = get_redis_context((const char*)cmcf->redis.host.data, cmcf->redis.port, cmcf->redis.timeout);

  if (context == NULL) {
    return NGX_DECLINED;
  }

  char address[INET_ADDRSTRLEN];
  derive_actor_address(r, address);

  if (address[0] == '\0') {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Invalid X-Forwarded-For. Blocking suspected attack");
    return NGX_HTTP_FORBIDDEN;
  }

  if (is_whitelisted(context, address)) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "%s is whitelisted by repsheet", address);
    redisFree(context);
    return NGX_DECLINED;
  }

  if (is_blacklisted(context, address)) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "%s was blocked by repsheet", address);
    redisFree(context);
    return NGX_HTTP_FORBIDDEN;
  }

  if (is_on_repsheet(context, address)) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "%s was found on repsheet. No action taken", address);
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
    NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_flag_slot,
    NGX_HTTP_MAIN_CONF_OFFSET,
    offsetof(repsheet_main_conf_t, enabled),
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

  conf->enabled = NGX_CONF_UNSET;
  conf->record = NGX_CONF_UNSET;
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

  return conf;
}


static ngx_http_module_t ngx_http_repsheet_module_ctx = {
  NULL,                               /* preconfiguration */
  ngx_http_repsheet_init,             /* postconfiguration */
  ngx_http_repsheet_create_main_conf, /* create main configuration */
  NULL,                               /* init main configuration */
  NULL,                               /* create server configuration */
  NULL,                               /* merge server configuration */
  ngx_http_repsheet_create_loc_conf,  /* create location configuration */
  NULL                                /* merge location configuration */
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
