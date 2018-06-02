#!/bin/bash
#
# Generates docs for IOS library
#

CURRENT_DIR="`pwd`"

cd ${CURRENT_DIR}/../ios

doxygen
