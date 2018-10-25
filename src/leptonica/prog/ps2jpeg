#!/bin/bash
#   ps2jpeg
#
#     Rasterizes a postscript file, saving as a set of RGB images
#
#     input:  PostScript file
#             root name of output files
#     output: 24 bpp RGB jpeg files for each page
#
#   N.B. Requires ghostscript

scriptname=${0##*/}

if test $# != 2
then
  echo "usage: " $scriptname " inpsfile outjpgroot"
  exit -1
fi

inpsfile=$1
outjpgroot=$2

# (need mysterious "primer")
# choose one of the two options below

# output image size depending on resolution
echo "0 neg 0 neg" translate | /usr/local/bin/gs -sDEVICE=jpeg -sOutputFile=${outjpgroot}%03d.jpg -r300x300 -q - ${inpsfile}

# output fixed image size
#echo "0 neg 0 neg" translate | gs -sDEVICE=jpeg -sOutputFile=${outjpgroot}%03d.jpg -g2550x3300 -r300x300 -q - ${inpsfile}


