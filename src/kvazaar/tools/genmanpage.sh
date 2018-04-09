#!/bin/sh

LANG=C
set -e

cd "$(dirname "$0")"

date="$(date +"%B %Y")"
version="$(awk '/#define KVZ_VERSION/ { print $3 }' ../src/global.h)"
manpage_file=../doc/kvazaar.1

cat <<EOF> $manpage_file
.TH KVAZAAR "1" "$date" "kvazaar v$version" "User Commands"
.SH NAME
kvazaar \- open source HEVC encoder
.SH SYNOPSIS
\fBkvazaar \fR\-i <input> \-\-input\-res <width>x<height> \-o <output>
.SH DESCRIPTION
EOF

../src/kvazaar --help 2>&1 | tail -n+5 | head -n-4 | \
  sed 's| : |\n|g;
       s| :$||g;
       s|^      --|.TP\n\\fB--|g;
       s|^  -|.TP\n\\fB-|g;
       s|^                               ||g;
       s|-|\\-|g;
       s|, \\-\\-|\\fR, \\fB\\-\\-|g;' \
  >> $manpage_file

for s in Required Presets Input Options "Video structure" "Compression tools" "Parallel processing" "Video Usability Information"; do
  sed -i "s|^${s}:|.SS \"${s}:\"|g" $manpage_file
done
