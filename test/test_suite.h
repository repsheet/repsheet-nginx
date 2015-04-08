#ifndef __TEST_SUITE_H
#define __TEST_SUITE_H

#include <check.h>

Suite *make_librepsheet_connection_suite();
Suite *make_cidr_suite();
Suite *make_blacklist_suite();
Suite *make_whitelist_suite();
Suite *make_marked_suite();
Suite *make_xff_suite();
Suite *make_common_suite();

#endif
