#include "../src/librepsheet.h"
#include "test_suite.h"

START_TEST(base_connection)
{
  redisContext *context = get_redis_context("localhost", 6379, 0);

  ck_assert(&context);
  ck_assert_int_eq(context->err, 0);
}
END_TEST

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

Suite *make_librepsheet_connection_suite(void) {
  Suite *suite = suite_create("librepsheet connection");

  TCase *tc_connection = tcase_create("connection");
  tcase_add_test(tc_connection, base_connection);
  suite_add_tcase(suite, tc_connection);

  TCase *tc_connection_operations = tcase_create("connection operations");
  tcase_add_test(tc_connection_operations, increment_rule_count_test);
  tcase_add_test(tc_connection_operations, mark_actor_test);
  suite_add_tcase(suite, tc_connection_operations);

  return suite;
}
