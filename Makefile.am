ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src . test

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = repsheet.pc

doc:
	doxygen .doxygen

clean-local:
	rm -rf src/*.o src/*.lo src/*.la src/*.gcda src/*.gcno test/*.gcda test/*.gcno librepsheet_test.info
