#!/bin/sh -e
# Generate C prototype definitions for Android libnfc
# Copyright (C) 2013 Jolla Ltd.
# Contact: Thomas Perl <thomas.perl@jollamobile.com>

mkdir -p output

cproto -x \
    android_platform_external_libnfc-nxp/src/*.c \
    -I android_platform_external_libnfc-nxp/inc \
    -I android_platform_external_libnfc-nxp/src \
    -I android_platform_hardware_libhardware/include \
    -I android_platform_system_core/include \
    -I android_platform_frameworks_native/include \
    > output/libnfc_prototypes.h

python generate_wrappers.py \
    output/libnfc_prototypes.h \
    symbols/libnfc.so.txt \
    /system/lib/libnfc.so \
    > output/libnfc.c

python generate_wrappers.py \
    output/libnfc_prototypes.h \
    symbols/libnfc_ndef.so.txt \
    /system/lib/libnfc_ndef.so \
    > output/libnfc_ndef.c

