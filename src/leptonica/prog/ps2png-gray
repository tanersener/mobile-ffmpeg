#!/bin/bash
#   ps2png-gray
#
#     Rasterizes a postscript file, saving as a set of 8 bpp png images
#
#     input:  PostScript file
#             root name of output files
#     output: 8 bpp png files for each page
#
#   N.B. Requires ghostscript

scriptname=${0##*/}

if test $# != 2
then
  echo "usage: " $scriptname " inpsfile outpngroot"
  exit -1
fi

inpsfile=$1
outpngroot=$2

# need mysterious "primer"
# choose one of the two options below

# output image size depending on resolution
echo "0 neg 0 neg" translate | gs -sDEVICE=pnggray -sOutputFile=${outpngroot}%03d.png -r300x300 -q - ${inpsfile}

# output fixed image size
#echo "0 neg 0 neg" translate | gs -sDEVICE=pnggray -sOutputFile=${outpngroot}%03d.png -g2550x3300 -r300x300 -q - ${inpsfile}



