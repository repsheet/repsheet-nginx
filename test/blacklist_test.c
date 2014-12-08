#include "hiredis/hiredis.h"
#include "../src/repsheet.h"
#include "../src/blacklist.h"
#include "check.h"

redisContext *context;
redisReply *reply;

void blacklist_setup(void)
{
  context = get_redis_context("localhost", 6379, 0);

  if (context == NULL || context->err) {
    ck_abort_msg("Could not connect to Redis");
  }
}

void blacklist_teardown(void)
{
  freeReplyObject(redisCommand(context, "flushdb"));
  if (reply) {
    freeReplyObject(reply);
    reply=NULL;
  }
  redisFree(context);
  context=NULL;
}

START_TEST(blacklist_ip_test)
{
  blacklist_actor(context, "1.1.1.1", IP, "IP Blacklist Actor Test");

  reply = redisCommand(context, "GET 1.1.1.1:repsheet:ip:blacklist");
  ck_assert_str_eq(reply->str, "IP Blacklist Actor Test");
}
END_TEST

START_TEST(blacklist_user_test)
{
  blacklist_actor(context, "repsheet", USER, "Users Blacklist Actor Test");

  reply = redisCommand(context, "GET repsheet:repsheet:users:blacklist");
  ck_assert_str_eq(reply->str, "Users Blacklist Actor Test");
}
END_TEST

START_TEST(blacklist_cidr_test)
{
  blacklist_actor(context, "10.0.0.0/24", BLOCK, "Users Blacklist CIDR Test");

  reply = redisCommand(context, "GET 10.0.0.0/24:repsheet:cidr:blacklist");
  ck_assert_str_eq(reply->str, "Users Blacklist CIDR Test");
}
END_TEST

START_TEST(is_ip_blacklisted_test)
{
  char value[MAX_REASON_LENGTH];

  blacklist_actor(context, "1.1.1.1", IP, "Is IP Blacklisted Test");
  int response = is_ip_blacklisted(context, "1.1.1.1", value);
  ck_assert_str_eq(value, "Is IP Blacklisted Test");

  ck_assert_int_eq(response, TRUE);
}
END_TEST

START_TEST(blacklist_reason_ip_found_test)
{
  char value[MAX_REASON_LENGTH];
  int response;

  blacklist_and_expire(context, IP, "1.1.1.1", 200, "Blacklist Reason IP Found Test");
  response = is_ip_blacklisted(context, "1.1.1.1", value);
  ck_assert_int_eq(response, TRUE);
  ck_assert_str_eq(value, "Blacklist Reason IP Found Test");
}
END_TEST

START_TEST(blacklist_reason_ip_not_found_test)
{
  char value[MAX_REASON_LENGTH];
  int response;

  blacklist_and_expire(context, IP, "1.1.1.1", 200, "Blacklist Reason IP Not Found Test");
  response = is_ip_blacklisted(context, "1.1.1.2", value);
  ck_assert_int_eq(response, FALSE);
}
END_TEST

START_TEST(is_ip_blacklisted_in_cidr_test)
{
  char value[MAX_REASON_LENGTH];

  redisCommand(context, "DEL %s:repsheet:cidr:blacklist", "0.0.0.1/32");

  redisCommand(context, "SET %s:repsheet:cidr:blacklist %s", "10.0.0.0/24", "CIDR 24 Test");
  int response = is_ip_blacklisted(context, "10.0.0.15", value);
  ck_assert_int_eq(response, TRUE);
  ck_assert_str_eq(value, "CIDR 24 Test");

  redisCommand(context, "SET %s:repsheet:cidr:blacklist %s", "0.0.0.1/32", "CIDR 32 Test");
  response = is_ip_blacklisted(context, "0.0.0.1", value);
  ck_assert_int_eq(response, TRUE);
  ck_assert_str_eq(value, "CIDR 32 Test");

  // Verify a value not blacklisted does not match.
  response = is_ip_blacklisted(context, "20.1.1.1", value);
  ck_assert_int_eq(response, FALSE);
}
END_TEST

START_TEST(is_user_blacklisted_test)
{
  char value[MAX_REASON_LENGTH];

  blacklist_actor(context, "repsheet", USER, "Is User Blacklisted Test");
  int response = is_user_blacklisted(context, "repsheet", value);

  ck_assert_str_eq(value, "Is User Blacklisted Test");
  ck_assert_int_eq(response, TRUE);
}
END_TEST

START_TEST(is_historical_offender_ip_test)
{
  redisCommand(context, "SADD repsheet:ip:blacklist:history 1.1.1.1");
  int response = is_historical_offender(context, IP, "1.1.1.1");

  ck_assert_int_eq(response, TRUE);
}
END_TEST

START_TEST(is_not_historical_offender_ip_test)
{
  redisCommand(context, "SADD repsheet:ip:blacklist:history 1.1.1.2");
  int response = is_historical_offender(context, IP, "1.1.1.1");

  ck_assert_int_eq(response, FALSE);
}
END_TEST

START_TEST(is_historical_offender_user_test)
{
  redisCommand(context, "SADD repsheet:user:blacklist:history test");
  int response = is_historical_offender(context, USER, "test");

  ck_assert_int_eq(response, TRUE);
}
END_TEST

START_TEST(is_not_historical_offender_user_test)
{
  int response = is_historical_offender(context, USER, "test");

  ck_assert_int_eq(response, FALSE);
}
END_TEST

START_TEST(is_country_blacklisted_false_test)
{
  ck_assert_int_eq(is_country_blacklisted(context, "KP"), FALSE);
}
END_TEST

START_TEST(is_country_blacklisted_true_test)
{
  redisCommand(context, "SADD repsheet:countries:blacklist KP");
  ck_assert_int_eq(is_country_blacklisted(context, "KP"), TRUE);
}
END_TEST

START_TEST(country_status_blacklisted_test)
{
  redisCommand(context, "SADD repsheet:countries:blacklist KP");
  ck_assert_int_eq(country_status(context, "KP"), BLACKLISTED);
}
END_TEST

Suite *make_blacklist_suite(void) {
  Suite *suite = suite_create("Blacklist Suite");

  TCase *tc_ip_blacklist = tcase_create("IP list");
  tcase_add_checked_fixture(tc_ip_blacklist, blacklist_setup, blacklist_teardown);
  tcase_add_test(tc_ip_blacklist, blacklist_ip_test);
  tcase_add_test(tc_ip_blacklist, is_ip_blacklisted_test);
  tcase_add_test(tc_ip_blacklist, blacklist_reason_ip_found_test);
  tcase_add_test(tc_ip_blacklist, blacklist_reason_ip_not_found_test);
  suite_add_tcase(suite, tc_ip_blacklist);

  TCase *tc_user_blacklist = tcase_create("User list");
  tcase_add_checked_fixture(tc_user_blacklist, blacklist_setup, blacklist_teardown);
  tcase_add_test(tc_user_blacklist, blacklist_user_test);
  tcase_add_test(tc_user_blacklist, is_user_blacklisted_test);
  suite_add_tcase(suite, tc_user_blacklist);

  TCase *tc_cidr_blacklist = tcase_create("CIDR list");
  tcase_add_checked_fixture(tc_cidr_blacklist, blacklist_setup, blacklist_teardown);
  tcase_add_test(tc_cidr_blacklist, blacklist_cidr_test);
  tcase_add_test(tc_cidr_blacklist, is_ip_blacklisted_in_cidr_test);
  suite_add_tcase(suite, tc_cidr_blacklist);

  TCase *tc_country_blacklist = tcase_create("Country list");
  tcase_add_checked_fixture(tc_country_blacklist, blacklist_setup, blacklist_teardown);
  tcase_add_test(tc_country_blacklist, is_country_blacklisted_true_test);
  tcase_add_test(tc_country_blacklist, is_country_blacklisted_false_test);
  tcase_add_test(tc_country_blacklist, country_status_blacklisted_test);
  suite_add_tcase(suite, tc_country_blacklist);

  TCase *tc_historical = tcase_create("Historical");
  tcase_add_checked_fixture(tc_historical, blacklist_setup, blacklist_teardown);
  tcase_add_test(tc_historical, is_historical_offender_ip_test);
  tcase_add_test(tc_historical, is_not_historical_offender_ip_test);
  tcase_add_test(tc_historical, is_historical_offender_user_test);
  tcase_add_test(tc_historical, is_not_historical_offender_user_test);
  suite_add_tcase(suite, tc_historical);

  return suite;
}
