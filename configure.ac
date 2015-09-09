AC_PREREQ(2.61)
AC_INIT([librepsheet], [6.1.0], [aaron@aaronbedra.com])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])
AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    test/Makefile
    repsheet.pc
])

AC_ARG_ENABLE([whitelist],
	AS_HELP_STRING([--disable-whitelist],
		[disable the whitelist functionality]),
	[AC_DEFINE([WHITELIST_ENABLED], [0], [Disables whitelist])],
	[AC_DEFINE([WHITELIST_ENABLED], [1], [Enables whitelist])]
)

AC_ARG_ENABLE([blacklist],
	AS_HELP_STRING([--disable-blacklist],
		[disable the blacklist functionality]),
	[AC_DEFINE([BLACKLIST_ENABLED], [0], [Disables blacklist lookup])],
	[AC_DEFINE([BLACKLIST_ENABLED], [1], [Enables blacklist lookup])]
)

AC_ARG_ENABLE([marked],
	AS_HELP_STRING([--disable-marked],
		[disable the marked functionality]),
	[AC_DEFINE([MARKED_ENABLED], [0], [Disables marked])],
	[AC_DEFINE([MARKED_ENABLED], [1], [Enables marked])]
)

AC_OUTPUT
