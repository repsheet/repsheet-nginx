#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <hiredis/hiredis.h>
#include <repsheet.h>

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
  ngx_str_t header_content;
} repsheet_loc_conf_t;

typedef struct {
    ngx_flag_t flagged;
  ngx_str_t reason;
} repsheet_ctx_t;

ngx_module_t ngx_http_repsheet_module;

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


static ngx_table_elt_t*
extract_proxy_header(ngx_http_request_t *r, repsheet_loc_conf_t *loc_conf)
{
  ngx_list_part_t *part;
  ngx_table_elt_t *h;
  ngx_uint_t i;

  part = &r->headers_in.headers.part;
  h = part->elts;

  for (i = 0; /**/; i++) {
    if (i >= part->nelts) {
      if (part->next == NULL) {
	break;
      }

      part = part->next;
      h = part->elts;
      i = 0;
    }

    if (ngx_strncmp(h[i].key.data, loc_conf->proxy_headers_header.data, h[i].key.len) == 0) {
      return &h[i];
    }
  }

  return NULL;
}


static int
validate_actor_address(ngx_http_request_t *r, ngx_table_elt_t *xff, char *address)
{
  int result;

  if (xff == NULL) {
    memcpy(address, r->connection->addr_text.data, r->connection->addr_text.len);
    address[r->connection->addr_text.len] = '\0';
    return NGX_OK;
  } else {
    result = remote_address((char *)r->connection->addr_text.data, (char*)xff->value.data, address);
    if (result == BLACKLISTED) {
      return NGX_DECLINED;
    } else {
      return NGX_OK;
    }
  }
}


static int
derive_actor_address(ngx_http_request_t *r, repsheet_loc_conf_t *loc_conf, char *address)
{
  int result;
  int alternate = 0;
  ngx_table_elt_t *xff;

  if (ngx_strncmp(loc_conf->proxy_headers_header.data, "X-Forwarded-For", 15) == 0) {
    xff = x_forwarded_for(r);
  } else {
    alternate = 1;
    xff = extract_proxy_header(r, loc_conf);
  }

  result = validate_actor_address(r, xff, address);

  if (result == NGX_DECLINED && alternate && loc_conf->proxy_headers_fallback) {
    return validate_actor_address(r, x_forwarded_for(r), address);
  } else {
    return result;
  }
}


static void
cleanup_connection(repsheet_main_conf_t *main_conf)
{
  if (main_conf->redis.connection != NULL) {
    redisFree(main_conf->redis.connection);
    main_conf->redis.connection = NULL;
  }
}


static int
reset_connection(repsheet_main_conf_t *main_conf)
{
  cleanup_connection(main_conf);

  redisContext *context = repsheet_connect((const char*)main_conf->redis.host.data,
                                           main_conf->redis.port,
                                           main_conf->redis.connection_timeout,
                                           main_conf->redis.read_timeout);

  if (context == NULL) {
    return NGX_DECLINED;
  } else {
    main_conf->redis.connection = context;
    return NGX_OK;
  }
}

static ngx_int_t
lookup_user(ngx_http_request_t *r, repsheet_main_conf_t *main_conf, repsheet_loc_conf_t *loc_conf)
{
  int user_status = NGX_DECLINED;
  char reason_user[MAX_REASON_LENGTH];
  ngx_int_t location;
  ngx_str_t cookie_value;

  reason_user[0] = '\0';

  location = ngx_http_parse_multi_header_lines(&r->headers_in.cookies, &main_conf->cookie, &cookie_value);
  if (location == NGX_DECLINED) {
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "[Repsheet] - Could not locate %V cookie", &main_conf->cookie);
  } else {
    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "[Repsheet] - Got value for %V cookie: %V", &main_conf->cookie, &cookie_value);
    char lookup_value[cookie_value.len + 1];
    memcpy(lookup_value, cookie_value.data, cookie_value.len);
    lookup_value[cookie_value.len] = '\0';
    user_status = actor_status(main_conf->redis.connection, (const char *)lookup_value, USER, reason_user);

    if (is_user_marked(main_conf->redis.connection, (const char *)lookup_value, reason_user)) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - USER %V was found on repsheet. Reason: %s", &cookie_value, reason_user);
      if (flag_request(r, reason_user) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - failed to flag request");
        return NGX_DECLINED;
      }
    }
  }

  if (user_status == DISCONNECTED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - The Redis request failed, bypassing further operations");
    return NGX_DECLINED;
  } else if (user_status == WHITELISTED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - User %V is whitelisted by repsheet. Reason: %s", &cookie_value, reason_user);
    return NGX_DECLINED;
  } else if (user_status == BLACKLISTED) {
    if (reason_user[0] != '\0') {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - User %V was blocked by repsheet. Reason: %s", &cookie_value, reason_user);
    } else {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - User %V was blocked by repsheet. No reason provided", &cookie_value);
    }
    return NGX_HTTP_FORBIDDEN;
  }

  return NGX_DECLINED;
}

static ngx_int_t
lookup_ip(ngx_http_request_t *r, repsheet_main_conf_t *main_conf, repsheet_loc_conf_t *loc_conf)
{
  int address_code;
  int ip_status = LIBREPSHEET_OK;
  char address[INET6_ADDRSTRLEN];
  char reason_ip[MAX_REASON_LENGTH];

  address[0] = reason_ip[0] = '\0';

  address_code = derive_actor_address(r, loc_conf, address);

  if (address_code == NGX_DECLINED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Request was blocked by repsheet. Reason: Invalid X-Forwarded-For", address);
    return NGX_HTTP_FORBIDDEN;
  }

  if (is_ip_marked(main_conf->redis.connection, address, reason_ip)) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - IP %s was found on repsheet. Reason: %s", address, reason_ip);
    if (flag_request(r, reason_ip) != NGX_OK) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - failed to flag request");
      return NGX_DECLINED;
    }
  }

  ip_status = actor_status(main_conf->redis.connection, address, IP, reason_ip);

  if (ip_status == DISCONNECTED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - The Redis request failed, bypassing further operations");
    return NGX_DECLINED;
  } else if (ip_status == WHITELISTED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - IP %s is whitelisted by repsheet. Reason: %s", address, reason_ip);
    return NGX_DECLINED;
  } else if (ip_status == BLACKLISTED) {
    if (reason_ip[0] != '\0') {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - IP %s was blocked by repsheet. Reason: %s", address, reason_ip);
    } else {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - IP %s was blocked by repsheet. No reason provided", address);
    }
    return NGX_HTTP_FORBIDDEN;
  }

  return NGX_DECLINED;
}


static ngx_int_t
ngx_http_repsheet_handler(ngx_http_request_t *r)
{
  static int set_cache_sizes = 0;

  repsheet_main_conf_t *main_conf = ngx_http_get_module_main_conf(r, ngx_http_repsheet_module);

  if ( set_cache_sizes == 0 ) {
    set_cache_sizes = 1;

    int n;

    n = main_conf->whitelist_CIDR_cache_initial_size;
    if ( n != NGX_CONF_UNSET ) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - setting whitelist CIDR cache size to : %d", n);
      set_initial_whitelist_size( n );
    }

    n = main_conf->blacklist_CIDR_cache_initial_size;
    if ( n != NGX_CONF_UNSET ) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - setting blacklist CIDR cache size to : %d", n);
      set_initial_blacklist_size( n );
    }

    n = main_conf->cache_expiry;
    if ( n != NGX_CONF_UNSET ) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - setting CIDR cache expiry to : %d seconds", n);
      set_cache_expiry( n );
    }
  }

  repsheet_loc_conf_t  *loc_conf  = ngx_http_get_module_loc_conf(r, ngx_http_repsheet_module);

  if (!loc_conf->enabled || loc_conf->enabled == NGX_CONF_UNSET) {
    return NGX_DECLINED;
  }

  loc_conf->header_content.data = NULL;

  int connection_status = check_connection(main_conf->redis.connection);
  if (connection_status == DISCONNECTED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - No Redis connection found, creating a new connection");

    if (main_conf->redis.connection != NULL) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Redis Context Error: %s", main_conf->redis.connection->errstr);
    }

    connection_status = reset_connection(main_conf);

    if (connection_status == NGX_DECLINED) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Unable to establish a connection to Redis, bypassing Repsheet operations");

      if (main_conf->redis.connection != NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Redis Context Error: %s", main_conf->redis.connection->errstr);
        cleanup_connection(main_conf);
      }

      return NGX_DECLINED;
    }
  }

  if (loc_conf->auto_blacklist || loc_conf->auto_mark) {
    int address_code;
    char address[INET6_ADDRSTRLEN];
    address[0] = '\0';

    address_code = derive_actor_address(r, loc_conf, address);

    if (address_code == NGX_DECLINED) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Request was blocked by repsheet. Reason: Invalid X-Forwarded-For", address);
      return NGX_HTTP_FORBIDDEN;
    }

    if (loc_conf->auto_blacklist) {
      blacklist(main_conf->redis.connection, address, IP, "bad robot");
      return NGX_HTTP_FORBIDDEN;
    }

    if (loc_conf->auto_mark) {
      mark(main_conf->redis.connection, address, IP, "bad robot");
    }
  }

  if (main_conf->user_lookup) {
    int user_status = lookup_user(r, main_conf, loc_conf);
    if (user_status != NGX_DECLINED) {
      return user_status;
    }
  }

  if (main_conf->ip_lookup) {
    int ip_status = lookup_ip(r, main_conf, loc_conf);
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
  {
    ngx_string("repsheet_user_cookie"),
    NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_str_slot,
    NGX_HTTP_MAIN_CONF_OFFSET,
    offsetof(repsheet_main_conf_t, cookie),
    NULL
  },
  {
    ngx_string("repsheet_blacklist"),
    NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_flag_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(repsheet_loc_conf_t, auto_blacklist),
    NULL
  },
  {
    ngx_string("repsheet_mark"),
    NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_flag_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(repsheet_loc_conf_t, auto_mark),
    NULL
  },
  {
    ngx_string("repsheet_whitelist_CIDR_cache_initial_size"),
    NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_num_slot,
    NGX_HTTP_MAIN_CONF_OFFSET,
    offsetof(repsheet_main_conf_t, whitelist_CIDR_cache_initial_size),
    NULL
  },
  {
    ngx_string("repsheet_blacklist_CIDR_cache_initial_size"),
    NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_num_slot,
    NGX_HTTP_MAIN_CONF_OFFSET,
    offsetof(repsheet_main_conf_t, blacklist_CIDR_cache_initial_size),
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
  {
    ngx_string("repsheet_cache_expiry"),
    NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_num_slot,
    NGX_HTTP_MAIN_CONF_OFFSET,
    offsetof(repsheet_main_conf_t, cache_expiry),
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

  conf->ip_lookup = NGX_CONF_UNSET;
  conf->user_lookup = NGX_CONF_UNSET;

  conf->proxy_headers = NGX_CONF_UNSET;
  conf->whitelist_CIDR_cache_initial_size = NGX_CONF_UNSET;
  conf->blacklist_CIDR_cache_initial_size = NGX_CONF_UNSET;
  conf->cache_expiry = NGX_CONF_UNSET_UINT;

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
  conf->auto_blacklist = NGX_CONF_UNSET;
  conf->auto_mark = NGX_CONF_UNSET;
  conf->header_content.data = NULL;
  conf->proxy_headers_fallback = NGX_CONF_UNSET;

  return conf;
}


static char*
ngx_http_repsheet_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
  repsheet_loc_conf_t *prev = (repsheet_loc_conf_t *)parent;
  repsheet_loc_conf_t *conf = (repsheet_loc_conf_t *)child;

  ngx_conf_merge_value(conf->enabled, prev->enabled, 0);
  ngx_conf_merge_value(conf->auto_blacklist, prev->auto_blacklist, 0);
  ngx_conf_merge_value(conf->auto_mark, prev->auto_mark, 0);
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
