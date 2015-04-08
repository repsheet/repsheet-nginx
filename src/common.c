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

/**
 * Sets the expiry for a record
 *
 * @param context the Redis connection
 * @param actor the IP address of the actor
 * @param label the label associated with the actor
 * @param expiry the length until the record expires
 *
 * @returns an integer response
 */
int expire(redisContext *context, const char *actor, char *label, int expiry)
{
  redisReply *reply;

  reply = redisCommand(context, "EXPIRE %s:%s %d", actor, label, expiry);
  if (reply) {
    freeReplyObject(reply);
    return LIBREPSHEET_OK;
  } else {
    return DISCONNECTED;
  }
}

/**
 * Blacklists and sets the expiry for a record. Adds the actor to the
 * blacklist history and sets the reason code.
 *
 * @param context the Redis connection
 * @param type IP or USER
 * @param actor the IP address of the actor
 * @param expiry the length until the record expires
 * @param reason the reason for blacklisting the actor
 *
 * @returns an integer response
 */
int blacklist_and_expire(redisContext *context, int type, const char *actor, int expiry, char *reason)
{
  redisReply *reply;

  redisCommand(context, "MULTI");
  switch(type) {
  case IP:
    redisCommand(context, "SETEX %s:repsheet:ip:blacklisted %d %s", actor, expiry, reason);
    redisCommand(context, "SADD repsheet:ip:blacklisted:history %s", actor);
    break;
  case USER:
    redisCommand(context, "SETEX %s:repsheet:users:blacklisted %d %s", actor, expiry, reason);
    redisCommand(context, "SADD repsheet:users:blacklisted:history %s", actor);
    break;
  }
  reply = redisCommand(context, "EXEC");

  if (reply) {
    freeReplyObject(reply);
    return LIBREPSHEET_OK;
  } else {
    return DISCONNECTED;
  }
}

/**
 * Increments the number of times a rule has been triggered for an actor
 *
 * @param context the Redis connection
 * @param actor the IP address of the actor
 * @param rule the ModSecurity rule number
 *
 * @returns an integer response
 */
int increment_rule_count(redisContext *context, const char *actor, char *rule)
{
  redisReply *reply;

  reply = redisCommand(context, "ZINCRBY %s:detected 1 %s", actor, rule);
  if (reply) {
    freeReplyObject(reply);
    return LIBREPSHEET_OK;
  } else {
    return DISCONNECTED;
  }
}
