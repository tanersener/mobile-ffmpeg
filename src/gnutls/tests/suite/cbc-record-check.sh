#!/bin/bash

ecode=0

FAILED=""

if test -z "${OPTIONS}";then
OPTIONS="sha1 sha256 sha384 sha256-new sha1-one"
fi

echo "Running for: $OPTIONS"

TIME=ns

function plot()
{
	hash=$1
	R --no-save <<_EOF_
#plot
require(lattice)

z=read.csv("out-$hash.txt", header=TRUE);

options(scipen=999)

m <- matrix(ncol=3,nrow=256)

png(filename = "$hash-density.png",width=3072,height=4096,units="px",bg="white");
d <- density(unlist(z[1,]));
plot(d, ylab="Delta", xlab="${TIME}", col="red", main=sprintf("estimated PDF for %d", 1));
for (row in c(4,72,253,256)){
  lines(density(unlist(z[row,])), col = sample(colours(), 5));
}
dev.off();

colnames(m) <- c("Delta","TimeMedian","TimeAverage")
for (row in 1:256){
  v=tail(z[row,], length(z[row,])-1)
  m[row,][1] = unlist(z[row,][1]);
  m[row,][2] = median(unlist(v));
  m[row,][3] = mean(unlist(v));
}

png(filename = "$hash-timings-med.png",width=1024,height=1024,units="px",bg="white");
plot(m[,1],m[,2],xlab="Delta",ylab="Median timings (${TIME})");
dev.off();

png(filename = "$hash-timings-ave.png",width=1024,height=1024,units="px",bg="white");
plot(m[,1],m[,3],xlab="Delta",ylab="Average timings (${TIME})");
dev.off();

estatus=0;
for (row in 2:256){
  v=ks.test(unlist(z[1,]),unlist(z[row,]));
  p = 0.05/255
  if (v[2] < p) {
    print (row);
    print (v[2]);
    estatus=1;
  }
}
q(status=estatus);
_EOF_

if test $? != 0;then
	echo "*** Failed test for $hash"
	FAILED="${FAILED} $hash"
	ecode=1
fi
}

for hash in ${OPTIONS};do
	if ! test -f out-$hash.txt;then
		./mini-record-timing $hash
	fi
	plot $hash
done

echo "Failed: ${FAILED}"

exit $ecode
