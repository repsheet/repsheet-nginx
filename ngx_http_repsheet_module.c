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
  redisContext *connection;
} repsheet_redis_t;

typedef struct {
  repsheet_redis_t redis;
  ngx_flag_t user_lookup;
  ngx_flag_t ip_lookup;
  ngx_flag_t proxy_headers;
  ngx_str_t cookie;
} repsheet_main_conf_t;

typedef struct {
  ngx_flag_t enabled;
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


static int
derive_actor_address(ngx_http_request_t *r, char *address)
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

    length = p - xff->value.data;
    addr = ngx_inet_addr(xff->value.data, length);
    if (addr != INADDR_NONE && length <= INET_ADDRSTRLEN) {
      strncpy(address, (char *)xff->value.data, length);
      address[length] = '\0';
      return NGX_OK;
    } else {
      return NGX_DECLINED;
    }
  } else {
    length = r->connection->addr_text.len;
    strncpy(address, (char *)r->connection->addr_text.data, length);
    address[length] = '\0';
    return NGX_OK;
  }
}


static int
reset_connection(ngx_http_request_t *r, repsheet_main_conf_t *main_conf)
{
  redisContext *context = get_redis_context((const char*)main_conf->redis.host.data, main_conf->redis.port, main_conf->redis.timeout);

  if (context == NULL) {
    return NGX_DECLINED;
  } else {
    main_conf->redis.connection = context;
    return NGX_OK;
  }
}


static void
set_repsheet_header(ngx_http_request_t *r)
{
  ngx_table_elt_t *h;
  h = ngx_list_push(&r->headers_in.headers);
  h->hash = 1;
  ngx_str_set(&h->key, "X-Repsheet");
  ngx_str_set(&h->value, "true");
}


static ngx_int_t
lookup_user(ngx_http_request_t *r, repsheet_main_conf_t *main_conf)
{
  int user_status = NGX_DECLINED;
  char reason_user[MAX_REASON_LENGTH];
  ngx_int_t location;
  ngx_str_t cookie_value;

  location = ngx_http_parse_multi_header_lines(&r->headers_in.cookies, &main_conf->cookie, &cookie_value);
  if (location == NGX_DECLINED) {
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "[Repsheet] - Could not locate %V cookie", &main_conf->cookie);
  } else {
    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "[Repsheet] - Got value for %V cookie: %V", &main_conf->cookie, &cookie_value);
    user_status = actor_status(main_conf->redis.connection, (const char *)cookie_value.data, USER, reason_user);
  }

  if (user_status == DISCONNECTED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - The Redis request failed, bypassing further operations");
    return NGX_DECLINED;
  } else if (user_status == WHITELISTED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - User %V is whitelisted by repsheet. Reason: %s", &cookie_value, reason_user);
    return NGX_DECLINED;
  } else if (user_status == BLACKLISTED) {
    if (reason_user != NULL) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - User %V was blocked by repsheet. Reason: %s", &cookie_value, reason_user);
    } else {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - User %V was blocked by repsheet. No reason provided", &cookie_value);
    }
    return NGX_HTTP_FORBIDDEN;
  } else if (user_status == MARKED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - User %s was found on repsheet. No action taken", &cookie_value);
    set_repsheet_header(r);
  }

  return NGX_DECLINED;
}

static ngx_int_t
lookup_ip(ngx_http_request_t *r, repsheet_main_conf_t *main_conf)
{
  int address_code;
  int ip_status = LIBREPSHEET_OK;
  char address[INET_ADDRSTRLEN];
  char reason_ip[MAX_REASON_LENGTH];

  address_code = derive_actor_address(r, address);

  if (address_code == NGX_DECLINED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Request was blocked by repsheet. Reason: Invalid X-Forwarded-For", address);
    return NGX_HTTP_FORBIDDEN;
  }

  ip_status = actor_status(main_conf->redis.connection, address, IP, reason_ip);

  if (ip_status == DISCONNECTED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - The Redis request failed, bypassing further operations");
    return NGX_DECLINED;
  } else if (ip_status == WHITELISTED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - IP %s is whitelisted by repsheet. Reason: %s", address, reason_ip);
    return NGX_DECLINED;
  } else if (ip_status == BLACKLISTED) {
    if (reason_ip != NULL) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - IP %s was blocked by repsheet. Reason: %s", address, reason_ip);
    } else {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - IP %s was blocked by repsheet. No reason provided", address);
    }
    return NGX_HTTP_FORBIDDEN;
  } else if (ip_status == MARKED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - IP %s was found on repsheet. No action taken", address);
    set_repsheet_header(r);
  }

  return NGX_DECLINED;
}

static ngx_int_t
ngx_http_repsheet_handler(ngx_http_request_t *r)
{
  repsheet_loc_conf_t  *loc_conf  = ngx_http_get_module_loc_conf(r, ngx_http_repsheet_module);

  if (!loc_conf->enabled || loc_conf->enabled == NGX_CONF_UNSET || r->main->internal) {
    return NGX_DECLINED;
  }

  repsheet_main_conf_t *main_conf = ngx_http_get_module_main_conf(r, ngx_http_repsheet_module);

  int connection_status = check_connection(main_conf->redis.connection);
  if (connection_status == DISCONNECTED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - No Redis connection found, creating a new connection");
    connection_status = reset_connection(r, main_conf);
    if (connection_status == NGX_DECLINED) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Unable to establish a connection to Redis, bypassing Repsheet operations");
      return NGX_DECLINED;
    }
  }

  if (main_conf->user_lookup) {
    int user_status = lookup_user(r, main_conf);
    if (user_status != NGX_DECLINED) {
      return user_status;
    }
  }

  if (main_conf->ip_lookup) {
    int ip_status = lookup_ip(r, main_conf);
    if (ip_status != NGX_DECLINED) {
      return ip_status;
    }
  }

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
    NGX_HTTP_MAIN_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_flag_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(repsheet_loc_conf_t, enabled),
    NULL
  },
  {
    ngx_string("repsheet_user_lookup"),
    NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_flag_slot,
    NGX_HTTP_MAIN_CONF_OFFSET,
    offsetof(repsheet_main_conf_t, user_lookup),
    NULL
  },
  {
    ngx_string("repsheet_ip_lookup"),
    NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_flag_slot,
    NGX_HTTP_MAIN_CONF_OFFSET,
    offsetof(repsheet_main_conf_t, ip_lookup),
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
  {
    ngx_string("repsheet_user_cookie"),
    NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_str_slot,
    NGX_HTTP_MAIN_CONF_OFFSET,
    offsetof(repsheet_main_conf_t, cookie),
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
  conf->redis.connection = NULL;

  conf->ip_lookup = NGX_CONF_UNSET;
  conf->user_lookup = NGX_CONF_UNSET;

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

  return conf;
}


static char*
ngx_http_repsheet_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
  repsheet_loc_conf_t *prev = (repsheet_loc_conf_t *)parent;
  repsheet_loc_conf_t *conf = (repsheet_loc_conf_t *)child;

  ngx_conf_merge_value(conf->enabled, prev->enabled, 0);

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
