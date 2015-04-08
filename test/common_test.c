#include "hiredis/hiredis.h"
#include "../src/repsheet.h"
#include "../src/common.h"
#include "check.h"

redisContext *context;
redisReply *reply;

void common_setup(void)
{
  context = get_redis_context("localhost", 6379, 0);

  if (context == NULL || context->err) {
    ck_abort_msg("Could not connect to Redis");
  }
}

void common_teardown(void)
{
  freeReplyObject(redisCommand(context, "flushdb"));
  if (reply) {
    freeReplyObject(reply);
    reply=NULL;
  }
  redisFree(context);
  context=NULL;
}

START_TEST(expire_test)
{
  mark_actor(context, "1.1.1.1", IP, "Expire Test");
  expire(context, "1.1.1.1", "repsheet:ip:marked", 200);
  reply = redisCommand(context, "TTL 1.1.1.1:repsheet:ip:marked");

  ck_assert_int_eq(reply->integer, 200);
}
END_TEST

START_TEST(blacklist_and_expire_ip_test)
{
  blacklist_and_expire(context, IP, "1.1.1.1", 200, "IP Blacklist And Expire Test");

  reply = redisCommand(context, "TTL 1.1.1.1:repsheet:ip:blacklisted");
  ck_assert_int_eq(reply->integer, 200);

  reply = redisCommand(context, "GET 1.1.1.1:repsheet:ip:blacklisted");
  ck_assert_str_eq(reply->str, "IP Blacklist And Expire Test");

  reply = redisCommand(context, "SISMEMBER repsheet:ip:blacklisted:history 1.1.1.1");
  ck_assert_int_eq(reply->integer, 1);
}
END_TEST

START_TEST(blacklist_and_expire_user_test)
{
  blacklist_and_expire(context, USER, "test", 200, "IP Blacklist And Expire Test");

  reply = redisCommand(context, "TTL test:repsheet:users:blacklisted");
  ck_assert_int_eq(reply->integer, 200);

  reply = redisCommand(context, "GET test:repsheet:users:blacklisted");
  ck_assert_str_eq(reply->str, "IP Blacklist And Expire Test");

  reply = redisCommand(context, "SISMEMBER repsheet:users:blacklisted:history test");
  ck_assert_int_eq(reply->integer, 1);
}
END_TEST

START_TEST(increment_rule_count_test)
{
  increment_rule_count(context, "1.1.1.1", "950001");
  reply = redisCommand(context, "ZRANGE 1.1.1.1:detected 0 -1");

  ck_assert_int_eq(reply->elements, 1);
  ck_assert_str_eq(reply->element[0]->str, "950001");
}
END_TEST

Suite *make_common_suite(void) {
  Suite *suite = suite_create("Common Suite");

  TCase *tc_expiry = tcase_create("Expiry");
  tcase_add_checked_fixture(tc_expiry, common_setup, common_teardown);
  tcase_add_test(tc_expiry, expire_test);
  tcase_add_test(tc_expiry, blacklist_and_expire_ip_test);
  tcase_add_test(tc_expiry, blacklist_and_expire_user_test);
  suite_add_tcase(suite, tc_expiry);

  TCase *tc_rule_count = tcase_create("Rule Count");
  tcase_add_checked_fixture(tc_rule_count, common_setup, common_teardown);
  tcase_add_test(tc_rule_count, increment_rule_count_test);
  suite_add_tcase(suite, tc_rule_count);

  return suite;
}
