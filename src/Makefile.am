AUTOMAKE_OPTIONS = -Wno-override
SUBDIRS = hiredis
include_HEADERS = repsheet.h
lib_LTLIBRARIES = librepsheet.la
librepsheet_la_SOURCES = librepsheet.c \
                         common.c      \
                         cidr.c        \
                         blacklist.c   \
                         whitelist.c   \
                         marked.c      \
                         xff.c
librepsheet_la_LIBADD = hiredis/libhiredis.so
librepsheet_la_LDFLAGS = -lm
librepsheet_la_CFLAGS = -std=c99

