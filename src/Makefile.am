AUTOMAKE_OPTIONS = -Wno-override
include_HEADERS = repsheet.h
lib_LTLIBRARIES = librepsheet.la
librepsheet_la_SOURCES = librepsheet.c \
                         common.c      \
                         cidr.c        \
                         blacklist.c   \
                         whitelist.c   \
                         marked.c      \
                         xff.c
librepsheet_la_LDFLAGS = -lm -lhiredis
librepsheet_la_CFLAGS = -std=c99

