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
AC_CHECK_LIB(rt, clock_gettime, dummy=yes, AC_MSG_ERROR(librt is required))

AC_ARG_ENABLE(debug,
  [  --enable-debug            Enable debug build (default=disabled)],
  [debug=$enableval],
  [debug="no"])
AM_CONDITIONAL( [WANT_DEBUG], [test x"$debug" = x"yes"])

AC_ARG_ENABLE(trace,
  [  --enable-trace            Enable TRACE statements (default=disabled)],
  [trace=$enableval],
  [trace="no"])
AM_CONDITIONAL( [WANT_TRACE], [test x"$trace" = x"yes"])

AC_ARG_ENABLE(mesa,
  [  --enable-mesa            Enable mesa headers (default=disabled)],
  [mesa=$enableval],
  [mesa="no"])
AM_CONDITIONAL( [WANT_MESA], [test x"$mesa" = x"yes"])

AC_ARG_ENABLE(wayland,
  [  --enable-wayland            Enable wayland support (default=disabled)],
  [wayland=$enableval
	PKG_CHECK_MODULES(WAYLAND_CLIENT, wayland-client,, exit)
	PKG_CHECK_MODULES(WAYLAND_SERVER, wayland-server,, exit)
	WAYLAND_PREFIX=`$PKG_CONFIG --variable=prefix wayland-client`

	AC_PATH_PROG([WAYLAND_SCANNER], [wayland-scanner],, [${WAYLAND_PREFIX}/bin$PATH_SEPARATOR$PATH])
	AC_DEFINE(WANT_WAYLAND, [], [We want Wayland support])
],
  [wayland="no"])
AM_CONDITIONAL( [WANT_WAYLAND], [test x"$wayland" = x"yes"])


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
      android_version=0x230
   elif test "x$enableval" = "xics" ; then
      alinker="ics"
      android_version=0x400
   elif test "x$enableval" = "xjb" ; then
      alinker="jb"
      android_version=0x410
   else
      echo
      echo "Error!"
      echo "Unknown android version"
      exit -1
   fi
  ],
  [
   alinker="gb"
   android_version=0x230
  ]
  )
AM_CONDITIONAL([WANT_GB], [test x$alinker = xgb])
AM_CONDITIONAL([WANT_ICS], [test x$alinker = xics])
AM_CONDITIONAL([WANT_JB], [test x$alinker = xjb])

AC_DEFINE_UNQUOTED(ANDROID_VERSION, [$android_version], [Android version])
AC_SUBST(ANDROID_VERSION, [$android_version])

AC_CONFIG_FILES([
	Makefile
	common/Makefile
	common/gingerbread/Makefile
	common/ics/Makefile
	common/jb/Makefile
	egl/egl.pc
	egl/Makefile
	egl/platforms/Makefile
	egl/platforms/common/Makefile
	egl/platforms/common/hybris-egl-platform.pc
	egl/platforms/null/Makefile
	egl/platforms/fbdev/Makefile
	egl/platforms/wayland/Makefile
	egl/platforms/hwcomposer/Makefile
	glesv2/glesv2.pc
	glesv2/Makefile
	hardware/Makefile
	libsync/Makefile
	ui/Makefile
	sf/Makefile
	input/Makefile
	camera/Makefile
	include/Makefile
	tests/Makefile
])
AM_COND_IF([WANT_WAYLAND], [AC_CONFIG_FILES([egl/platforms/common/wayland-egl.pc])])
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
echo "  trace...................: $trace"
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

