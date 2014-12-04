#include "hiredis/hiredis.h"
#include "../src/repsheet.h"
#include "../src/whitelist.h"
#include "check.h"

redisContext *context;
redisReply *reply;

void whitelist_setup(void)
{
  context = get_redis_context("localhost", 6379, 0);

  if (context == NULL || context->err) {
    ck_abort_msg("Could not connect to Redis");
  }
}

void whitelist_teardown(void)
{
  freeReplyObject(redisCommand(context, "flushdb"));
  if (reply) {
    freeReplyObject(reply);
    reply=NULL;
  }
  redisFree(context);
  context=NULL;
}

START_TEST(whitelist_ip_test)
{
  whitelist_actor(context, "1.1.1.1", IP, "IP Whitelist Actor Test");

  reply = redisCommand(context, "GET 1.1.1.1:repsheet:ip:whitelist");
  ck_assert_str_eq(reply->str, "IP Whitelist Actor Test");
}
END_TEST

START_TEST(whitelist_user_test)
{
  whitelist_actor(context, "repsheet", USER, "User Whitelist Actor Test");

  reply = redisCommand(context, "GET repsheet:repsheet:users:whitelist");
  ck_assert_str_eq(reply->str, "User Whitelist Actor Test");
}
END_TEST

START_TEST(whitelist_cidr_test)
{
  whitelist_actor(context, "10.0.0.0/24", BLOCK, "Users Whitelist CIDR Test");

  reply = redisCommand(context, "GET 10.0.0.0/24:repsheet:cidr:whitelist");
  ck_assert_str_eq(reply->str, "Users Whitelist CIDR Test");
}
END_TEST

START_TEST(is_ip_whitelisted_test)
{
  char value[MAX_REASON_LENGTH];

  whitelist_actor(context, "1.1.1.1", IP, "Is IP Whitelisted Test");
  int response = is_ip_whitelisted(context, "1.1.1.1", value);

  ck_assert_str_eq(value, "Is IP Whitelisted Test");
  ck_assert_int_eq(response, TRUE);
}
END_TEST

START_TEST(is_ip_whitelisted_in_cidr_test)
{
  char value[MAX_REASON_LENGTH];

  redisCommand(context, "DEL %s:repsheet:cidr:whitelist", "0.0.0.1/32");

  redisCommand(context, "SET %s:repsheet:cidr:whitelist %s", "10.0.0.0/24", "CIDR 24 Test");
  int response = is_ip_whitelisted(context, "10.0.0.15", value);
  ck_assert_int_eq(response, TRUE);
  ck_assert_str_eq(value, "CIDR 24 Test");

  redisCommand(context, "SET %s:repsheet:cidr:whitelist %s", "0.0.0.1/32", "CIDR 32 Test");
  response = is_ip_whitelisted(context, "0.0.0.1", value);
  ck_assert_int_eq(response, TRUE);
  ck_assert_str_eq(value, "CIDR 32 Test");

  // Verify a value not whitelisted does not match.
  response = is_ip_whitelisted(context, "20.1.1.1", value);
  ck_assert_int_eq(response, FALSE);
}
END_TEST

START_TEST(is_user_whitelisted_test)
{
  char value[MAX_REASON_LENGTH];

  whitelist_actor(context, "repsheet", USER, "Is User Whitelisted Test");
  int response = is_user_whitelisted(context, "repsheet", value);

  ck_assert_str_eq(value, "Is User Whitelisted Test");
  ck_assert_int_eq(response, TRUE);
}
END_TEST

START_TEST(is_country_whitelisted_true_test)
{
  redisCommand(context, "SADD repsheet:countries:whitelist AU");
  ck_assert_int_eq(is_country_whitelisted(context, "AU"), TRUE);
}
END_TEST

START_TEST(is_country_whitelisted_false_test)
{
  ck_assert_int_eq(is_country_whitelisted(context, "KP"), FALSE);
}
END_TEST

START_TEST(country_status_whitelisted_test)
{
  redisCommand(context, "SADD repsheet:countries:whitelist AU");
  ck_assert_int_eq(country_status(context, "AU"), WHITELISTED);
}
END_TEST


Suite *make_whitelist_suite(void) {
  Suite *suite = suite_create("Whitelist Suite");

  TCase *tc_ip_whitelist = tcase_create("IP list");
  tcase_add_checked_fixture(tc_ip_whitelist, whitelist_setup, whitelist_teardown);
  tcase_add_test(tc_ip_whitelist, whitelist_ip_test);
  tcase_add_test(tc_ip_whitelist, is_ip_whitelisted_test);
  suite_add_tcase(suite, tc_ip_whitelist);

  TCase *tc_user_whitelist = tcase_create("User list");
  tcase_add_checked_fixture(tc_user_whitelist, whitelist_setup, whitelist_teardown);
  tcase_add_test(tc_user_whitelist, whitelist_user_test);
  tcase_add_test(tc_user_whitelist, is_user_whitelisted_test);
  suite_add_tcase(suite, tc_user_whitelist);

  TCase *tc_cidr_whitelist = tcase_create("CIDR list");
  tcase_add_checked_fixture(tc_cidr_whitelist, whitelist_setup, whitelist_teardown);
  tcase_add_test(tc_cidr_whitelist, is_ip_whitelisted_in_cidr_test);
  tcase_add_test(tc_cidr_whitelist, whitelist_cidr_test);
  suite_add_tcase(suite, tc_cidr_whitelist);

  TCase *tc_country_whitelist = tcase_create("Country list");
  tcase_add_checked_fixture(tc_country_whitelist, whitelist_setup, whitelist_teardown);
  tcase_add_test(tc_country_whitelist, is_country_whitelisted_true_test);
  tcase_add_test(tc_country_whitelist, is_country_whitelisted_false_test);
  tcase_add_test(tc_country_whitelist, country_status_whitelisted_test);
  suite_add_tcase(suite, tc_country_whitelist);

  return suite;
}
