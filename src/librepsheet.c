#include <stdlib.h>
#include <string.h>
#include <pcre.h>

#include "hiredis/hiredis.h"

#include "config.h"
#include "repsheet.h"

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
 * - autoconf, automake, libtool, libpcre, hiredis, check (optional for tests)
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
 * This function establishes a connection to Redis
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
  }

  return UNSUPPORTED;
}

/**
 * Top level API for determining the status of a country
 *
 * @param context the Redis connection
 * @param country_code the 2 digit ISO country code
 *
 * @returns an integer response
 */
int country_status(redisContext *context, const char *country_code)
{
  int response;

#if WHITELIST_ENABLED
  response = is_country_whitelisted(context, country_code);
  if (response == DISCONNECTED) { return DISCONNECTED; }
  if (response == TRUE)         { return WHITELISTED; }
#endif

#if BLACKLIST_ENABLED
  response = is_country_blacklisted(context, country_code);
  if (response == DISCONNECTED) { return DISCONNECTED; }
  if (response == TRUE)         { return BLACKLISTED; }
#endif

#if MARKED_ENABLED
  response = is_country_marked(context, country_code);
  if (response == DISCONNECTED) { return DISCONNECTED; }
  if (response == TRUE)         { return MARKED; }
#endif

  return LIBREPSHEET_OK;
}

/**
 * Determines the IP address of the actor. If X-Forwarded-For has
 * information, the function takes the first address in the string. If
 * no XFF information is provided the function returns the
 * connected_address.
 *
 * @param connected_address The IP of the connection to the server
 * @param xff_header The contents of the X-Forwarded-For header
 */
const char *remote_address(char *connected_address, const char *xff_header)
{
  if (xff_header == NULL) {
    return connected_address;
  }

  int error_offset, subvector[30];
  const char *error, *address;

  pcre *re_compiled;

  char *regex = "\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b";

  re_compiled = pcre_compile(regex, 0, &error, &error_offset, NULL);

  int matches = pcre_exec(re_compiled, 0, xff_header, strlen(xff_header), 0, 0, subvector, 30);

  if(matches < 0) {
    return NULL;
  } else {
    pcre_get_substring(xff_header, subvector, matches, 0, &(address));
  }

  pcre_free(re_compiled);

  return address;
}
