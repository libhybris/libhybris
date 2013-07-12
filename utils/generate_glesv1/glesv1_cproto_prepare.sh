#!/bin/sh
#
# Extract / preprocess and format function prototypes
# Dependencies: cproto
#

cproto -I ../../hybris/include/ -x glesv1_cproto_template.h >glesv1_functions.h

