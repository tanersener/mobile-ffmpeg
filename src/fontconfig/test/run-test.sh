#!/bin/sh
# fontconfig/test/run-test.sh
#
# Copyright © 2000 Keith Packard
#
# Permission to use, copy, modify, distribute, and sell this software and its
# documentation for any purpose is hereby granted without fee, provided that
# the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation, and that the name of the author(s) not be used in
# advertising or publicity pertaining to distribution of the software without
# specific, written prior permission.  The authors make no
# representations about the suitability of this software for any purpose.  It
# is provided "as is" without express or implied warranty.
#
# THE AUTHOR(S) DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
# EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY SPECIAL, INDIRECT OR
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
# DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
# TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
case "$OSTYPE" in
    msys ) MyPWD=`pwd -W` ;;  # On Msys/MinGW, returns a MS Windows style path.
    *    ) MyPWD=`pwd`    ;;  # On any other platforms, returns a Unix style path.
esac

TESTDIR=${srcdir-"$MyPWD"}

FONTDIR="$MyPWD"/fonts
CACHEDIR="$MyPWD"/cache.dir
EXPECTED=${EXPECTED-"out.expected"}

ECHO=true

FCLIST=../fc-list/fc-list$EXEEXT
FCCACHE=../fc-cache/fc-cache$EXEEXT

which bwrap > /dev/null 2>&1
if [ $? -eq 0 ]; then
    BWRAP=`which bwrap`
fi

FONT1=$TESTDIR/4x6.pcf
FONT2=$TESTDIR/8x16.pcf

check () {
  $FCLIST - family pixelsize | sort > out
  echo "=" >> out
  $FCLIST - family pixelsize | sort >> out
  echo "=" >> out
  $FCLIST - family pixelsize | sort >> out
  tr -d '\015' <out >out.tmp; mv out.tmp out
  if cmp out $TESTDIR/$EXPECTED > /dev/null ; then : ; else
    echo "*** Test failed: $TEST"
    echo "*** output is in 'out', expected output in '$EXPECTED'"
    exit 1
  fi
  rm out
}

prep() {
  rm -rf $CACHEDIR
  rm -rf $FONTDIR
  mkdir $FONTDIR
}

dotest () {
  TEST=$1
  test x$VERBOSE = x || echo Running: $TEST
}

sed "s!@FONTDIR@!$FONTDIR!
s!@CACHEDIR@!$CACHEDIR!" < $TESTDIR/fonts.conf.in > fonts.conf

FONTCONFIG_FILE="$MyPWD"/fonts.conf
export FONTCONFIG_FILE

dotest "Basic check"
prep
cp $FONT1 $FONT2 $FONTDIR
check

dotest "With a subdir"
prep
cp $FONT1 $FONT2 $FONTDIR
$FCCACHE $FONTDIR
check

dotest "Subdir with a cache file"
prep
mkdir $FONTDIR/a
cp $FONT1 $FONT2 $FONTDIR/a
$FCCACHE $FONTDIR/a
check

dotest "Complicated directory structure"
prep
mkdir $FONTDIR/a
mkdir $FONTDIR/a/a
mkdir $FONTDIR/b
mkdir $FONTDIR/b/a
cp $FONT1 $FONTDIR/a
cp $FONT2 $FONTDIR/b/a
check

dotest "Subdir with an out-of-date cache file"
prep
mkdir $FONTDIR/a
$FCCACHE $FONTDIR/a
sleep 1
cp $FONT1 $FONT2 $FONTDIR/a
check

dotest "Dir with an out-of-date cache file"
prep
cp $FONT1 $FONTDIR
$FCCACHE $FONTDIR
sleep 1
mkdir $FONTDIR/a
cp $FONT2 $FONTDIR/a
check

dotest "Re-creating .uuid"
prep
cp $FONT1 $FONTDIR
$FCCACHE $FONTDIR
cat $FONTDIR/.uuid > out1
$FCCACHE -f $FONTDIR
cat $FONTDIR/.uuid > out2
if cmp out1 out2 > /dev/null ; then : ; else
  echo "*** Test failed: $TEST"
  echo "*** .uuid was modified unexpectedly"
  exit 1
fi
$FCCACHE -r $FONTDIR
cat $FONTDIR/.uuid > out2
if cmp out1 out2 > /dev/null ; then
  echo "*** Test failed: $TEST"
  echo "*** .uuid wasn't modified"
  exit 1
fi
rm out1 out2

dotest "Consistency between .uuid and cache name"
prep
cp $FONT1 $FONTDIR
$FCCACHE $FONTDIR
cat $FONTDIR/.uuid
$FCCACHE -r $FONTDIR
uuid=`cat $FONTDIR/.uuid`
ls $CACHEDIR/$uuid*
if [ $? != 0 ]; then
  echo "*** Test failed: $TEST"
  echo "No cache for $uuid"
  ls $CACHEDIR
  exit 1
fi
n=`ls -1 $CACHEDIR/*cache-* | wc -l`
if [ $n != 1 ]; then
  echo "*** Test failed: $TEST"
  echo "Unexpected cache was created"
  ls $CACHEDIR
  exit 1
fi

dotest "Keep mtime of the font directory"
prep
cp $FONT1 $FONTDIR
touch -d @0 $FONTDIR
stat $FONTDIR | grep Modify > out1
$FCCACHE $FONTDIR
stat $FONTDIR | grep Modify > out2
if cmp out1 out2 > /dev/null ; then : ; else
    echo "*** Test failed: $TEST"
    echo "mtime was modified"
    exit 1
fi

if [ x"$BWRAP" != "x" ]; then
dotest "Basic functionality with the bind-mounted cache dir"
prep
cp $FONT1 $FONT2 $FONTDIR
$FCCACHE $FONTDIR
sleep 1
ls -l $CACHEDIR > out1
TESTTMPDIR=`mktemp -d /tmp/fontconfig.XXXXXXXX`
sed "s!@FONTDIR@!$TESTTMPDIR/fonts!
s!@CACHEDIR@!$TESTTMPDIR/cache.dir!" < $TESTDIR/fonts.conf.in > bind-fonts.conf
$BWRAP --bind / / --bind $CACHEDIR $TESTTMPDIR/cache.dir --bind $FONTDIR $TESTTMPDIR/fonts --bind .. $TESTTMPDIR/build --dev-bind /dev /dev --setenv FONTCONFIG_FILE $TESTTMPDIR/build/test/bind-fonts.conf $TESTTMPDIR/build/fc-match/fc-match$EXEEXT -f "%{file}\n" ":foundry=Misc" > xxx
ls -l $CACHEDIR > out2
if cmp out1 out2 > /dev/null ; then : ; else
  echo "*** Test failed: $TEST"
  echo "cache was updated."
  exit 1
fi
if [ x`cat xxx` != "x$TESTTMPDIR/fonts/4x6.pcf" ]; then
  echo "*** Test failed: $TEST"
  echo "file property doesn't points to the new place: $TESTTMPDIR/fonts/4x6.pcf"
  exit 1
fi
rm -rf $TESTTMPDIR out1 out2 xxx bind-fonts.conf
fi

rm -rf $FONTDIR $CACHEFILE $CACHEDIR $FONTCONFIG_FILE out
