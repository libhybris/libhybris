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
      exit 1
   fi
  ],
  [arch="arm"]
  )
AM_CONDITIONAL([WANT_ARCH_ARM], [test x$arch = xarm])
AM_CONDITIONAL([WANT_ARCH_X86], [test x$arch = xx86])

DEFAULT_EGL_PLATFORM="null"
AC_ARG_WITH(default-egl-platform,
  [  --with-default-egl-platform=NAME     Use NAME as default EGL platform if not specified by environment ],
  [ DEFAULT_EGL_PLATFORM="$withval" ],
  [ ])
AC_SUBST(DEFAULT_EGL_PLATFORM)

DEFAULT_HYBRIS_LD_LIBRARY_PATH="/vendor/lib:/system/lib"
AC_ARG_WITH(default-hybris-ld-library-path,
  [  --with-default-hybris-ld-library-path=PATH1:PATH2:...     Use PATH1:PATH2 for default HYBRIS_LD_LIBRARY_PATH if not specified by environment ],
  [ DEFAULT_HYBRIS_LD_LIBRARY_PATH="$withval" ],
  [ ])
AC_SUBST(DEFAULT_HYBRIS_LD_LIBRARY_PATH)


android_headers_path=""
AC_ARG_WITH(android-headers,
  [  --with-android-headers=DIR     Use android headers available in DIR. See utils/extract-headers.sh ],
  [
      if [ ! test -e $withval/android-version.h ]; then
	echo "Error! missing android-version.h in android header path"
	exit 1
      fi 
      rm -f include/android
      ln -sf $withval include/android   	
      android_headers_path="$withval"      

      # According to http://en.wikipedia.org/wiki/Android_version_history#Version_history_by_API_level
      android_headers_major=`grep -E "define ANDROID_VERSION_MAJOR " $android_headers_path/android-version.h | sed "s|^\#define ANDROID_VERSION_MAJOR \(.*\)$|\1|g"`

      android_headers_minor=`grep -E "define ANDROID_VERSION_MINOR " $android_headers_path/android-version.h | sed "s|^\#define ANDROID_VERSION_MINOR \(.*\)$|\1|g"`

      android_headers_patch=`grep -E "define ANDROID_VERSION_PATCH " $android_headers_path/android-version.h | sed "s|^\#define ANDROID_VERSION_PATCH \(.*\)$|\1|g"`
  
      if [ test "x$android_headers_major" = x ]; then
	  echo "Error! Was unable to locate define ANDROID_VERSION_MAJOR in $android_headers_path/android-version.h"
      fi

      AC_SUBST(ANDROID_VERSION_MAJOR, [$android_headers_major])
      AC_SUBST(ANDROID_VERSION_MINOR, [$android_headers_minor])
      AC_SUBST(ANDROID_VERSION_PATCH, [$android_headers_patch])

      echo "Android headers version is $android_headers_major.$android_headers_minor.$android_headers_patch"
  ],
  [
      echo "Error! need to provide path to Android headers --with-android-headers=DIR. See utils/extract-headers.sh"
      exit 1
  ]
  )

# Add automake tests for version/API needs here that you need in code, including test .am's

AM_CONDITIONAL([HAS_ANDROID_4_2_0], [test $android_headers_major -ge 4 -a $android_headers_minor -ge 2 ])
AM_CONDITIONAL([HAS_ANDROID_4_1_0], [test $android_headers_major -ge 4 -a $android_headers_minor -ge 1 ])
AM_CONDITIONAL([HAS_ANDROID_4_0_0], [test $android_headers_major -ge 4 -a $android_headers_minor -ge 0 ])
AM_CONDITIONAL([HAS_ANDROID_2_3_0], [test $android_headers_major -ge 2 -a $android_headers_minor -ge 3 ])

AM_CONDITIONAL([HAS_LIBNFC_NXP_HEADERS], [test -f ${android_headers_path}/libnfc-nxp/phLibNfc.h])

AC_CONFIG_FILES([
	Makefile
	properties/Makefile
	properties/libandroid-properties.pc
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
	egl/platforms/hwcomposer/hwcomposer-egl.pc
	glesv1/glesv1_cm.pc
	glesv1/Makefile
	glesv2/glesv2.pc
	glesv2/Makefile
	hardware/Makefile
	hardware/libhardware.pc
	libnfc_nxp/Makefile
	libnfc_nxp/libnfc_nxp.pc
	libnfc_ndef_nxp/Makefile
	libnfc_ndef_nxp/libnfc_ndef_nxp.pc
	libsync/Makefile
	libsync/libsync.pc
	ui/Makefile
	sf/Makefile
	sf/libsf.pc
	input/Makefile
	input/libis.pc
	camera/Makefile
	camera/libcamera.pc
	media/Makefile
	media/libmedia.pc
	wifi/Makefile
	wifi/libwifi.pc
	include/Makefile
	utils/Makefile
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
echo "  headers path ...........: $android_headers_path"
echo
echo "  default egl platform ...: $DEFAULT_EGL_PLATFORM"
echo
echo "  default ld_library_path.: $DEFAULT_HYBRIS_LD_LIBRARY_PATH"
echo
echo "------------------------------------------------------------------------"
echo
echo "Now type 'make' to compile and 'make install' to install this package."

