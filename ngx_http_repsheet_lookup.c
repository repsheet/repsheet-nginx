#include <ngx_http.h>

#include "ngx_http_repsheet_cache.h"
#include "ngx_http_repsheet_xff.h"
#include "ngx_http_repsheet_lookup.h"

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
