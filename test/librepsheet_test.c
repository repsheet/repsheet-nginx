#include "../src/librepsheet.h"
#include "test_suite.h"

START_TEST(increment_rule_count_test)
{
  redisContext *context = get_redis_context("localhost", 6379, 0);

  increment_rule_count(context, "1.1.1.1", "950001");

  redisReply *reply;
  reply = redisCommand(context, "ZRANGE 1.1.1.1:detected 0 -1");
  ck_assert_int_eq(reply->elements, 1);
  ck_assert_str_eq(reply->element[0]->str, "950001");

}
END_TEST

START_TEST(mark_actor_test)
{
  redisContext *context = get_redis_context("localhost", 6379, 0);

  mark_actor(context, "1.1.1.1");

  redisReply *reply;
  reply = redisCommand(context, "GET 1.1.1.1:repsheet");
  ck_assert_str_eq(reply->str, "true");

}
END_TEST

START_TEST(blacklist_actor_test)
{
  redisContext *context = get_redis_context("localhost", 6379, 0);

  blacklist_actor(context, "1.1.1.1");

  redisReply *reply;
  reply = redisCommand(context, "GET 1.1.1.1:repsheet:blacklist");
  ck_assert_str_eq(reply->str, "true");

}
END_TEST

START_TEST(whitelist_actor_test)
{
  redisContext *context = get_redis_context("localhost", 6379, 0);

  whitelist_actor(context, "1.1.1.1");

  redisReply *reply;
  reply = redisCommand(context, "GET 1.1.1.1:repsheet:whitelist");
  ck_assert_str_eq(reply->str, "true");

}
END_TEST

START_TEST(expire_test)
{
  redisContext *context = get_redis_context("localhost", 6379, 0);

  mark_actor(context, "1.1.1.1");
  expire(context, "1.1.1.1", "repsheet", 200);

  redisReply *reply;
  reply = redisCommand(context, "TTL 1.1.1.1:repsheet");
  ck_assert_int_eq(reply->integer, 200);
}
END_TEST

START_TEST(is_on_repsheet_test)
{
  int response;
  redisContext *context = get_redis_context("localhost", 6379, 0);

  mark_actor(context, "1.1.1.1");
  response = is_on_repsheet(context, "1.1.1.1");

  ck_assert_int_eq(response, TRUE);
}
END_TEST

START_TEST(is_blacklisted_test)
{
  int response;
  redisContext *context = get_redis_context("localhost", 6379, 0);

  blacklist_actor(context, "1.1.1.1");
  response = is_blacklisted(context, "1.1.1.1");

  ck_assert_int_eq(response, TRUE);
}
END_TEST

START_TEST(is_whitelisted_test)
{
  int response;
  redisContext *context = get_redis_context("localhost", 6379, 0);

  whitelist_actor(context, "1.1.1.1");
  response = is_whitelisted(context, "1.1.1.1");

  ck_assert_int_eq(response, TRUE);
}
END_TEST

START_TEST(blacklist_and_expire_test)
{
  redisContext *context = get_redis_context("localhost", 6379, 0);

  blacklist_and_expire(context, "1.1.1.1", 200, "test");

  redisReply *reply;
  reply = redisCommand(context, "TTL 1.1.1.1:repsheet:blacklist");
  ck_assert_int_eq(reply->integer, 200);

  reply = redisCommand(context, "GET 1.1.1.1:repsheet:blacklist");
  ck_assert_str_eq(reply->str, "true");

  reply = redisCommand(context, "GET 1.1.1.1:repsheet:blacklist:reason");
  ck_assert_str_eq(reply->str, "test");

  reply = redisCommand(context, "SISMEMBER repsheet:blacklist:history 1.1.1.1");
  ck_assert_int_eq(reply->integer, 1);
}
END_TEST

Suite *make_librepsheet_connection_suite(void) {
  Suite *suite = suite_create("librepsheet connection");

  TCase *tc_connection_operations = tcase_create("connection operations");
  tcase_add_test(tc_connection_operations, increment_rule_count_test);

  tcase_add_test(tc_connection_operations, mark_actor_test);
  tcase_add_test(tc_connection_operations, blacklist_actor_test);
  tcase_add_test(tc_connection_operations, whitelist_actor_test);


  tcase_add_test(tc_connection_operations, is_on_repsheet_test);
  tcase_add_test(tc_connection_operations, is_blacklisted_test);
  tcase_add_test(tc_connection_operations, is_whitelisted_test);

  tcase_add_test(tc_connection_operations, expire_test);
  tcase_add_test(tc_connection_operations, blacklist_and_expire_test);

  suite_add_tcase(suite, tc_connection_operations);

  return suite;
}
