#include <ngx_http.h>

#include "ngx_http_repsheet_cache.h"

int check_connection(redisContext *context) {
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

int reset_connection(repsheet_main_conf_t *main_conf) {
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

void cleanup_connection(repsheet_main_conf_t *main_conf) {
  if (main_conf->redis.connection != NULL) {
    redisFree(main_conf->redis.connection);
    main_conf->redis.connection = NULL;
  }
}

void evaluate_connection(ngx_http_request_t *r, repsheet_main_conf_t *main_conf)
{
  int connection_status = check_connection(main_conf->redis.connection);

  if (connection_status == DISCONNECTED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - No Redis connection found, creating a new connection");

    if (main_conf->redis.connection != NULL) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Redis Context Error: %s", main_conf->redis.connection->errstr);
    }

    connection_status = reset_connection(main_conf);

    if (connection_status == DISCONNECTED) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Unable to connect to Redis, bypassing Repsheet operations");

      if (main_conf->redis.connection != NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "[Repsheet] - Redis Context Error: %s", main_conf->redis.connection->errstr);
        cleanup_connection(main_conf);
      }
    }
  }
}

int blacklist(redisContext *context, char *actor, char *reason) {
  redisReply *reply;
  reply = redisCommand(context, "REPSHEET.BLACKLIST %s %s", actor, reason);

  if (reply) {
    freeReplyObject(reply);
    return OK;
  } else {
    return DISCONNECTED;
  }
}

int whitelist(redisContext *context, char *actor, char *reason) {
  redisReply *reply;
  reply = redisCommand(context, "REPSHEET.WHITELIST %s %s", actor, reason);

  if (reply) {
    freeReplyObject(reply);
    return OK;
  } else {
    return DISCONNECTED;
  }
}

int mark(redisContext *context, char *actor, char *reason) {
  redisReply *reply;
  reply = redisCommand(context, "REPSHEET.MARK %s %s", actor, reason);

  if (reply) {
    freeReplyObject(reply);
    return OK;
  } else {
    return DISCONNECTED;
  }
}

Status status(redisContext *context, char *actor) {
  redisReply *reply;
  reply = redisCommand(context, "REPSHEET.STATUS %s", actor);

  if (reply) {
    if (reply->type == REDIS_REPLY_ERROR) {
      freeReplyObject(reply);
      return DISCONNECTED;
    } else {
      char *message = reply->str;

      if (ngx_strncmp(message, "WHITELISTED", ngx_strlen(message)) == 0) {
	freeReplyObject(reply);
        return WHITELISTED;
      }

      if (ngx_strncmp(message, "BLACKLISTED", ngx_strlen(message)) == 0) {
	freeReplyObject(reply);
        return BLACKLISTED;
      }

      if (ngx_strncmp(message, "MARKED", ngx_strlen(message)) == 0) {
	freeReplyObject(reply);
        return MARKED;
      }

      freeReplyObject(reply);
      return OK;
    }
  } else {
    return DISCONNECTED;
  }
}

int get_reason(redisContext *context, char *actor, Status status, char *reason) {
  redisReply *reply;

  switch (status) {
  case WHITELISTED:
    reply = redisCommand(context, "GET %s:repsheet:ip:whitelisted", actor);
    break;
  case BLACKLISTED:
    reply = redisCommand(context, "GET %s:repsheet:ip:blacklisted", actor);
    break;
  case MARKED:
    reply = redisCommand(context, "GET %s:repsheet:ip:marked", actor);
    break;
  default:
    return INVALID;
  }

  if (reply) {
    if (reply->type == REDIS_REPLY_ERROR) {
      freeReplyObject(reply);
      return DISCONNECTED;
    } else {
      ngx_memcpy(reason, reply->str, reply->len);
      freeReplyObject(reply);
      return OK;
    }
  } else {
    return DISCONNECTED;
  }
}

int is_ip_whitelisted(Status status) {
  return status == WHITELISTED;
}

int is_ip_blacklisted(Status status) {
  return status == BLACKLISTED;
}

int is_ip_marked(Status status) {
  return status == MARKED;
}
