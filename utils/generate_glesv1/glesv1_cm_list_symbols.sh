#!/bin/sh
# List symbols in shared object

LIBGLESV1_CM=/system/lib/libGLESv1_CM.so

objdump -T $LIBGLESV1_CM | grep 'DF .text' | awk '{ print $6 }'

