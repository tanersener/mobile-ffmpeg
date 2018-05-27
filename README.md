# MobileFFmpeg
Source code and scripts to build FFmpeg for Android and IOS platform

### 1. Features
- Builds both Android and IOS
- Supports 18 external libraries and 10 architectures in total
- Exposes FFmpeg capabilities both directly and through MobileFFmpeg wrapper library
- Creates shared libraries (.so for Android, .dylib for IOS)
- Licensed under LGPL 3.0
#### 1.1 Android
- Creates Android archive with .aar extension
#### 1.2 IOS
- Creates IOS dynamic universal (fat) library
- Creates IOS dynamic framework for IOS 8 or later
### 2. Architectures
#### 2.1 Android
- arm-v7a
- arm-v7a-neon
- arm64-v8a
- x86
- x86_64
#### 2.2 IOS
- armv7
- armv7s
- arm64
- i386
- x86_64
### 3. FFmpeg Support
This repository branch contains FFmpeg version 3.4.2 with support for the following external libraries.
- fontconfig
- freetype
- fribidi
- gmp
- gnutls
- kvazaar
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

External libraries and their dependencies are explained in the [External Libraries](https://github.com/tanersener/mobile-ffmpeg/wiki/External-Libraries) page.
### 4. Using
\* You can use prebuilt library binaries from the following locations
### 4.1 Android
### 4.2 IOS

### 5. Building
- pkg-config required by freetype and ffmpeg

- gcc required by freetype
- cmake v3.9.x or later required by libwebp
- gperf required by fontconfig
- yasm required for libvpx
- makeinfo (texinfo) required by gmp

#### 4.1 Android

Android NDK and gradle is required to build Android platform

#### 4.2 IOS

XCode is required to build IOS platform. Command Line Tools, go to https://developer.apple.com/download/more/ MacOS

autoconf automake libtool

- export PATH=${PATH}:/usr/local/opt/gettext/bin by gnutls
- curl, lipo for IOS

\* Use `android.sh` to build FFmpeg for Android. Visit [android.sh](https://github.com/tanersener/mobile-ffmpeg/wiki/android.sh) page for all build options.

### 6. API

\* MobileFFmpeg API defines

### 7. Known Issues

- On very few occasions, top level build scripts fail to build one of the external libraries randomly. When the details are analyzed it is seen that compilation fails due to not-existing cpu specific instructions.
Somehow cleaning the previous architecture build fails and this breaks the new compilation for that library. When the top level script is run again the error disappears.
It wasn't possible to find the exact cause of this issue since it happens rarely and disappears on the next run.

### 8. License

This project is licensed under the LGPL v3.0.

Source code of FFmpeg and external libraries is included in compliance with their individual licenses.

Digital assets used in test applications are published in the public domain.

Please visit [License](https://github.com/tanersener/mobile-ffmpeg/wiki/License) page for the details.

### 9. Contributing

This project is stable but far from complete. If you have any recommendations or ideas to improve it, please feel free to submit issues or pull requests. Any help is appreciated.

### 10 See Also

- [FFmpeg License and Legal Considerations](https://ffmpeg.org/legal.html)

**PS**: Asterisk (*) denotes ongoing work
