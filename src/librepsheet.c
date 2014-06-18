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
 * $ ./autogen.sh
 * $ ./configure
 * $ make
 * $ make check # optional
 * $ sudo make install
 *
 * You can verify that the install succeeded by checking package config:
 *
 * $ pkg-config --list-all | grep repsheet
 * repsheet                  repsheet - The Repsheet core logic library
 */

#include "repsheet.h"

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
 * Adds the actor to the Repsheet
 *
 * @param context the Redis connection
 * @param actor the IP address of the actor
 */
void mark_actor(redisContext *context, const char *actor)
{
  freeReplyObject(redisCommand(context, "SET %s:repsheet true", actor));
}

/**
 * Adds the actor to the Repsheet blacklist
 *
 * @param context the Redis connection
 * @param actor the IP address of the actor
 */
void blacklist_actor(redisContext *context, const char *actor)
{
  freeReplyObject(redisCommand(context, "SET %s:repsheet:blacklist true", actor));
}

/**
 * Adds the actor to the Repsheet whitelist
 *
 * @param context the Redis connection
 * @param actor the IP address of the actor
 */
void whitelist_actor(redisContext *context, const char *actor)
{
  freeReplyObject(redisCommand(context, "SET %s:repsheet:whitelist true", actor));
}

/**
 * Checks to see if an actor is on the Repsheet
 *
 * @param context the Redis connection
 * @param actor the IP address of the actor
 *
 * @returns TRUE if yes, FALSE if no
 */
int is_on_repsheet(redisContext *context, const char *actor)
{
  redisReply *reply;

  reply = redisCommand(context, "GET %s:repsheet", actor);
  if (reply->str && strcmp(reply->str, "true") == 0) {
    freeReplyObject(reply);
    return TRUE;
  }

  return FALSE;
}

/**
 * Checks to see if an actor is on the Repsheet blacklist
 *
 * @param context the Redis connection
 * @param actor the IP address of the actor
 *
 * @returns TRUE if yes, FALSE if no
 */
int is_blacklisted(redisContext *context, const char *actor)
{
  redisReply *reply;

  reply = redisCommand(context, "GET %s:repsheet:blacklist", actor);
  if (reply->str && strcmp(reply->str, "true") == 0) {
    freeReplyObject(reply);
    return TRUE;
  }

  return FALSE;
}

/**
 * Checks to see if an actor is on the Repsheet whitelist
 *
 * @param context the Redis connection
 * @param actor the IP address of the actor
 *
 * @returns TRUE if yes, FALSE if no
 */
int is_whitelisted(redisContext *context, const char *actor)
{
  redisReply *reply;

  reply = redisCommand(context, "GET %s:repsheet:whitelist", actor);
  if (reply->str && strcmp(reply->str, "true") == 0) {
    freeReplyObject(reply);
    return TRUE;
  }

  return FALSE;
}

/**
 * Checks to see if an actor has been previously blacklisted
 *
 * @param context the Redis connection
 * @param actor the addres of the actor in question
 */

int is_historical_offender(redisContext *context, const char *actor)
{
  redisReply *reply;

  reply = redisCommand(context, "SISMEMBER repsheet:blacklist:history %s", actor);
  if (reply) {
    if (reply->integer == 1) {
      freeReplyObject(reply);
      return 1;
    } else {
      freeReplyObject(reply);
      return 0;
    }
  }

  return 0;
}

/**
 * Sets the expiry for a record
 *
 * @param context the Redis connection
 * @param actor the IP address of the actor
 * @param label the label associated with the actor
 * @param expiry the length until the record expires
 */
void expire(redisContext *context, const char *actor, char *label, int expiry)
{
  freeReplyObject(redisCommand(context, "EXPIRE %s:%s %d", actor, label, expiry));
}

/**
 * Blacklists and sets the expiry for a record. Adds the actor to the
 * blacklist history and sets the reason code.
 *
 * @param context the Redis connection
 * @param actor the IP address of the actor
 * @param expiry the length until the record expires
 * @param reason the reason for blacklisting the actor
 */
void blacklist_and_expire(redisContext *context, const char *actor, int expiry, char *reason)
{
  freeReplyObject(redisCommand(context, "SETEX %s:repsheet:blacklist %d true", actor, expiry));
  freeReplyObject(redisCommand(context, "SETEX %s:repsheet:blacklist:reason %d %s", actor, expiry, reason));
  freeReplyObject(redisCommand(context, "SADD repsheet:blacklist:history %s", actor));
}

/**
 * Records details about the request and stores them in Redis
 *
 * @param context the redis connection
 * @param timestamp the request timestamp
 * @param user_agent the requets user agent string
 * @param http_method the http method
 * @param uri the endpoint for the request
 * @param arguments the query string
 * @param redis_max_length the maximum number of requests to keep in Redis
 * @param redis_expiry the time to live for the entry
 * @param actor the ip address of the actor
 */
void record(redisContext *context, char *timestamp, const char *user_agent,
            const char *http_method, char *uri, char *arguments, int redis_max_length,
            int redis_expiry, const char *actor)
{
  char *t, *ua, *method, *u, *args, *rec;

  //TODO: should probably use asprintf here to save a bunch of
  //nonsense calls. Make sure there is a sane way to do this across
  //platforms.

  if (timestamp == NULL) {
    t = malloc(2);
    sprintf(t, "%s", "-");
  } else {
    t = malloc(strlen(timestamp) + 1);
    sprintf(t, "%s", timestamp);
  }

  if (user_agent == NULL) {
    ua = malloc(2);
    sprintf(ua, "%s", "-");
  } else {
    ua = malloc(strlen(user_agent) + 1);
    sprintf(ua, "%s", user_agent);
  }

  if (http_method == NULL) {
    method = malloc(2);
    sprintf(method, "%s", "-");
  } else {
    method = malloc(strlen(http_method) + 1);
    sprintf(method, "%s", http_method);
  }

  if (uri == NULL) {
    u = malloc(2);
    sprintf(u, "%s", "-");
  } else {
    u = malloc(strlen(uri) + 1);
    sprintf(u, "%s", uri);
  }

  if (arguments == NULL) {
    args = malloc(2);
    sprintf(args, "%s", "-");
  } else {
    args = malloc(strlen(arguments) + 1);
    sprintf(args, "%s", arguments);
  }

  rec = (char*)malloc(strlen(t) + strlen(ua) + strlen(method) + strlen(u) + strlen(args) + 9);
  sprintf(rec, "%s, %s, %s, %s, %s", t, ua, method, u, args);

  free(t);
  free(ua);
  free(method);
  free(u);
  free(args);

  freeReplyObject(redisCommand(context, "LPUSH %s:requests %s", actor, rec));
  freeReplyObject(redisCommand(context, "LTRIM %s:requests 0 %d", actor, (redis_max_length - 1)));
  if (redis_expiry > 0) {
    freeReplyObject(redisCommand(context, "EXPIRE %s:requests %d", actor, redis_expiry));
  }

  free(rec);
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

/**
 * Finds the number of ModSecurity rules in X-WAF-Events header. This
 * is used before the actual processor so that the events array can be
 * properly allocated.
 *
 * @param waf_events the contents of the X-WAF-Events header
 */

int matches(const char *waf_events)
{
  int match, error_offset;
  int offset = 0;
  int matches = 0;
  int ovector[100];

  const char *error;

  pcre *regex = pcre_compile("(?<!\\d)\\d{6}(?!\\d)", PCRE_MULTILINE, &error, &error_offset, 0);

  while (offset < strlen(waf_events) && (match = pcre_exec(regex, 0, waf_events, strlen(waf_events), offset, 0, ovector, sizeof(ovector))) >= 0) {
    matches++;
    offset = ovector[1];
  }

  return matches;
}

/**
 * Populates the events array with ModSecurity rule ids that were
 * present in the X-WAF-Events header
 *
 * @param waf_events the contents of the X-WAF-Events header
 * @param events the pre-allocated events array to place results
 */

void process_mod_security_headers(const char *waf_events, char *events[])
{
  int i = 0;
  int matches = 0;
  int offset = 0;
  int count = 0;
  int match, error_offset;
  int ovector[100];

  char *prev_event = NULL;

  const char *event;
  const char *error;

  pcre *regex;

  regex = pcre_compile("(?<!\\d)\\d{6}(?!\\d)", PCRE_MULTILINE, &error, &error_offset, 0);

  while (offset < strlen(waf_events) && (match = pcre_exec(regex, 0, waf_events, strlen(waf_events), offset, 0, ovector, sizeof(ovector))) >= 0) {
    for(i = 0; i < match; i++) {
      pcre_get_substring(waf_events, ovector, match, i, &(event));
      if (event != prev_event) {
        strcpy(events[count], event);
        prev_event = (char*)event;
      }
    }
    count++;
    offset = ovector[1];
  }
}

/**
 * Increments the number of times a rule has been triggered for an actor
 *
 * @param context the Redis connection
 * @param actor the IP address of the actor
 * @param rule the ModSecurity rule number
 */
void increment_rule_count(redisContext *context, const char *actor, char *rule)
{
  freeReplyObject(redisCommand(context, "ZINCRBY %s:detected 1 %s", actor, rule));
}

/**
 * Processes the X-WAF-Score header to determine the total ModSecurity
 * anomaly score
 *
 * @param waf_score the X-WAF-Score header
 */

int modsecurity_total(const char *waf_score)
{
  int offset = 0;
  int match, error_offset;
  int ovector[100];

  const char *event;
  const char *error;

  pcre *regex;

  regex = pcre_compile("\\d+;", PCRE_MULTILINE, &error, &error_offset, 0);

  match = pcre_exec(regex, 0, waf_score, strlen(waf_score), offset, 0, ovector, sizeof(ovector));

  if (match && match > 0) {
    pcre_get_substring(waf_score, ovector, match, 0, &(event));
    return strtol(event, 0, 10);
  }

  return 0;
}

/**
 * Processes a request content blocking rule. Returns a constructed
 * repsheet_rule_t. If any errors exist on the rule it will populate
 * the error member of the struct.
 *
 * @param rule_string the string representation of the rule
 */

repsheet_rule_t process_rule(char *rule_string)
{
  mpc_parser_t *Field = mpc_new("field");
  mpc_parser_t *Separator = mpc_new("sep");
  mpc_parser_t *Rule = mpc_new("rule");

  mpca_lang(MPCA_LANG_DEFAULT,
    " field : /[a-zA-Z]+/;    "
    " sep : ':' ;             "
    " rule : /^/ <field><sep><field><sep><field> /$/; ",
    Field, Separator, Rule, NULL);

  mpc_result_t r;
  repsheet_rule_t rule;

  if (mpc_parse("input", rule_string, Rule, &r)) {
    mpc_ast_t *nodes = r.output;
    rule.part = nodes->children[1]->contents;
    rule.location = nodes->children[3]->contents;
    rule.field = nodes->children[5]->contents;
    rule.error = NULL;
  } else {
    rule.part = NULL;
    rule.location = NULL;
    rule.field = NULL;
    rule.error = mpc_err_string(r.error);
  }

  // TODO: in the future this will get much more advanced. Right now
  // the only supported part and location is header:cookie
  if (rule.part && !strcasecmp(rule.part, "header") == 0) {
    rule.error = "Unsupported request part\n";
  }

  if (rule.location && !strcasecmp(rule.location, "cookie") == 0) {
    rule.error = "Unsupported request location\n";
  }

  return rule;
}
