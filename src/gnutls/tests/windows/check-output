#!/bin/sh

TMPFILE=wine.$$.out

wine $* >$TMPFILE

grep -i "Unhandled exception" $TMPFILE
if test $? = 0;then
	ret=1
else
	ret=0
fi

echo $TMPFILE
rm -f $TMPFILE

exit $ret
