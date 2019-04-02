#!/bin/bash
#   pdf2jpeg
#
#     Rasterizes a PDF file, saving as a set of 24 bpp RGB images
#
#     input:  PDF
#             root name of output files
#     output: 24 bpp RGB jpeg files for each page
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
  echo "usage: " $scriptname " inpdffile outjpgroot"
  exit -1
fi

inpdffile=$1
outjpgroot=$2

# strip off directory and suffix parts of $1 to use in other names
basename=${1##*/}
baseroot=${basename%.*}

# make names for temporary files
tmppdffile=${baseroot}.$$_.pdf
tmppdfroot=${tmppdffile%.*}

# have the temporary files deleted on exit, interrupt, etc:
trap "/bin/rm -f ${tmppdfroot}*" EXIT SIGHUP SIGINT SIGTERM

cp $inpdffile $tmppdffile

# need mysterious "primer"
# choose one of the two options below

#--------------------------------------------------------------------#
#               output image size depending on resolution            #
#--------------------------------------------------------------------#
echo "0 neg 0 neg" translate | gs -sDEVICE=jpeg -sOutputFile=${outjpgroot}%03d.jpg -r300x300 -q - ${tmppdffile}
#echo "0 neg 0 neg" translate | gs -sDEVICE=jpeg -sOutputFile=${outjpgroot}%03d.jpg -r150x150 -q - ${tmppdffile}
#echo "0 neg 0 neg" translate | gs -sDEVICE=jpeg -sOutputFile=${outjpgroot}%03d.jpg -r75x75 -q - ${tmppdffile}


#--------------------------------------------------------------------#
#                       output fixed image size                      #
#--------------------------------------------------------------------#
#echo "0 neg 0 neg" translate | gs -sDEVICE=jpeg -sOutputFile=${outjpgroot}%03d.jpg -g2550x3300 -r300x300 -q - ${tmppdffile}


