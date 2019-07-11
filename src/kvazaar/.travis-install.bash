#!/bin/bash

# Download FFmpeg and HM decoder and place them in $PATH.

set -euvo pipefail

mkdir -p "${HOME}/bin"

wget http://ultravideo.cs.tut.fi/ffmpeg-release-32bit-static.tar.xz
sha256sum -c - << EOF
4d3302ba0415e08ca10ca578dcd1f0acc48fadc9b803718283c8c670350c903e  ffmpeg-release-32bit-static.tar.xz
EOF
tar xf ffmpeg-release-32bit-static.tar.xz
cp ffmpeg-2.6.3-32bit-static/ffmpeg "${HOME}/bin/ffmpeg"
chmod +x "${HOME}/bin/ffmpeg"

wget http://ultravideo.cs.tut.fi/ubuntu-12.04-hmdec-16.10.tgz
sha256sum -c - << EOF
e00d61dd031a14aab1a03c0b23df315b8f6ec3fab66a0e2ae2162496153ccf92  ubuntu-12.04-hmdec-16.10.tgz
EOF
tar xf ubuntu-12.04-hmdec-16.10.tgz
cp hmdec-16.10 "${HOME}/bin/TAppDecoderStatic"
chmod +x "${HOME}/bin/TAppDecoderStatic"

export PATH="${HOME}/bin:${PATH}"
