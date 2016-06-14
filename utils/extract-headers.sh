#!/bin/sh

ANDROID_ROOT=$1
HEADERPATH=$2
MAJOR=$3
MINOR=$4

PATCH=$5
PATCH2=$6
PATCH3=$7

usage() {
    echo "Usage: extract-headers.sh <ANDROID_ROOT> <HEADER_PATH> [Android Platform Version]"
    echo
    echo "  ANDROID_ROOT: Directory containing the Android source tree."
    echo "  HEADER_PATH:  Where the headers will be extracted to."
    echo
    echo "Android Platform Version:"
    echo "  This field is optional. If not specified, automatic extraction is attempted."
    echo
    echo "Ex:"
    echo "    ./extract-headers.sh  android-aosp/  /tmp/android-headers/  4 2 2"

}

if [ x$ANDROID_ROOT = x -o "x$HEADERPATH" = x ]; then
    usage
    exit 1
fi


if [ x$MAJOR = x -o x$MINOR = x -o x$PATCH = x ]; then
    VERSION_DEFAULTS=$ANDROID_ROOT/build/core/version_defaults.mk

    parse_defaults_failed() {
        echo "Error: Cannot read PLATFORM_VERSION from ${VERSION_DEFAULTS}."
        echo "Please specify MAJOR, MINOR and PATCH manually to continue."
        exit 1
    }

    if [ ! -f $VERSION_DEFAULTS ]; then
        parse_defaults_failed
    fi

    IFS="." read MAJOR MINOR PATCH PATCH2 PATCH3 <<EOF
$(IFS="." awk '/PLATFORM_VERSION := ([0-9.]+)/ { print $3; }' < $VERSION_DEFAULTS)
EOF

    if [ x$MAJOR = x -o x$MINOR = x ]; then
        parse_defaults_failed
    fi
    if [ x$PATCH = x ]; then
        PATCH=0
    fi

    echo -n "Auto-detected version: ${MAJOR}.${MINOR}.${PATCH}";echo "${PATCH2:+.${PATCH2}}${PATCH3:+.${PATCH3}}"
fi

require_sources() {
    # require_sources [FILE|DIR] ...
    # Check if the given paths exist in the Android source
    while [ $# -gt 0 ]; do
        SOURCE_PATH=$ANDROID_ROOT/$1
        shift

        if [ ! -e "$SOURCE_PATH" ]; then
            echo "Cannot extract headers: '$SOURCE_PATH' does not exist."
            exit 1
        fi
    done
}

extract_headers_to() {
    # extract_headers_to <TARGET> [FILE|DIR] ...
    # For each FILE argument, copy it to TARGET
    # For each DIR argument, copy all its contents to TARGET
    TARGET_DIRECTORY=$HEADERPATH/$1
    echo "  $1"
    shift

    while [ $# -gt 0 ]; do
        SOURCE_PATH=$ANDROID_ROOT/$1
        if [ -d $SOURCE_PATH ]; then
            for file in $SOURCE_PATH/*.h; do
                echo "    $1/$(basename $file)"
                mkdir -p $TARGET_DIRECTORY
                cp $file $TARGET_DIRECTORY/
            done
        elif [ -f $SOURCE_PATH ]; then
            echo "    $1"
            mkdir -p $TARGET_DIRECTORY
            cp $SOURCE_PATH $TARGET_DIRECTORY/
        else
            echo "Missing file: $1"
        fi
        shift
    done
}


# Make sure that the dir given contains at least some of the assumed structures.
require_sources \
    hardware/libhardware/include/hardware

mkdir -p $HEADERPATH

# Default PATCH2,3 to 0
PATCH2=${PATCH2:-0}
PATCH3=${PATCH3:-0}

cat > $HEADERPATH/android-version.h << EOF
#ifndef ANDROID_VERSION_H_
#define ANDROID_VERSION_H_

#define ANDROID_VERSION_MAJOR $MAJOR
#define ANDROID_VERSION_MINOR $MINOR
#define ANDROID_VERSION_PATCH $PATCH
#define ANDROID_VERSION_PATCH2 $PATCH2
#define ANDROID_VERSION_PATCH3 $PATCH3

#endif
EOF

cat > $HEADERPATH/android-config.h << EOF
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

   The CONFIG GOES HERE line can be used by automation to modify
   this file.
*/

#include <android-version.h>

/* CONFIG GOES HERE */

#endif
EOF

cat > $HEADERPATH/android-headers.pc <<'EOF'
Name: Android header files
Description: Header files needed to write applications for the Android platform
Version: androidversion

prefix=/usr
exec_prefix=${prefix}
includedir=${prefix}/include

Cflags: -I${includedir}/android
EOF

sed -i -e s:androidversion:$MAJOR.$MINOR.$PATCH:g $HEADERPATH/android-headers.pc

extract_headers_to hardware \
    hardware/libhardware/include/hardware

extract_headers_to hardware_legacy \
    hardware/libhardware_legacy/include/hardware_legacy/vibrator.h
if [ $MAJOR -ge 4 -a $MINOR -ge 1 -o $MAJOR -ge 5 ]; then
    extract_headers_to hardware_legacy \
        hardware/libhardware_legacy/include/hardware_legacy/audio_policy_conf.h
fi

extract_headers_to cutils \
    system/core/include/cutils

if [ $MAJOR -eq 4 -a $MINOR -ge 4 -o $MAJOR -ge 5 ]; then
    extract_headers_to log \
        system/core/include/log
fi

if [ $MAJOR -ge 4 ]; then
    extract_headers_to system \
        system/core/include/system
fi

extract_headers_to android \
    system/core/include/android

if [ $MAJOR -eq 4 -a $MINOR -ge 1 ]; then
    extract_headers_to linux \
        bionic/libc/kernel/common/linux/sync.h \
        bionic/libc/kernel/common/linux/sw_sync.h

    extract_headers_to sync \
        system/core/include/sync
elif [ $MAJOR -eq 5 ]; then
    extract_headers_to linux \
        bionic/libc/kernel/uapi/linux/sync.h \
        bionic/libc/kernel/uapi/linux/sw_sync.h

    extract_headers_to sync \
        system/core/libsync/include/sync
fi

if [ $MAJOR -eq 2 -a $MINOR -ge 3 -o $MAJOR -ge 3 ]; then
    extract_headers_to libnfc-nxp \
        external/libnfc-nxp/inc \
        external/libnfc-nxp/src
fi

extract_headers_to private \
    system/core/include/private/android_filesystem_config.h

if [ $MAJOR -ge 5 ]; then
    extract_headers_to linux \
        bionic/libc/kernel/uapi/linux/android_alarm.h \
        bionic/libc/kernel/uapi/linux/binder.h
elif [ $MAJOR -eq 2 -a $MINOR -ge 2 -o $MAJOR -ge 3 ]; then
    extract_headers_to linux \
        external/kernel-headers/original/linux/android_alarm.h \
        external/kernel-headers/original/linux/binder.h
else
    extract_headers_to linux \
        bionic/libc/kernel/common/linux/android_alarm.h \
        bionic/libc/kernel/common/linux/binder.h
fi

# In order to make it easier to trace back the origins of headers, fetch
# some repository information from the Git source tree (if available).
# Tested with AOSP and CM.
NOW=$(LC_ALL=C date)

# Add here all sub-projects of AOSP/CM from which headers are extracted
GIT_PROJECTS="
    hardware/libhardware
    hardware/libhardware_legacy
    system/core
    external/libnfc-nxp
    external/linux-headers
"

echo "Extracting Git revision information"
rm -f $HEADERPATH/SOURCE_GIT_REVISION_INFO
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
done) > $HEADERPATH/git-revisions.txt 2>&1

# Repo manifest that can be used to fetch the sources for re-extracting headers
if [ -e $ANDROID_ROOT/.repo/manifest.xml ]; then
    cp $ANDROID_ROOT/.repo/manifest.xml $HEADERPATH/
fi

# Add a makefile to make packaging easier
cat > ${HEADERPATH}/Makefile << EOF
PREFIX?=/usr/local
INCLUDEDIR?=\$(PREFIX)/include/android
all:
	@echo "Use '\$(MAKE) install' to install"

install:
	mkdir -p \$(DESTDIR)/\$(INCLUDEDIR)
	cp android-config.h android-version.h android-headers.pc \$(DESTDIR)/\$(INCLUDEDIR)
	sed -i -e s:prefix=/usr:prefix=\$(PREFIX):g \$(DESTDIR)/\$(INCLUDEDIR)/android-headers.pc
	cp -r hardware \$(DESTDIR)/\$(INCLUDEDIR)
	cp -r hardware_legacy \$(DESTDIR)/\$(INCLUDEDIR)
	cp -r cutils \$(DESTDIR)/\$(INCLUDEDIR)
	cp -r system \$(DESTDIR)/\$(INCLUDEDIR)
	cp -r android \$(DESTDIR)/\$(INCLUDEDIR)
	cp -r linux \$(DESTDIR)/\$(INCLUDEDIR)
	cp -r sync \$(DESTDIR)/\$(INCLUDEDIR)
	cp -r libnfc-nxp \$(DESTDIR)/\$(INCLUDEDIR)
	cp -r private \$(DESTDIR)/\$(INCLUDEDIR)
EOF

find "$HEADERPATH" -type f -exec chmod 0644 {} \;

exit 0
# vim: noai:ts=4:sw=4:ss=4:expandtab
