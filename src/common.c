#include "repsheet.h"
#include "common.h"

int set(redisContext *context, const char *prefix, char *keyspace, const char *value)
{
  redisReply *reply;
  reply = redisCommand(context, "SET %s:repsheet:%s %s", prefix, keyspace, value);

  if (reply) {
    freeReplyObject(reply);
    return LIBREPSHEET_OK;
  } else {
    return DISCONNECTED;
  }
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
