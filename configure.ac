AC_INIT([libhybris], [0.1.0])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE([dist-bzip2 subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# If library source has changed since last release, increment revision
# If public symbols have been added, removed or changed since last release,
#  increment current and set revision to 0
# If public symbols have been added since last release, increment age
# If public symbols have been removed since last release, set age to 0
m4_define([hybris_lt_current], [1])
m4_define([hybris_lt_revision], [0])
m4_define([hybris_lt_age], [0])

AC_PROG_CC
AM_PROG_CC_C_O
AC_GNU_SOURCE
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)

PKG_PROG_PKG_CONFIG

LT_CURRENT=hybris_lt_current
LT_REVISION=hybris_lt_revision
LT_AGE=hybris_lt_age
AC_SUBST([LT_CURRENT])
AC_SUBST([LT_REVISION])
AC_SUBST([LT_AGE])

AC_CHECK_LIB(dl, dlopen, dummy=yes, AC_MSG_ERROR(dynamic linking loader is required))

AC_ARG_ENABLE(debug,
  [  --enable-debug            Enable debug build (default=disabled)],
  [debug=$enableval],
  [debug="no"])
AM_CONDITIONAL( [WANT_DEBUG], [test x"$debug" = x"yes"])

AC_CONFIG_FILES([
	Makefile
	common/Makefile
	egl/Makefile
	glesv2/Makefile
	hardware/Makefile
	include/Makefile
	tests/Makefile
])
AC_OUTPUT

echo
echo "------------------------------------------------------------------------"
echo "$PACKAGE_NAME $PACKAGE_VERSION"
echo "------------------------------------------------------------------------"
echo
echo "Configuration Options:"
echo
echo "  debug build.............: $debug"
echo
echo "  prefix..................: $prefix"
echo
echo "------------------------------------------------------------------------"
echo
echo "Now type 'make' to compile and 'make install' to install this package."

