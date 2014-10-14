TESTS = test_runner
check_PROGRAMS = $(TESTS)
test_runner_SOURCES = librepsheet_test.c   \
                      cidr_test.c          \
                      test_runner.c        \
                      ../src/librepsheet.c \
                      ../src/librepsheet.h \
                      ../src/common.c      \
                      ../src/common.h      \
                      ../src/cidr.h        \
                      ../src/cidr.c        \
                      ../src/blacklist.c   \
                      ../src/blacklist.h   \
                      ../src/whitelist.c   \
                      ../src/whitelist.h   \
                      ../src/marked.c      \
                      ../src/marked.h      \
                      ../src/modsecurity.c \
                      ../src/modsecurity.h \
                      ../src/recorder.c    \
                      ../src/recorder.h    \
                      ../src/xff.c         \
                      ../src/xff.h
test_runner_CFLAGS = -g -O0 --coverage $(CHECK_CFLAGS)
test_runner_LDADD = $(CHECK_LIBS) -lhiredis -lpcre -lm
