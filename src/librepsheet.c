#include "librepsheet.h"

redisContext *get_redis_context(char *host, int port, int timeout)
{
  redisContext *context;

  struct timeval time = {0, (timeout > 0) ? timeout : 10000};

  context = redisConnectWithTimeout(host, port, time);
  if (context == NULL || context->err) {
    if (context) {
      printf("Error: %s\n", context->errstr);
      redisFree(context);
    } else {
      printf("Error: could not connect to Redis\n");
    }
    return NULL;
  } else {
    return context;
  }
}

void increment_rule_count(redisContext *context, char *actor, char *rule)
{
  freeReplyObject(redisCommand(context, "ZINCRBY %s:detected 1 %s", actor, rule));
}
