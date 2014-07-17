TESTS = test_runner
check_PROGRAMS = $(TESTS)
test_runner_SOURCES = librepsheet_test.c   \
                      test_runner.c        \
                      ../src/librepsheet.c \
                      ../src/librepsheet.h
test_runner_CFLAGS = -g -O0 --coverage $(CHECK_CFLAGS)
test_runner_LDADD = $(CHECK_LIBS) -lhiredis -lpcre -lm
