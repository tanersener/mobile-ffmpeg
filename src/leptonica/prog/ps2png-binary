#!/bin/bash
#   ps2png-binary
#
#     Rasterizes a postscript file, saving as a set of binary png images
#
#     input:  PostScript file
#             root name of output files
#     output: png binary files for each page
#
#     Restriction: the input PostScript file must be binary
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
echo "0 neg 0 neg" translate | gs -sDEVICE=pngmono -sOutputFile=${outpngroot}%03d.png -r300x300 -q - ${inpsfile}

# output fixed image size
#echo "0 neg 0 neg" translate | gs -sDEVICE=pngmono -sOutputFile=${outpngroot}%03d.png -g2550x3300 -r300x300 -q - ${inpsfile}



