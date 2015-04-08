#include <string.h>
#include "repsheet.h"
#include "common.h"
#include "cidr.h"

#include "blacklist.h"

/**
 * @file blacklist.c
 * @author Aaron Bedra
 * @date 12/09/2014
 */

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
    return set_list(context, actor, "ip", "blacklisted", reason);
    break;
  case USER:
    return set_list(context, actor, "users", "blacklisted", reason);
    break;
  case BLOCK:
    return set_list(context, actor, "cidr", "blacklisted", reason);
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
  redisReply *ip = redisCommand(context, "GET %s:repsheet:ip:blacklisted", actor);
  if (ip) {
    if (ip->type == REDIS_REPLY_STRING) {
      populate_reason(ip, reason);
      freeReplyObject(ip);
      return TRUE;
    } else {
      freeReplyObject(ip);
    }
  } else {
    return DISCONNECTED;
  }

  redisReply *blacklisted = redisCommand(context, "KEYS *:repsheet:cidr:blacklisted");
  if (blacklisted) {
    if (blacklisted->type == REDIS_REPLY_ARRAY) {
      int i;
      redisReply *value;
      char *block;
      for(i = 0; i < blacklisted->elements; i++) {
        block = strtok(blacklisted->element[i]->str, ":");
        if (cidr_contains(block, actor) > 0) {
          value = redisCommand(context, "GET %s:repsheet:cidr:blacklisted", block);
          if (value) {
            populate_reason(value, reason);
            freeReplyObject(value);
          }
          freeReplyObject(blacklisted);
          return TRUE;
        }
      }
    } else{
      freeReplyObject(blacklisted);
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

  reply = redisCommand(context, "GET %s:repsheet:users:blacklisted", actor);
  if (reply) {
    if (reply->type == REDIS_REPLY_STRING) {
      populate_reason(reply, reason);
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
  reply = redisCommand(context, "SISMEMBER repsheet:countries:blacklisted %s", country_code);
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

/**
 * Checks to see if an actor has been previously blacklisted
 *
 * @param context the Redis connection
 * @param type IP or USER
 * @param actor the addres of the actor in question
 *
 * @returns an integer response
 */

int is_historical_offender(redisContext *context, int type, const char *actor)
{
  redisReply *reply;

  switch(type) {
  case IP:
    reply = redisCommand(context, "SISMEMBER repsheet:ip:blacklisted:history %s", actor);
    break;
  case USER:
    reply = redisCommand(context, "SISMEMBER repsheet:user:blacklisted:history %s", actor);
    break;
  default:
    return UNSUPPORTED;
  }

  if (reply) {
    if (reply->integer == 1) {
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
