---
layout: default
---
MobileFFmpeg aims to provide FFmpeg on both mobile platforms with support for shared libraries including LGPL and GPL building options.
### 1. Features
- Includes build scripts and prebuilt libraries for both Android and IOS
- Supports 21 external libraries, 2 GPL libraries and 10 architectures in total
- Exposes FFmpeg capabilities both directly from FFmpeg libraries and through MobileFFmpeg wrapper library
- Different branches for FFmpeg v3.4.2 and FFmpeg v4.0.1
- Includes cross-compiling instructions for 32 open-source libraries
- Creates Android archive with .aar extension
- Creates IOS dynamic universal (fat) library
- Creates IOS dynamic framework for IOS 8 or later
- Licensed under LGPL 3.0, can be customized to support GPL v3.0

### 2. Architectures
- Android arm-v7a
- Android arm-v7a-neon
- Android arm64-v8a
- Android x86
- Android x86_64
- IOS armv7
- IOS armv7s
- IOS arm64
- IOS i386
- IOS x86_64

### 3. FFmpeg Support
Latest release (v2.0) of this project contains FFmpeg version 4.0.x with support for the following external libraries.
- fontconfig
- freetype
- fribidi
- gmp
- gnutls
- kvazaar
- lame
- libass
- libiconv
- libilbc
- libtheora
- libvorbis
- libvpx
- libwebp
- libxml2
- opencore-amr
- opus
- shine
- snappy
- speex
- wavpack
- x264
- xvidcore

### 4. Using
#### 4.1 Android
Import `mobile-ffmpeg-2.0.aar` into your project and use the following source code.
```
    import com.arthenica.mobileffmpeg.FFmpeg;

    int rc = FFmpeg.execute("-i", "file1.mp4", "-c:v", "libxvid", "file1.avi");
    Log.i(Log.TAG, String.format("Command execution %s.", (rc == 0?"completed successfully":"failed with rc=" + rc));
```
#### 4.2 IOS
Use MobileFFmpeg in your project by adding all IOS frameworks from `prebuilt/ios-framework` path or 
by adding all `ffmpeg` and `mobile-ffmpeg` dylibs with headers from folders inside `prebuilt/ios-universal`.

Then run the following Objective-C source code.
```
    #import <mobileffmpeg/mobileffmpeg.h>

    NSString* command = @"-i file1.mp4 -c:v libxvid file1.avi";
    NSArray* commandArray = [command componentsSeparatedByString:@" "];
    char **arguments = (char **)malloc(sizeof(char*) * ([commandArray count]));
    for (int i=0; i < [commandArray count]; i++) {
        NSString *argument = [commandArray objectAtIndex:i];
        arguments[i] = (char *) [argument UTF8String];
    }

    int result = mobileffmpeg_execute((int) [commandArray count], arguments);

    NSLog(@"Process exited with rc %d\n", result);
    
    free(arguments);
```

### 5. Building
#### 5.1 Prerequisites
1. Use your package manager (apt, yum, dnf, brew, etc.) to install the following packages.

    ```
    autoconf automake libtool pkg-config curl cmake gcc gperf texinfo yasm nasm
    ```
2. Android builds require these additional packages.

    - **Android SDK 5.0 Lollipop (API Level 21)** or later
    - **Android NDK r16b** or later with LLDB and CMake
    - **gradle 4.4** or later

3. IOS builds need these extra packages and tools.
    - **IOS SDK 7.0.x** or later
    - **Xcode 8.x** or later 
    - **Command Line Tools**
    - **lipo** utility

#### 5.2 Build Scripts
Use `android.sh` and `ios.sh` to build MobileFFmpeg for each platform.
After a successful build, compiled FFmpeg and MobileFFmpeg libraries can be found under `prebuilt` directory.

##### 5.2.1 Android
```
export ANDROID_NDK_ROOT=<Android NDK Path>
./android.sh
```
##### 5.2.2 IOS
```
./ios.sh
```
#### 5.3 GPL Support
From `v1.1` onwards it is possible to enable to GPL libraries `x264` and `xvidcore` from top level build scripts.
Their source code is not included in the repository and downloaded when enabled.

### 6. Documentation

A more detailed documentation is available at [Wiki](https://github.com/tanersener/mobile-ffmpeg/wiki).

### 7. License

This project is licensed under the LGPL v3.0. However, if source code is built using optional `--enable-gpl` flag or 
prebuilt binaries with `-gpl` postfix are used then MobileFFmpeg is subject to the GPL v3.0 license.

Source code of FFmpeg and external libraries is included in compliance with their individual licenses.

Digital assets used in test applications are published in the public domain.

Please visit [License](https://github.com/tanersener/mobile-ffmpeg/wiki/License) page for the details.

### 8. See Also

- [FFmpeg API Documentation](https://ffmpeg.org/doxygen/3.4/index.html)
- [FFmpeg License and Legal Considerations](https://ffmpeg.org/legal.html)
