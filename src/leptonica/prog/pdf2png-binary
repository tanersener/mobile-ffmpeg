#!/bin/bash
#   pdf2png-binary
#
#     Rasterizes a PDF file, saving as a set of binary png images
#
#     input:  PDF
#             root name of output files
#     output: 1 bpp png files for each page
#
#  Note 1:  Requires ghostscript
#
#  Note 2:  A modern alternative to ghostcript is to use poplar:
#    If the pdf is composed of images that were orthographically generated:
#          pdftoppm -png <pdf-file> <pdf-root>    [output in png]
#    If the pdf is composed of images that were scanned:
#          pdftoppm -jpeg <pdf-file> <pdf-root>   [output in jpeg]

scriptname=${0##*/}

if test $# != 2
then
  echo "usage: " $scriptname " inpdffile outpngroot"
  exit -1
fi

inpdffile=$1
outpngroot=$2

# need mysterious "primer"
# choose one of the two options below

# output image size depending on resolution
echo "0 neg 0 neg" translate | gs -sDEVICE=pngmono -sOutputFile=${outpngroot}%03d.png -r300x300 -q - ${inpdffile}

# output fixed image size
#echo "0 neg 0 neg" translate | gs -sDEVICE=pngmono -sOutputFile=${outpngroot}%03d.png -g2550x3300 -r300x300 -q - ${inpdffile}


