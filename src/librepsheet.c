/**
 * @file librepsheet.c
 * @author Aaron Bedra
 * @date 4/12/2014
 */

/*! \mainpage librepsheet
 *
 * \section intro_sec Introduction
 *
 * librepsheet is the core logic library for Repsheet. It is used in the Repsheet webserver modules as well as the backend.
 *
 * \section install_sec Installation
 *
 * librepsheet can be installed via your package manager of choice or compiled from source
 *
 * \subsection step1 From source
 *
 * TODO: write this
 */

#include "librepsheet.h"

/**
 * This function establishes a connection to Redis
 *
 * @param host the hostname of the Redis server
 * @param port the port number of the Redis server
 * @param timeout the length in milliseconds before the connection times out
 *
 * @returns a Redis connection
 */
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

/**
 * Increments the number of times a rule has been triggered for an actor
 *
 * @param context the Redis connection
 * @param actor the IP address of the actor
 * @param rule the ModSecurity rule number
 */
void increment_rule_count(redisContext *context, char *actor, char *rule)
{
  freeReplyObject(redisCommand(context, "ZINCRBY %s:detected 1 %s", actor, rule));
}

/**
 * Adds the actor to the Repsheet
 *
 * @param context the Redis connection
 * @param actor the IP address of the actor
 */
void mark_actor(redisContext *context, char *actor)
{
  freeReplyObject(redisCommand(context, "SET %s:repsheet true", actor));
}

/**
 * Sets the expiry for a record
 *
 * @param context the Redis connection
 * @param actor the IP address of the actor
 * @param label the label associated with the actor
 * @param expiry the length until the record expires
 */
void expire(redisContext *context, char *actor, char *label, int expiry)
{
  freeReplyObject(redisCommand(context, "EXPIRE %s:%s %d", actor, label, expiry));
}

