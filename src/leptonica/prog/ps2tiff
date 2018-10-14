#!/bin/bash
#   ps2tiff
#
#     Rasterizes a postscript file, saving as a set of g4 compressed
#     tiff images
#
#     input:  PostScript file
#             root name of output files
#     output: ccitt-g4 compressed tiff binary files for each page
#
#     Restriction: the input PostScript file must be binary
#
#   N.B. Requires ghostscript

scriptname=${0##*/}

if test $# != 2
then
  echo "usage: " $scriptname " inpsfile outtifroot"
  exit -1
fi

inpsfile=$1
outtifroot=$2

# assert (input postscript filename ends in .ps)
if test ${inpsfile##*.} != ps
then
  echo $scriptname ": " $inpsfile "does not end in .ps"
  exit -1
fi

# output image size depending on resolution
#echo "0 neg 0 neg" translate | gs -sDEVICE=tiffg4 -sOutputFile=${outtifroot}%03d.tif -r300x300 -q - ${inpsfile}
echo "0 neg 0 neg" translate | ~/pckg/gs/ghostscript-8.53/bin/gs -sDEVICE=tiffg4 -sOutputFile=${outtifroot}%03d.tif -r300x300 -q - ${inpsfile}

# output fixed image size
#echo "0 neg 0 neg" translate | gs -sDEVICE=tiffg4 -sOutputFile=${outtifroot}%03d.tif -g2550x3300 -r300x300 -q - ${inpsfile}

# use this to output to a single multipage tiff file
#tiffcp -c g4 ${outtifroot}*.tif ${outtifroot}.tif

