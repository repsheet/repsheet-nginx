#include <ngx_http.h>

#include "ngx_http_repsheet_cache.h"
#include "ngx_http_repsheet_xff.h"
#include "ngx_http_repsheet_lookup.h"

void set_repsheet_header(ngx_http_request_t *r) {
  ngx_table_elt_t *h;
  h = ngx_list_push(&r->headers_in.headers);
  h->hash = 1;
  ngx_str_set(&h->key, "X-Repsheet");
  ngx_str_set(&h->value, "true");
}

ngx_int_t lookup_ip(ngx_http_request_t *r, repsheet_main_conf_t *main_conf, repsheet_loc_conf_t *loc_conf, ngx_int_t cache_connection_status)
{
  if (cache_connection_status == NGX_ERROR) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Cache connection error, bypassing Repsheet");
    return NGX_DECLINED;
  }

  char address[INET6_ADDRSTRLEN];
  int address_code = derive_actor_address(r, loc_conf, address);

  if (address_code == NGX_DECLINED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Request was blocked by repsheet. Reason: Invalid X-Forwarded-For", address);
    return NGX_HTTP_FORBIDDEN;
  }

  repsheet_cache_status_t *cache_status = status(main_conf->redis.connection, address);

  if (cache_status->status == DISCONNECTED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - The Redis request failed, bypassing further operations");
    free_cache_status(cache_status);
    return NGX_DECLINED;
  }

  if (cache_status->status != OK) {
    if (cache_status->status == INVALID) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Could not retrieve reason, bypassing");
      free_cache_status(cache_status);
      return NGX_DECLINED;
    } else if (cache_status->status == MARKED) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - IP %s was found on repsheet. Reason: %s", address, cache_status->reason);
      set_repsheet_header(r);
      free_cache_status(cache_status);
      return NGX_DECLINED;
    } else if (cache_status->status == WHITELISTED) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - IP %s is whitelisted by repsheet. Reason: %s", address, cache_status->reason);
      free_cache_status(cache_status);
      return NGX_DECLINED;
    } else if (cache_status->status == BLACKLISTED) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - IP %s was blocked by repsheet. Reason: %s", address, cache_status->reason);
      free_cache_status(cache_status);
      return NGX_HTTP_FORBIDDEN;
    } else {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Invalid status %s, bypassing", cache_status->status_str);
      free_cache_status(cache_status);
      return NGX_DECLINED;
    }
  } else {
    free_cache_status(cache_status);
    return NGX_DECLINED;
  }
}
