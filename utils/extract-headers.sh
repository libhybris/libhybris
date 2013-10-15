#! /bin/sh

ANDROID_ROOT=$1
HEADERPATH=$2
MAJOR=$3
MINOR=$4

PATCH=$5
PATCH2=$6
PATCH3=$7

if [ x$ANDROID_ROOT = x -o "x$HEADERPATH" = x ]; then
    echo "Syntax: extract-headers.sh ANDROID_ROOT HEADERPATH [MAJOR] [MINOR] [PATCH] [PATCH2] [PATCH3]"
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

    PLATFORM_VERSION=$(egrep -o "PLATFORM_VERSION := [0-9.]+" $VERSION_DEFAULTS | awk '{ print $3 }')
    if [ x$PLATFORM_VERSION = x ]; then
        parse_defaults_failed
    fi

    MAJOR=$(echo $PLATFORM_VERSION | cut -d. -f1)
    MINOR=$(echo $PLATFORM_VERSION | cut -d. -f2)
    PATCH=$(echo $PLATFORM_VERSION | cut -d. -f3)
    PATCH2=$(echo $PLATFORM_VERSION | cut -d. -f4)
    PATCH3=$(echo $PLATFORM_VERSION | cut -d. -f5)

    if [ x$MAJOR = x -o x$MINOR = x -o x$PATCH = x ]; then
        parse_defaults_failed
    fi

    echo -n "Auto-detected version: ${MAJOR}.${MINOR}.${PATCH}"
    if [ x$PATCH3 != x ]; then
        echo ".${PATCH2}.${PATCH3}"
    elif [ x$PATCH2 != x ]; then
        echo ".${PATCH2}"
    else
        echo ""
    fi
fi

# Make sure that the dir given contains at least some of the assumed structures.
if [ ! -d "$ANDROID_ROOT/hardware/libhardware/include/hardware/" ]; then
    echo "Given Android root dir '$ANDROID_ROOT/hardware/libhardware/include/hardware/' doesn't exist."
    exit 1
fi

mkdir -p $HEADERPATH

if [ x$PATCH2 = x ]; then
    PATCH2=0
fi

if [ x$PATCH3 = x ]; then
    PATCH3=0
fi

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

mkdir -p $HEADERPATH/hardware/
cp $ANDROID_ROOT/hardware/libhardware/include/hardware/* $HEADERPATH/hardware/

mkdir -p $HEADERPATH/hardware_legacy/
cp $ANDROID_ROOT/hardware/libhardware_legacy/include/hardware_legacy/audio_policy_conf.h $HEADERPATH/hardware_legacy/

mkdir -p $HEADERPATH/cutils/
cp $ANDROID_ROOT/system/core/include/cutils/* $HEADERPATH/cutils/

mkdir -p $HEADERPATH/system/
cp $ANDROID_ROOT/system/core/include/system/* $HEADERPATH/system/

mkdir -p $HEADERPATH/android/
cp $ANDROID_ROOT/system/core/include/android/* $HEADERPATH/android/

if [ -e $ANDROID_ROOT/external/kernel-headers/original/linux/sync.h ]; then
	mkdir -p $HEADERPATH/linux
	cp $ANDROID_ROOT/external/kernel-headers/original/linux/sync.h $HEADERPATH/linux
	cp $ANDROID_ROOT/external/kernel-headers/original/linux/sw_sync.h $HEADERPATH/linux
fi

if [ -e $ANDROID_ROOT/system/core/include/sync/sync.h ]; then
	mkdir -p $HEADERPATH/sync/
	cp $ANDROID_ROOT/system/core/include/sync/* $HEADERPATH/sync/
fi

if [ -e $ANDROID_ROOT/external/libnfc-nxp/inc/phNfcConfig.h ]; then
	mkdir -p $HEADERPATH/libnfc-nxp/
	cp $ANDROID_ROOT/external/libnfc-nxp/inc/*.h $HEADERPATH/libnfc-nxp/
	cp $ANDROID_ROOT/external/libnfc-nxp/src/*.h $HEADERPATH/libnfc-nxp/
fi

mkdir -p $HEADERPATH/private
cp $ANDROID_ROOT/system/core/include/private/android_filesystem_config.h $HEADERPATH/private

exit 0
