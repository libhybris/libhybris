#/bin/sh

ANDROID_ROOT=$1
HEADERPATH=$2
MAJOR=$3
MINOR=$4
PATCH=$5
PATCH2=$6
PATCH3=$7

if [ x$ANDROID_ROOT = x -o "x$HEADERPATH" = x -o x$MAJOR = x -o x$MINOR = x ]; then
    echo "Syntax: extract-headers.sh ANDROID_ROOT HEADERPATH MAJOR MINOR PATCH [PATCH2 PATCH3]"
    exit -1
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

mkdir -p $HEADERPATH/cutils/
cp $ANDROID_ROOT/system/core/include/cutils/* $HEADERPATH/cutils/

mkdir -p $HEADERPATH/system/
cp $ANDROID_ROOT/system/core/include/system/* $HEADERPATH/system/

if [ -e $ANDROID_ROOT/external/kernel-headers/original/linux/sync.h ]; then
	mkdir -p $HEADERPATH/linux
	cp $ANDROID_ROOT/external/kernel-headers/original/linux/sync.h $HEADERPATH/linux
	cp $ANDROID_ROOT/external/kernel-headers/original/linux/sw_sync.h $HEADERPATH/linux
fi

if [ -e $ANDROID_ROOT/system/core/include/sync/sync.h ]; then
	mkdir -p $HEADERPATH/sync/
	cp $ANDROID_ROOT/system/core/include/sync/* $HEADERPATH/sync/
fi

mkdir -p $HEADERPATH/private
cp $ANDROID_ROOT/system/core/include/private/android_filesystem_config.h $HEADERPATH/private

exit 0
