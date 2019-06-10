#include <ngx_http.h>

#include "ngx_http_repsheet_cache.h"

static int check_connection(redisContext *context) {
  if (context == NULL || context->err) {
    return DISCONNECTED;
  }

  redisReply *reply;
  reply = redisCommand(context, "PING");
  if (reply) {
    if (reply->type == REDIS_REPLY_ERROR) {
      freeReplyObject(reply);
      return DISCONNECTED;
    } else {
      freeReplyObject(reply);
      return CONNECTED;
    }
  } else {
    return DISCONNECTED;
  }
}

static void cleanup_connection(repsheet_main_conf_t *main_conf) {
  if (main_conf->redis.connection != NULL) {
    redisFree(main_conf->redis.connection);
    main_conf->redis.connection = NULL;
  }
}

static int reset_connection(repsheet_main_conf_t *main_conf) {
  cleanup_connection(main_conf);

  const char *host = (const char *) main_conf->redis.host.data;
  int port = main_conf->redis.port;
  int connection_timeout = main_conf->redis.connection_timeout;
  int read_timeout = main_conf->redis.read_timeout;

  struct timeval ct = {0, (connection_timeout > 0) ? (connection_timeout * 1000) : 10000};
  struct timeval rt = {0, (read_timeout > 0) ? (read_timeout * 1000) : 10000};

  redisContext *context = redisConnectWithTimeout(host, port, ct);
  if (context == NULL || context->err) {
    if (context) {
      redisFree(context);
    }
    return DISCONNECTED;
  } else {
    redisSetTimeout(context, rt);
    main_conf->redis.connection = context;
    return CONNECTED;
  }
}

ngx_int_t evaluate_cache_connection(ngx_http_request_t *r, repsheet_main_conf_t *main_conf)
{
  int connection_status = check_connection(main_conf->redis.connection);

  if (connection_status == DISCONNECTED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Redis disconnected, creating new connection");

    connection_status = reset_connection(main_conf);

    if (connection_status == DISCONNECTED) {
      if (main_conf->redis.connection != NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Redis Context Error: %s", main_conf->redis.connection->errstr);
        cleanup_connection(main_conf);
      }

      return NGX_ERROR;
    }
  }

  return NGX_OK;
}

void free_cache_status(repsheet_cache_status_t *cache_status) {
  free(cache_status->status_str);
  free(cache_status->reason);
  free(cache_status);
}

repsheet_cache_status_t *status(redisContext *context, char *actor) {
  repsheet_cache_status_t *cache_status = malloc(sizeof(repsheet_cache_status_t));
  redisReply *reply = redisCommand(context, "REPSHEET.STATUS %s", actor);

  if (reply) {
    if (reply->type == REDIS_REPLY_ERROR) {
      freeReplyObject(reply);
      cache_status->status = DISCONNECTED;
      return cache_status;
    } else if (reply->type != REDIS_REPLY_ARRAY) {
      freeReplyObject(reply);
      cache_status->status = ERROR;
      return cache_status;
    } else {
      if (reply->elements != 2) {
	cache_status->status = ERROR;
	return cache_status;
      }

      cache_status->status_str = strdup(reply->element[0]->str);
      cache_status->reason = strdup(reply->element[1]->str);
      freeReplyObject(reply);

      if (ngx_strncmp(cache_status->status_str, "WHITELISTED", ngx_strlen(cache_status->status_str)) == 0) {
	cache_status->status = WHITELISTED;
        return cache_status;
      } else if (ngx_strncmp(cache_status->status_str, "BLACKLISTED", ngx_strlen(cache_status->status_str)) == 0) {
	cache_status->status = BLACKLISTED;
        return cache_status;
      } else if (ngx_strncmp(cache_status->status_str, "MARKED", ngx_strlen(cache_status->status_str)) == 0) {
	cache_status->status = MARKED;
        return cache_status;
      } else {
	cache_status->status = OK;
	return cache_status;
      }
    }
  } else {
    cache_status->status = DISCONNECTED;
    return cache_status;
  }
}
