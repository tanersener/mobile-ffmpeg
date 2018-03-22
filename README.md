# mobile-ffmpeg
Source code & scripts to build FFmpeg for Android and IOS platform

### Features
- Supports both Android and IOS
- Can be customized to support a specific platform and/or library
- Licensed under LGPL 3.0

### Architectures
#### Android
- arm-v7a
- arm-v7a-neon
- arm64-v8a
- x86
- x86_64

#### IOS
- arm-v7
- arm-v7s
- arm64-v8a
- x86_64

### FFmpeg Support
This repository contains FFmpeg version 3.4.2 with support for the following external libraries.

- fontconfig
- freetype
- fribidi
- gmp
- gnutls
- libiconv
- lame
- libass
- libtheora
- libvorbis
- libvpx
- libwebp
- libxml2
- opencore-amr
- shine
- speex
- wavpack

Supported libraries and their dependencies are explained in the [Supported Libraries](https://github.com/tanersener/mobile-ffmpeg/wiki/Supported-Libraries) wiki page.

### Prerequisites

- gcc required by freetype
- cmake v3.9.x or later required by libwebp
- gperf required by fontconfig
- pkg-config required by freetype and ffmpeg

#### Android

Android NDK is required to build Android platform

#### IOS

XCode is required to build IOS platform

### Usage

Use `android.sh` to build FFmpeg for Android. Visit [android.sh](https://github.com/tanersener/mobile-ffmpeg/wiki/android.sh) wiki page for all build options.

### LICENSE

Files provided by this project are licensed under the LGPL v3.0.
Source code of external libraries, FFmpeg and other supported libraries, is included with their individual licenses.

Visit [License](https://github.com/tanersener/mobile-ffmpeg/wiki/License) wiki page for the details.

### See Also

- [FFmpeg License and Legal Considerations](https://ffmpeg.org/legal.html)
