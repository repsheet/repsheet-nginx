#include <string.h>

#include "repsheet.h"
#include "common.h"

/**
 * @file common.c
 * @author Aaron Bedra
 * @date 12/09/2014
 */

void populate_reason(redisReply *reply, char *reason)
{
  snprintf(reason, MAX_REASON_LENGTH, "%s", reply->str);
}

int set_list(redisContext *context, const char *prefix, char *keyspace, char *list, const char *value)
{
  redisReply *reply;
  reply = redisCommand(context, "SET %s:repsheet:%s:%s %s", prefix, keyspace, list, value);

  if (reply) {
    freeReplyObject(reply);
    return LIBREPSHEET_OK;
  } else {
    return DISCONNECTED;
  }
}

int set_block(redisContext *context, const char *prefix, char *list, const char *value)
{
  redisReply *reply;
  reply = redisCommand(context, "SET %s:repsheet:cidr:%s %s", prefix, list, value);
  if (reply && reply->type != REDIS_REPLY_NIL) {
    reply = redisCommand(context, "SADD repsheet:cidr:%s %s", list, prefix);
    if (reply && reply->type != REDIS_REPLY_NIL) {
      freeReplyObject(reply);
      return LIBREPSHEET_OK;
    } else {
      return DISCONNECTED;
    }
  } else {
    return DISCONNECTED;
  }
}
