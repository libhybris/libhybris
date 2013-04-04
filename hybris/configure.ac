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
AC_PROG_CXX
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

AC_ARG_ENABLE(mesa,
  [  --enable-mesa            Enable mesa headers (default=disabled)],
  [mesa=$enableval],
  [mesa="no"])
AM_CONDITIONAL( [WANT_MESA], [test x"$mesa" = x"yes"])

AC_ARG_ENABLE(arch,
  [  --enable-arch[=arch]     Compile specific CPU target(default=arm)
                              arm: compile for ARM
                              x86: Compile for x86],
  [ if   test "x$enableval" = "xarm" ; then
      arch="arm"
   elif test "x$enableval" = "xx86" ; then
      arch="x86"
   else
      echo
      echo "Error!"
      echo "Unknown architecture' type"
      exit -1
   fi
  ],
  [arch="arm"]
  )
AM_CONDITIONAL([WANT_ARCH_ARM], [test x$arch = xarm])
AM_CONDITIONAL([WANT_ARCH_X86], [test x$arch = xx86])

AC_ARG_ENABLE(alinker,
  [  --enable-alinker[=linker]      Compile specific Android Linker (default=gingerbread)
                                    gb: gingerbread
                                    ics: Ice Cream sandwich
                                    jb: Jelly Bean],
  [ if   test "x$enableval" = "xgb" ; then
      alinker="gb"
   elif test "x$enableval" = "xics" ; then
      alinker="ics"
   elif test "x$enableval" = "xjb" ; then
      alinker="jb"
   else
      echo
      echo "Error!"
      echo "Unknown android version"
      exit -1
   fi
  ],
  [alinker="gb"]
  )
AM_CONDITIONAL([WANT_GB], [test x$alinker = xgb])
AM_CONDITIONAL([WANT_ICS], [test x$alinker = xics])
AM_CONDITIONAL([WANT_JB], [test x$alinker = xjb])


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
echo "  arch  ..................: $arch"
echo
echo "  linker..................: $alinker"
echo
echo "------------------------------------------------------------------------"
echo
echo "Now type 'make' to compile and 'make install' to install this package."

