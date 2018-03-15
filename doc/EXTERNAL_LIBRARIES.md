This project contains source code of FFmpeg and the following libraries.

#### External Libraries

These libraries are supported by FFmpeg and needs to be compiled before FFmpeg.
Below you can find their versions and dependencies.

- fontconfig v2.12.93
    - depends libuuid, libxml2, libiconv, freetype
- freetype v2.9
    - depends libpng
- fribidi v1.0.1
- gmp v6.1.2
- gnutls v3.5.18
    - depends nettle, gmp, libiconv
- libiconv v1.15
- lame v3.100
    - depends libiconv
- libass v0.14.0
    - depends freetype, fribidi, fontconfig, libiconv, libuuid, libxml2
- libtheora v1.1.1
    - depends libogg, libvorbis
- libvorbis v1.3.5
    - depends libogg
- libvpx v1.7.0
- libwebp v0.6.1
    - depends giflib, libjpeg, libpng, tifflib
- libxml2 v2.9.7
    - depends libiconv
- opencore-amr v0.1.5
- shine v3.1.1
- speex v1.2.0
- wavpack v5.1.0

#### Supplementary Packages for External Libraries

Supplementary packages are libraries required to compile external libraries.

- giflib v5.1.4
- libjpeg v9c
- libogg v1.3.3
- libpng v1.6.34
- libuuid v1.0.3
- nettle v3.4
    - depends gmp
- tiff v4.0.9
    - depends libjpeg