#! /bin/sh

error() {
    echo "Error: $1" >&2
}
usage() {
    echo "Usage: extract-headers.sh [Options] <ANDROID_ROOT> <HEADER_PATH>"
    echo
    echo "  ANDROID_ROOT: Directory containing the Android source tree."
    echo "  HEADER_PATH:  Where the headers will be extracted to."
    echo
    echo "Options:"
    echo "  -v|--version MAJOR.MINOR.PATCH.[PATCH2].[PATCH3]]"
    echo "               Override the Android version detected by this script"
    echo "  -p|--pkgconfigpath <path>"
    echo "               pkgconfig path for the headers installed location"
    exit 1
}

##############
while [ $# -gt 0 ]; do
    case $1 in
    *-version|-v)
        if [ x$2 = x ]; then error "--version needs an argument"; usage; fi
        IFS="." read MAJOR MINOR PATCH PATCH2 PATCH3 <<EOF
$2
EOF
        echo "Using version ${MAJOR}.${MINOR}.${PATCH}.${PATCH2}.${PATCH3}"
        shift 2
    ;;
    *-pkgconfigpath|-p)
	if [ "$2" = "" ]; then error "--pkgconfigpath needs an argument"; usage; fi
	PKGCONFIGPATH=$2
	shift 2
    ;;
    *)
        break
    ;;
    esac
done

##############
ANDROID_ROOT=$1
HEADER_PATH=$2

# Required arguments
[ x$ANDROID_ROOT = x ] && error "missing argument ANDROID_ROOT" && usage
[ x$HEADER_PATH = x ] && error "missing argument HEADER_PATH" && usage

shift 2

# check if android source exists
if [ ! -e "$ANDROID_ROOT" ]; then
    error "Cannot extract headers: '$ANDROID_ROOT' does not exist."
    exit 1
fi

# In case one of the version number is missing,
# try to extract if from the version_defaults.mk
if [ x$MAJOR = x -o x$MINOR = x -o x$PATCH = x ]; then
    VERSION_DEFAULTS=$ANDROID_ROOT/build/core/version_defaults.mk

    echo "not all version fields supplied:  trying to extract from $VERSION_DEFAULTS"
    if [ ! -f $VERSION_DEFAULTS ]; then
        error "$VERSION_DEFAULTS not found"
    fi

    IFS="." read MAJOR MINOR PATCH PATCH2 PATCH3 <<EOF
$(IFS="." awk '/PLATFORM_VERSION([A-Z0-9.]*|_LAST_STABLE) := ([0-9.]+)/ { print $3; }' < $VERSION_DEFAULTS)
EOF
    if [ x$MINOR = x ]; then
         MINOR=0
    fi
    if [ x$PATCH = x ]; then
         PATCH=0
    fi
    if [ x$MAJOR = x -o x$MINOR = x -o x$PATCH = x ]; then
        error "Cannot read PLATFORM_VERSION from ${VERSION_DEFAULTS}."
        error "Please specify MAJOR, MINOR and PATCH manually to continue."
        exit 1
    fi

    echo -n "Auto-detected version: ${MAJOR}.${MINOR}.${PATCH}";echo "${PATCH2:+.${PATCH2}}${PATCH3:+.${PATCH3}}"
fi

##############
require_sources() {
    # require_sources [FILE|DIR] ...
    # Check if the given paths exist in the Android source
    while [ $# -gt 0 ]; do
        SOURCE_PATH=$ANDROID_ROOT/$1
        shift

        if [ ! -e "$SOURCE_PATH" ]; then
            error "Cannot extract headers: '$SOURCE_PATH' does not exist."
            exit 1
        fi
    done
}

extract_headers_to() {
    # extract_headers_to <TARGET> [FILE|DIR] ...
    # For each FILE argument, copy it to TARGET
    # For each DIR argument, copy all its contents to TARGET
    TARGET_DIRECTORY=$HEADER_PATH/$1
    if [ ! -d "$TARGET_DIRECTORY" ]; then
        mkdir -p "$TARGET_DIRECTORY"
    fi
    echo "  $1"
    shift

    while [ $# -gt 0 ]; do
        SOURCE_PATH=$ANDROID_ROOT/$1
        if [ -d $SOURCE_PATH ]; then
            for file in $SOURCE_PATH/*; do
                echo "    $1/$(basename $file)"
                cp -L $file $TARGET_DIRECTORY/
            done
        else
            echo "    $1"
            cp -L $SOURCE_PATH $TARGET_DIRECTORY/
        fi
        shift
    done
}

check_header_exists() {
    # check_header_exists <FILENAME>
    HEADER_FILE=$ANDROID_ROOT/$1
    if [ ! -e "$HEADER_FILE" ]; then
        return 1
    fi

    return 0
}

##############

# Make sure that the dir given contains at least some of the assumed structures.
require_sources \
    hardware/libhardware/include/hardware

mkdir -p $HEADER_PATH

# Default PATCH2,3 to 0
PATCH2=${PATCH2:-0}
PATCH3=${PATCH3:-0}

cat > $HEADER_PATH/android-version.h << EOF
#ifndef ANDROID_VERSION_H_
#define ANDROID_VERSION_H_

#define ANDROID_VERSION_MAJOR $MAJOR
#define ANDROID_VERSION_MINOR $MINOR
#define ANDROID_VERSION_PATCH $PATCH
#define ANDROID_VERSION_PATCH2 $PATCH2
#define ANDROID_VERSION_PATCH3 $PATCH3

#endif
EOF

cat > $HEADER_PATH/android-config.h << EOF
#ifndef HYBRIS_CONFIG_H_
#define HYBRIS_CONFIG_H_

/* When android is built for a specific device the build is
   modified by BoardConfig.mk and possibly other mechanisms.
   eg
   device/samsung/i9305/BoardConfig.mk:
       COMMON_GLOBAL_CFLAGS += -DCAMERA_WITH_CITYID_PARAM
   device/samsung/smdk4412-common/BoardCommonConfig.mk:
       COMMON_GLOBAL_CFLAGS += -DEXYNOS4_ENHANCEMENTS

   This file allows those global configurations, which are not
   otherwise defined in the build headers, to be available in
   hybris builds.

   Typically it is generated at hardware adaptation time.

   The CONFIG GOES... line below can be used by automation to modify
   this file.
*/

#include <android-version.h>

/* CONFIG GOES HERE */

#endif
EOF

extract_headers_to hardware \
    hardware/libhardware/include/hardware

check_header_exists hardware/libhardware_legacy/include/hardware_legacy/vibrator.h && \
    extract_headers_to hardware_legacy \
        hardware/libhardware_legacy/include/hardware_legacy/vibrator.h

check_header_exists hardware/libhardware_legacy/include/hardware_legacy/wifi.h && \
    extract_headers_to hardware_legacy \
        hardware/libhardware_legacy/include/hardware_legacy/wifi.h

check_header_exists frameworks/opt/net/wifi/libwifi_hal/include/hardware_legacy/wifi.h && \
    extract_headers_to hardware_legacy \
        frameworks/opt/net/wifi/libwifi_hal/include/hardware_legacy/wifi.h

extract_headers_to hardware_legacy \
    hardware/libhardware_legacy/include/hardware_legacy/audio_policy_conf.h

extract_headers_to cutils \
    system/core/include/cutils

extract_headers_to log \
    system/core/include/log

extract_headers_to system \
    system/core/include/system

check_header_exists system/media/audio/include/system/audio.h && \
    extract_headers_to system \
        system/media/audio/include/system

extract_headers_to android \
    system/core/include/android

check_header_exists bionic/libc/kernel/common/linux/sync.h && \
    extract_headers_to linux \
        bionic/libc/kernel/common/linux/sync.h \
        bionic/libc/kernel/common/linux/sw_sync.h

check_header_exists system/core/include/sync/sync.h && \
    extract_headers_to sync \
        system/core/include/sync

check_header_exists bionic/libc/kernel/uapi/linux/sync.h && \
    extract_headers_to linux \
        bionic/libc/kernel/uapi/linux/sync.h \
        bionic/libc/kernel/uapi/linux/sw_sync.h

check_header_exists bionic/libc/kernel/uapi/linux/sync_file.h && \
    extract_headers_to linux \
        bionic/libc/kernel/uapi/linux/sync_file.h

check_header_exists bionic/libc/include/android/dlext.h && \
    extract_headers_to android \
        bionic/libc/include/android/dlext.h \
        bionic/libc/include/android/api-level.h \
        bionic/libc/include/android/set_abort_message.h

check_header_exists system/core/libsync/include/sync/sync.h && \
    extract_headers_to sync \
        system/core/libsync/include/sync

check_header_exists system/core/libsync/include/ndk/sync.h && \
    extract_headers_to ndk \
        system/core/libsync/include/ndk

check_header_exists external/libnfc-nxp/inc/phNfcConfig.h && \
    extract_headers_to libnfc-nxp \
        external/libnfc-nxp/inc \
        external/libnfc-nxp/src

check_header_exists system/media/radio/include/system/radio_metadata.h && \
    extract_headers_to system \
        system/media/radio/include/system/radio_metadata.h

extract_headers_to private \
    system/core/include/private/android_filesystem_config.h \
    bionic/libc/private

check_header_exists frameworks/native/libs/nativewindow/include/android/native_window.h && \
    extract_headers_to android \
        frameworks/native/libs/nativewindow/include/android

check_header_exists frameworks/native/libs/arect/include/android/rect.h && \
    extract_headers_to android \
        frameworks/native/libs/arect/include/android/rect.h

check_header_exists frameworks/native/libs/nativebase/include/nativebase/nativebase.h && \
    extract_headers_to nativebase \
        frameworks/native/libs/nativebase/include/nativebase/nativebase.h

check_header_exists frameworks/native/libs/nativewindow/include/system/window.h && \
    extract_headers_to system \
        frameworks/native/libs/nativewindow/include/system/window.h

check_header_exists frameworks/native/libs/nativewindow/include/vndk/window.h && \
    extract_headers_to vndk \
        frameworks/native/libs/nativewindow/include/vndk

check_header_exists frameworks/native/libs/nativewindow/include/apex/window.h && \
    extract_headers_to apex \
        frameworks/native/libs/nativewindow/include/apex

# In order to make it easier to trace back the origins of headers, fetch
# some repository information from the Git source tree (if available).
# Tested with AOSP and CM.
NOW=$(LC_ALL=C date)

# Add here all sub-projects of AOSP/CM from which headers are extracted
GIT_PROJECTS="
    bionic
    hardware/libhardware
    hardware/libhardware_legacy
    system/core
    system/media
    external/kernel-headers
    external/libnfc-nxp
"

echo "Extracting Git revision information"
if ! which repo >/dev/null 2>&1; then
    echo "Can't extract Git info: missing 'repo'"
    SKIPGIT=1
fi
if ! which git >/dev/null 2>&1; then
    echo "Can't extract Git info: missing 'git'"
    SKIPGIT=1
fi


rm -f $HEADER_PATH/SOURCE_GIT_REVISION_INFO
if [ x$SKIPGIT != x1 ]; then
    (for GIT_PROJECT in $GIT_PROJECTS; do
	TARGET_DIR=$ANDROID_ROOT/$GIT_PROJECT
	echo "================================================"
	echo "$GIT_PROJECT @ $NOW"
	echo "================================================"
	if [ -e $TARGET_DIR/.git ]; then
            (
		set -x
		cd $ANDROID_ROOT
		repo status $GIT_PROJECT
		cd $TARGET_DIR
		git show-ref --head
		git remote -v
            )
            echo ""
            echo ""
	else
            echo "WARNING: $GIT_PROJECT does not contain a Git repository"
	fi
	done) > $HEADER_PATH/git-revisions.txt 2>&1

    # Repo manifest that can be used to fetch the sources for re-extracting headers
    if [ -e $ANDROID_ROOT/.repo/manifest.xml ]; then
	cp $ANDROID_ROOT/.repo/manifest.xml $HEADER_PATH/
    fi
fi

find "$HEADER_PATH" -type f -exec chmod 0644 {} \;


# Create a pkconfig if we know the final installation path otherwise
# make a Makefile and a pc.in file for it to handle
if [ x$PKGCONFIGPATH = x ]; then
    # Add a makefile to make packaging easier
    cat > ${HEADER_PATH}/Makefile << EOF
PREFIX?=/usr/local
INCLUDEDIR?=\$(PREFIX)/include/android
all:
	@echo "Use '\$(MAKE) install' to install"

install:
	mkdir -p \$(DESTDIR)/\$(INCLUDEDIR)
	cp -r * \$(DESTDIR)/\$(INCLUDEDIR)
	rm -f \$(DESTDIR)/\$(INCLUDEDIR)/Makefile
	rm -f \$(DESTDIR)/\$(INCLUDEDIR)/android-headers.pc.in
	mkdir -p \$(DESTDIR)/\$(PREFIX)/lib/pkgconfig
	sed -e 's;@prefix@;\$(PREFIX)/;g; s;@includedir@;\$(INCLUDEDIR);g' android-headers.pc.in > \$(DESTDIR)/\$(PREFIX)/lib/pkgconfig/android-headers.pc
EOF

    echo "Creating ${HEADER_PATH}/android-headers.pc.in"
    cat > ${HEADER_PATH}/android-headers.pc.in << EOF
Name: android-headers
Description: Provides the headers for the droid system
Version: ${MAJOR}.${MINOR}.${PATCH}
Cflags: -I@includedir@
EOF
else
    echo "Creating ${HEADER_PATH}/android-headers.pc"
    cat > ${HEADER_PATH}/android-headers.pc << EOF
Name: android-headers
Description: Provides the headers for the droid system
Version: ${MAJOR}.${MINOR}.${PATCH}
Cflags: -I$PKGCONFIGPATH
EOF
fi
# vim: noai:ts=4:sw=4:ss=4:expandtab
