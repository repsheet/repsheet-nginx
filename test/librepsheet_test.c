#include "hiredis/hiredis.h"
#include "../src/repsheet.h"
#include "test_suite.h"
#include "assert.h"
#include "check.h"

redisContext *context;
redisReply *reply;

void setup(void)
{
  context = get_redis_context("localhost", 6379, 0);

  if (context == NULL || context->err) {
    ck_abort_msg("Could not connect to Redis");
  }
}

void teardown(void)
{
  freeReplyObject(redisCommand(context, "flushdb"));
  if (reply) {
    freeReplyObject(reply);
    reply=NULL;
  }
  redisFree(context);
  context=NULL;
}

START_TEST(get_redis_context_failure_test)
{
  context = get_redis_context("localhost", 12345, 0);
  ck_assert(context == NULL);
}
END_TEST

START_TEST(check_connection_test)
{
  context = get_redis_context("localhost", 6379, 0);
  ck_assert_int_eq(LIBREPSHEET_OK, check_connection(context));
}
END_TEST

START_TEST(check_connection_failure_test)
{
  context = get_redis_context("localhost", 12345, 0);
  ck_assert_int_eq(DISCONNECTED, check_connection(context));
}
END_TEST

START_TEST(actor_status_test)
{
  char value[MAX_REASON_LENGTH];

  whitelist_actor(context, "1.1.1.1", IP, "IP Whitelist Actor Status");
  whitelist_actor(context, "whitelist", USER, "User Whitelist Actor Status");
  blacklist_actor(context, "1.1.1.2", IP, "IP Blacklist Actor Status");
  blacklist_actor(context, "blacklist", USER, "User Blacklist Actor Status");
  mark_actor(context, "1.1.1.3", IP, "IP Marked Actor Status");
  mark_actor(context, "marked", USER, "User Marked Actor Status");

  ck_assert_int_eq(actor_status(context, "1.1.1.1", IP, value), WHITELISTED);
  ck_assert_str_eq(value, "IP Whitelist Actor Status");
  ck_assert_int_eq(actor_status(context, "1.1.1.2", IP, value), BLACKLISTED);
  ck_assert_str_eq(value, "IP Blacklist Actor Status");
  ck_assert_int_eq(actor_status(context, "1.1.1.3", IP, value), MARKED);
  ck_assert_str_eq(value, "IP Marked Actor Status");

  ck_assert_int_eq(actor_status(context, "whitelist", USER, value), WHITELISTED);
  ck_assert_str_eq(value, "User Whitelist Actor Status");
  ck_assert_int_eq(actor_status(context, "blacklist", USER, value), BLACKLISTED);
  ck_assert_str_eq(value, "User Blacklist Actor Status");
  ck_assert_int_eq(actor_status(context, "marked", USER, value), MARKED);
  ck_assert_str_eq(value, "User Marked Actor Status");

  ck_assert_int_eq(actor_status(context, "good", UNSUPPORTED, value), UNSUPPORTED);
}
END_TEST

START_TEST(country_status_test)
{
  redisCommand(context, "SADD repsheet:countries:marked KP");
  redisCommand(context, "SADD repsheet:countries:whitelist US");
  redisCommand(context, "SADD repsheet:countries:blacklist AU");

  ck_assert_int_eq(country_status(context, "KP"), MARKED);
  ck_assert_int_eq(country_status(context, "US"), WHITELISTED);
  ck_assert_int_eq(country_status(context, "AU"), BLACKLISTED);
  ck_assert_int_eq(country_status(context, "UK"), LIBREPSHEET_OK);
}
END_TEST

Suite *make_librepsheet_connection_suite(void) {
  Suite *suite = suite_create("API Suite");

  TCase *tc_redis_connection = tcase_create("Connection API");
  tcase_add_test(tc_redis_connection, get_redis_context_failure_test);
  tcase_add_test(tc_redis_connection, check_connection_test);
  tcase_add_test(tc_redis_connection, check_connection_failure_test);
  suite_add_tcase(suite, tc_redis_connection);

  TCase *tc_connection_operations = tcase_create("Actor API");
  tcase_add_checked_fixture(tc_connection_operations, setup, teardown);
  tcase_add_test(tc_connection_operations, actor_status_test);
  tcase_add_test(tc_connection_operations, country_status_test);
  suite_add_tcase(suite, tc_connection_operations);

  return suite;
}
