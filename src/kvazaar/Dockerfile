# A simple Dockerfile for building Kvazaar from the git repository
# Example build command when in this directory: docker build -t kvazaar .
#
# Example usage
# Run with an input YUV file and output HEVC binary file
#     docker run -i -a STDIN -a STDOUT kvazaar -i - --input-res=320x240 -o - < testfile_320x240.yuv > out.265
#
# Use libav or ffmpeg to input (almost) any format and convert it to YUV420 for kvazaar, audio is disabled
#
#     RESOLUTION=`avconv -i input.avi 2>&1 | grep Stream | grep -oP ', \K[0-9]+x[0-9]+'`
#     avconv -i input.avi -an -f rawvideo -pix_fmt yuv420p - | docker run -i -a STDIN -a STDOUT kvazaar -i - --wpp --threads=8 --input-res=$RESOLUTION --preset=ultrafast -o - > output.265
#  or
#     RESOLUTION=`ffmpeg -i input.avi 2>&1 | grep Stream | grep -oP ', \K[0-9]+x[0-9]+'`
#     ffmpeg -i input.avi -an -f rawvideo -pix_fmt yuv420p - | docker run -i -a STDIN -a STDOUT kvazaar -i - --wpp --threads=8 --input-res=$RESOLUTION --preset=ultrafast -o - > output.265
#

# Use Ubuntu 18.04 as a base for now, it's around 88MB
FROM ubuntu:18.04

MAINTAINER Marko Viitanen <fador@iki.fi>

# List of needed packages to be able to build kvazaar with autotools
ENV REQUIRED_PACKAGES automake autoconf libtool m4 build-essential git yasm pkgconf

ADD . kvazaar
# Run all the commands in one RUN so we don't have any extra history
# data in the image.
RUN apt-get update \
    && apt-get install -y $REQUIRED_PACKAGES \
    && apt-get clean \
    && cd kvazaar \
    && ./autogen.sh \
    && ./configure --disable-shared \
    && make\
    && make install \
    && AUTOINSTALLED_PACKAGES=`apt-mark showauto` \
    && apt-get remove --purge --force-yes -y $REQUIRED_PACKAGES $AUTOINSTALLED_PACKAGES \
    && apt-get clean autoclean \
    && apt-get autoremove -y \
    && rm -rf /var/lib/{apt,dpkg,cache,log}/

ENTRYPOINT ["kvazaar"]
CMD ["--help"]
