#include <ngx_http.h>

#include "ngx_http_repsheet_cache.h"
#include "ngx_http_repsheet_xff.h"
#include "ngx_http_repsheet_lookup.h"

void set_reason_header(ngx_http_request_t *r, ngx_str_t *reason) {
  ngx_table_elt_t *h;
  h = ngx_list_push(&r->headers_in.headers);
  h->hash = 1;
  ngx_str_set(&h->key, "X-Repsheet");
  h->value.data = reason->data;
  h->value.len = reason->len;
}

ngx_int_t lookup_ip(ngx_http_request_t *r, repsheet_main_conf_t *main_conf, repsheet_loc_conf_t *loc_conf)
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
      ngx_str_t value;
      ngx_memcpy(value.data, reason, ngx_strlen(reason));
      value.len = ngx_strlen(reason);
      set_reason_header(r, &value);
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
