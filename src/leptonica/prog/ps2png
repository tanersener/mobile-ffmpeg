#!/bin/bash
#   ps2png
#
#     Rasterizes a postscript file, saving as a set of bitmaps
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

# strip off directory and suffix parts of $1 to use in other names
basename=${1##*/}
baseroot=${basename%.*}

# make names for temporary files
tmppsfile=${baseroot}.$$_.ps
tmppsroot=${tmppsfile%.*}

# have the temporary files deleted on exit, interrupt, etc:
trap "/bin/rm -f ${tmppsroot}*" EXIT SIGHUP SIGINT SIGTERM

cp $inpsfile $tmppsfile

# need mysterious "primer"
echo "0 neg 0 neg" translate | gs -sDEVICE=pngmono -sOutputFile=${outpngroot}%03d.png -g2550x3300 -r300x300 -q - ${tmppsfile}




