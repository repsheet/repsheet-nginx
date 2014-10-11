#include <string.h>
#include "repsheet.h"
#include "common.h"
#include "cidr.h"

#include "blacklist.h"

/**
 * Adds the actor to the Repsheet blacklist
 *
 * @param context the Redis connection
 * @param actor the actors user data or ip address
 * @param type IP or USER
 * @reason string to describe the reason for blacklisting
 *
 * @returns an integer result
 */
int blacklist_actor(redisContext *context, const char *actor, int type, const char *reason)
{
  redisReply *reply;

  switch(type) {
  case IP:
    return set_list(context, actor, "ip", "blacklist", reason);
    break;
  case USER:
    return set_list(context, actor, "users", "blacklist", reason);
    break;
  default:
    return UNSUPPORTED;
    break;
  }
}

/**
 * Checks to see if an ip is on the Repsheet blacklist
 *
 * @param context the Redis connection
 * @param actor the IP address of the actor
 * @param reason returns the reason for blacklisting.
 *
 * @returns TRUE if yes, FALSE if no, DISCONNECTED if error
 */
int is_ip_blacklisted(redisContext *context, const char *actor, char *reason)
{
  redisReply *ip = redisCommand(context, "GET %s:repsheet:ip:blacklist", actor);
  if (ip) {
    if (ip->type == REDIS_REPLY_STRING) {
      _populate_reason(ip, reason);
      freeReplyObject(ip);
      return TRUE;
    } else {
      freeReplyObject(ip);
    }
  } else {
    return DISCONNECTED;
  }

  redisReply *blacklist = redisCommand(context, "KEYS *:repsheet:cidr:blacklist");
  if (blacklist) {
    if (blacklist->type == REDIS_REPLY_ARRAY) {
      int i;
      redisReply *value;
      char *block;
      for(i = 0; i < blacklist->elements; i++) {
        block = strtok(blacklist->element[i]->str, ":");
        if (cidr_contains(block, actor)) {
          value = redisCommand(context, "GET %s:repsheet:cidr:blacklist", block);
          if (value) {
            _populate_reason(value, reason);
            freeReplyObject(value);
          }
          freeReplyObject(blacklist);
          return TRUE;
        }
      }
    } else{
      freeReplyObject(blacklist);
    }
  } else {
    return DISCONNECTED;
  }

  return FALSE;
}

/**
 * Checks to see if a user is on the Repsheet blacklist
 *
 * @param context the Redis connection
 * @param actor the user
 * @param reason returns the reason for blacklisting.
 *
 * @returns TRUE if yes, FALSE if no, DISCONNECTED if error
 */
int is_user_blacklisted(redisContext *context, const char *actor, char *reason)
{
  redisReply *reply;

  reply = redisCommand(context, "GET %s:repsheet:users:blacklist", actor);
  if (reply) {
    if (reply->type == REDIS_REPLY_STRING) {
      _populate_reason(reply, reason);
      freeReplyObject(reply);
      return TRUE;
    } else {
      freeReplyObject(reply);
      return FALSE;
    }
  } else {
    return DISCONNECTED;
  }
}

/**
 * Looks up the country to see if it is blacklisted.
 *
 * @param context the Redis connection
 * @param country_code the 2 digit ISO country code
 *
 * @returns an integer response
 */
int is_country_blacklisted(redisContext *context, const char *country_code)
{
  redisReply *reply;
  reply = redisCommand(context, "SISMEMBER repsheet:countries:blacklist %s", country_code);
  if (reply) {
    if (reply->type == REDIS_REPLY_INTEGER) {
      freeReplyObject(reply);
      return reply->integer;
    } else {
      freeReplyObject(reply);
      return NIL;
    }
  } else {
    return DISCONNECTED;
  }
}
