#include <stdlib.h>
#include <string.h>

#include "hiredis/hiredis.h"

#include "config.h"
#include "repsheet.h"
#include "whitelist.h"
#include "blacklist.h"
#include "marked.h"

/**
 * @file librepsheet.c
 * @author Aaron Bedra
 * @date 5/19/2014
 */

/*! \mainpage librepsheet
 *
 * \section intro_sec Introduction
 *
 * librepsheet is the core logic library for Repsheet. It is used in the Repsheet webserver modules as well as the backend.
 *
 * \section install_sec Installation
 *
 * librepsheet can be installed via your package manager of choice or
 * compiled from source. There are packages available for RedHat and
 * Debian based systems.
 *
 * \subsection step1 From source
 *
 * To build librepsheet you will need the following dependencies:
 *
 * - autoconf, automake, libtool, hiredis, check (optional for tests)
 *
 * The library can be built and installed using the following commands:
 *
 * <code>
 * $ ./autogen.sh <br />
 * $ ./configure <br />
 * $ make <br />
 * $ make check # optional <br />
 * $ sudo make install
 * </code>
 *
 * You can verify that the install succeeded by checking package config:
 *
 * <code>
 * $ pkg-config --list-all | grep repsheet
 * repsheet                  repsheet - The Repsheet core logic library
 * </code>
 */

/**
 * This function establishes a connection to Redis with a connect time out
 *
 * @param host the hostname of the Redis server
 * @param port the port number of the Redis server
 * @param timeout the length in milliseconds before the connection times out
 *
 * @returns a Redis connection
 */
redisContext *get_redis_context(const char *host, int port, int timeout)
{
  redisContext *context;

  struct timeval time = {0, (timeout > 0) ? (timeout * 1000) : 10000};

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
 * This function establishes a connection to Redis with a connect and request time out
 *
 * @param host the hostname of the Redis server
 * @param port the port number of the Redis server
 * @param connect_timeout the length in milliseconds before the connection times out
 * @param read_timeout the length in milliseconds before a request times out
 *
 * @returns a Redis connection
 */
redisContext *repsheet_connect(const char *host, int port, int connect_timeout, int read_timeout) {
  redisContext *context;

  struct timeval ct = {0, (connect_timeout > 0) ? (connect_timeout * 1000) : 10000};
  struct timeval rt = {0, (read_timeout > 0) ? (read_timeout * 1000) : 10000};

  context = redisConnectWithTimeout(host, port, ct);
  if (context == NULL || context->err) {
    if (context) {
      printf("Error: %s\n", context->errstr);
      redisFree(context);
    } else {
      printf("Error: could not connect to Redis\n");
    }
    return NULL;
  } else {
    redisSetTimeout(context, rt);
    return context;
  }
}

/**
 * Tests the connection to ensure it is working properly
 *
 * @param context the Redis connection
 *
 * @returns an integer response with the connection status
 */
int check_connection(redisContext *context)
{
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
      return LIBREPSHEET_OK;
    }
  } else {
    return DISCONNECTED;
  }
}

/**
 * Tests the connection to ensure it is working properly
 *
 * @param context the Redis connection
 *
 * @returns an integer response with the connection status
 */
int repsheet_reconnect(redisContext *context)
{
  if (context == NULL) {
    return DISCONNECTED;
  }

  if (context && context->err == 0) {
    return LIBREPSHEET_OK;
  }

  redisReconnect(context);

  if (context->err == 0) {
    return LIBREPSHEET_OK;
  } else {
    return DISCONNECTED;
  }
}

/**
 * Top level API for determining the status of an actor
 *
 * @param context the Redis connection
 * @param actor the IP address or user value of the actor
 * @param type IP or USER
 * @param reason return the reason if whitelisted, blacklisted, or marked.
 *
 * @returns an integer status
 */
int actor_status(redisContext *context, const char *actor, int type, char *reason)
{
  int response;

  reason[0] = '\0';

  switch(type) {
  case IP:
#if WHITELIST_ENABLED
    response = is_ip_whitelisted(context, actor, reason);
    if (response == DISCONNECTED) { return DISCONNECTED; }
    if (response == TRUE)         { return WHITELISTED; }
#endif

#if BLACKLIST_ENABLED
    response = is_ip_blacklisted(context, actor, reason);
    if (response == DISCONNECTED) { return DISCONNECTED; }
    if (response == TRUE)         { return BLACKLISTED; }
#endif

#if MARKED_ENABLED
    response = is_ip_marked(context, actor, reason);
    if (response == DISCONNECTED) { return DISCONNECTED; }
    if (response == TRUE)         { return MARKED; }
#endif

    break;
  case USER:
#if WHITELIST_ENABLED
    response = is_user_whitelisted(context, actor, reason);
    if (response == DISCONNECTED) { return DISCONNECTED; }
    if (response == TRUE)         { return WHITELISTED; }
#endif

#if BLACKLIST_ENABLED
    response = is_user_blacklisted(context, actor, reason);
    if (response == DISCONNECTED) { return DISCONNECTED; }
    if (response == TRUE)         { return BLACKLISTED; }
#endif

#if MARKED_ENABLED
    response = is_user_marked(context, actor, reason);
    if (response == DISCONNECTED) { return DISCONNECTED; }
    if (response == TRUE)         { return MARKED; }
#endif

    break;
  default:
    return UNSUPPORTED;
    break;
  }

  return LIBREPSHEET_OK;
}
