#include <stdlib.h>
#include "test_suite.h"

int main(void) {
  SRunner *runner = srunner_create(make_librepsheet_connection_suite());
  srunner_add_suite(runner, make_cidr_suite());
  srunner_add_suite(runner, make_blacklist_suite());
  srunner_add_suite(runner, make_whitelist_suite());
  srunner_add_suite(runner, make_marked_suite());
  srunner_add_suite(runner, make_modsecurity_suite());
  srunner_add_suite(runner, make_recorder_suite());
  srunner_add_suite(runner, make_xff_suite());
  srunner_add_suite(runner, make_common_suite());
  srunner_run_all(runner, CK_VERBOSE);
  int number_failed = srunner_ntests_failed(runner);
  srunner_free(runner);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
