#!/bin/bash
#
# Generates javadoc for Android library
#

CURRENT_DIR="`pwd`"

gradle -b ${CURRENT_DIR}/../android/app/build.gradle clean build javadoc
