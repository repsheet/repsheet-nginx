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
  size_t s = reply->len > MAX_REASON_LENGTH ? MAX_REASON_LENGTH : reply->len;
  strncpy(reason, reply->str, s);
  reason[s] = '\0';
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
