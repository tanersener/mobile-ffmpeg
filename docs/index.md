---
layout: default
---
FFmpeg for Android and IOS

<img src="https://github.com/tanersener/mobile-ffmpeg/raw/dev-v2.x/docs/assets/mobile-ffmpeg-logo-v4.png" width="120">

### 1. Features
- Supports FFmpeg `v3.4.x` and `v4.0.x` releases
- Use prebuilt binaries available under `Github`/`JCenter`/`CocoaPods` or build your own version with external libraries you need
- Includes 24 external libraries, 4 GPL libraries and 10 architectures in total
- Exposes FFmpeg capabilities both directly from FFmpeg libraries and through MobileFFmpeg wrapper library
- Includes cross-compile instructions for 38 open-source libraries

   `chromaprint`, `expat`, `ffmpeg`, `fontconfig`, `freetype`, `fribidi`, `giflib`, `gmp`, `gnutls`, `kvazaar`, `lame`, `libaom`, `libass`, `libiconv`, `libilbc`, `libjpeg`, `libjpeg-turbo`, `libogg`, `libpng`, `libtheora`, `libuuid`, `libvorbis`, `libvpx`, `libwebp`, `libxml2`, `nettle`, `opencore-amr`, `opus`, `shine`, `snappy`, `soxr`, `speex`, `tiff`, `vid.stab`, `wavpack`, `x264`, `x265`, `xvidcore`

- Prebuilt binaries under `Github`, `JCenter` and `CocoaPods`
- Supports `arm-v7a`, `arm-v7a-neon`, `arm64-v8a`, `x86` and `x86_64` Android architectures
- Creates Android archive with .aar extension
- Supports `armv7`, `armv7s`, `arm64`, `i386` and `x86_64` IOS architectures
- `ARC` enabled library
- Built with `-fembed-bitcode` flag
- Creates IOS dynamic universal (fat) library
- Creates IOS dynamic framework for IOS 8 or later
- Licensed under LGPL 3.0, can be customized to support GPL v3.0


### 2. Using
Prebuilt libraries are available under [Github](https://github.com/tanersener/mobile-ffmpeg/releases), [JCenter](https://bintray.com/bintray/jcenter) and [CocoaPods](https://cocoapods.org)

There are eight different prebuilt packages. Below you can see which external libraries are enabled in each of them.

| min | min-gpl | https | https-gpl | audio | video | full | full-gpl |
| :----: | :----: | :----: | :----: | :----: | :----: | :----: | :----: |
|  -  |  vid.stab<sup>3</sup> <br/> x264<sup>1</sup> <br/> x265<sup>3</sup> <br/> xvidcore<sup>1</sup>  |  gnutls  |  gnutls <br/> vid.stab<sup>3</sup> <br/> x264<sup>1</sup> <br/> x265<sup>3</sup> <br/> xvidcore<sup>1</sup>  |  chromaprint<sup>3</sup> <br/> lame <br/> libilbc<sup>1</sup> <br/> libvorbis <br/> opencore-amr <br/> opus<sup>1</sup> <br/> shine <br/> soxr<sup>2</sup> <br/> speex <br/> wavpack  |  fontconfig <br/> freetype <br/> fribidi <br/> kvazaar <br/> libaom<sup>2</sup> <br/> libass <br/> libiconv <br/> libtheora <br/> libvpx <br/> snappy<sup>1</sup>  |  chromaprint<sup>3</sup> <br/> fontconfig <br/> freetype <br/> fribidi <br/> gmp <br/> gnutls <br/> kvazaar <br/> lame <br/> libaom<sup>2</sup> <br/> libass <br/> libiconv <br/> libilbc<sup>1</sup> <br/> libtheora <br/> libvorbis <br/> libvpx <br/> libwebp <br/> libxml2 <br/> opencore-amr <br/> opus<sup>1</sup> <br/> shine <br/> snappy<sup>1</sup> <br/> soxr<sup>2</sup> <br/> speex <br/> wavpack  |  chromaprint<sup>3</sup> <br/> fontconfig <br/> freetype <br/> fribidi <br/> gmp <br/> gnutls <br/> kvazaar <br/> lame <br/> libaom<sup>2</sup> <br/> libass <br/> libiconv <br/> libilbc<sup>1</sup> <br/> libtheora <br/> libvorbis <br/> libvpx <br/> libwebp <br/> libxml2 <br/> opencore-amr <br/> opus<sup>1</sup> <br/> shine <br/> snappy<sup>1</sup> <br/> soxr<sup>2</sup> <br/> speex <br/> vid.stab<sup>3</sup> <br/> wavpack <br/> x264<sup>1</sup> <br/> x265<sup>3</sup> <br/> xvidcore<sup>1</sup>  |
| [ ![Download](https://api.bintray.com/packages/tanersener/maven/mobile-ffmpeg-min/images/download.svg) ](https://bintray.com/tanersener/maven/mobile-ffmpeg-min/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tanersener/maven/mobile-ffmpeg-min-gpl/images/download.svg) ](https://bintray.com/tanersener/maven/mobile-ffmpeg-min-gpl/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tanersener/maven/mobile-ffmpeg-https/images/download.svg) ](https://bintray.com/tanersener/maven/mobile-ffmpeg-https/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tanersener/maven/mobile-ffmpeg-https-gpl/images/download.svg) ](https://bintray.com/tanersener/maven/mobile-ffmpeg-https-gpl/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tanersener/maven/mobile-ffmpeg-audio/images/download.svg) ](https://bintray.com/tanersener/maven/mobile-ffmpeg-audio/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tanersener/maven/mobile-ffmpeg-video/images/download.svg) ](https://bintray.com/tanersener/maven/mobile-ffmpeg-video/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tanersener/maven/mobile-ffmpeg-full/images/download.svg) ](https://bintray.com/tanersener/maven/mobile-ffmpeg-full/_latestVersion) | [ ![Download](https://api.bintray.com/packages/tanersener/maven/mobile-ffmpeg-full-gpl/images/download.svg) ](https://bintray.com/tanersener/maven/mobile-ffmpeg-full-gpl/_latestVersion) |

<sup>1</sup> - Supported since `v1.1`

<sup>2</sup> - Supported since `v2.0`

<sup>3</sup> - Supported since `v2.1`

#### 2.1 Android
1. Add MobileFFmpeg dependency from `jcenter()`
    ```
    dependencies {`
        implementation 'com.arthenica:mobile-ffmpeg-full:2.1'
    }
    ```

2. Execute commands.
    ```
    import com.arthenica.mobileffmpeg.FFmpeg;

    int rc = FFmpeg.execute("-i file1.mp4 -c:v mpeg4 file2.mp4");

    if (rc == RETURN_CODE_SUCCESS) {
        Log.i(Config.TAG, "Command execution completed successfully.");
    } else if (rc == RETURN_CODE_CANCEL) {
        Log.i(Config.TAG, "Command execution cancelled by user.");
    } else {
        Log.i(Config.TAG, String.format("Command execution failed with rc=%d.", rc));
    }
    ```

3. Stop an ongoing operation.
    ```
    FFmpeg.cancel();
    ```

4. Enable log callback.
    ```
    Config.enableLogCallback(new LogCallback() {
        public void apply(LogMessage message) {
            Log.d(Config.TAG, message.getText());
        }
    });
    ```

5. Enable statistics callback.
    ```
    Config.enableStatisticsCallback(new StatisticsCallback() {
        public void apply(Statistics newStatistics) {
            Log.d(Config.TAG, String.format("frame: %d, time: %d", newStatistics.getVideoFrameNumber(), newStatistics.getTime()));
        }
    });
    ```

6. Set log level.
    ```
    Config.setLogLevel(Level.AV_LOG_FATAL);
    ```

7. Register custom fonts directory.
    ```
    Config.setFontDirectory(this, "fonts", Collections.EMPTY_MAP);
    ```

#### 2.2 IOS
1. Add MobileFFmpeg pod to your `Podfile`
    ```
    pod 'mobile-ffmpeg-full', '~> 2.1'
    ```

2. Create and execute commands.
    ```
    #import <mobileffmpeg/MobileFFmpeg.h>

    int rc = [MobileFFmpeg execute: @"-i file1.mp4 -c:v mpeg4 file2.mp4"];

    if (rc == RETURN_CODE_SUCCESS) {
        NSLog(@"Command execution completed successfully.\n");
    } else if (rc == RETURN_CODE_CANCEL) {
        NSLog(@"Command execution cancelled by user.\n");
    } else {
        NSLog(@"Command execution failed with rc=%d.\n", rc);
    }
    ```

3. Stop an ongoing operation.
    ```
    [MobileFFmpeg cancel];
    ```

4. Enable log callback.
    ```
    - (void)logCallback: (int)level :(NSString*)message {
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"%@", message);
        });
    }
    ...
    [MobileFFmpegConfig setLogDelegate:self];
    ```

5. Enable statistics callback.
    ```
    - (void)statisticsCallback:(Statistics *)newStatistics {
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"frame: %d, time: %d\n", newStatistics.getVideoFrameNumber, newStatistics.getTime);
        });
    }
    ...
    [MobileFFmpegConfig setStatisticsDelegate:self];
    ```

6. Set log level.
    ```
    [MobileFFmpegConfig setLogLevel:AV_LOG_FATAL];
    ```

7. Register custom fonts directory.
    ```
    [MobileFFmpegConfig setFontDirectory:@"fonts" with:nil];
    ```

### 3. Versions

- `MobileFFmpeg v1.x` is the previous stable, includes `FFmpeg v3.4.4`

- `MobileFFmpeg v2.x` is the current stable, includes `FFmpeg v4.0.2`
    
### 4. Building
#### 4.1 Prerequisites
1. Use your package manager (apt, yum, dnf, brew, etc.) to install the following packages.

    ```
    autoconf automake libtool pkg-config curl cmake gcc gperf texinfo yasm nasm bison
    ```
2. Android builds require these additional packages.
    - **Android SDK 5.0 Lollipop (API Level 21)** or later
    - **Android NDK r16b** or later with LLDB and CMake
    - **gradle 4.4** or later

3. IOS builds need these extra packages and tools.
    - **IOS SDK 7.0.x** or later
    - **Xcode 8.x** or later
    - **Command Line Tools**

#### 4.2 Build Scripts
Use `android.sh` and `ios.sh` to build MobileFFmpeg for each platform.
After a successful build, compiled FFmpeg and MobileFFmpeg libraries can be found under `prebuilt` directory.

##### 4.2.1 Android
```
export ANDROID_NDK_ROOT=<Android NDK Path>
./android.sh
```

<img src="https://github.com/tanersener/mobile-ffmpeg/raw/master/docs/assets/android_custom.gif" width="600">

##### 4.2.2 IOS
```
./ios.sh
```

<img src="https://github.com/tanersener/mobile-ffmpeg/raw/master/docs/assets/ios_custom.gif" width="600">

#### 4.3 GPL Support
It is possible to enable GPL licensed libraries `x264`, `xvidcore` since `v1.1` and `vid.stab`, `x265` since `v2.1` 
from the top level build scripts.
Their source code is not included in the repository and downloaded when enabled.

#### 4.4 External Libraries
`build` directory includes build scripts for external libraries. There are two scripts for each library, one for Android
and one for IOS. They include all options/flags used to cross-compile the libraries.

### 5. Documentation

A more detailed documentation is available at [Wiki](https://github.com/tanersener/mobile-ffmpeg/wiki).

### 6. License

This project is licensed under the LGPL v3.0. However, if source code is built using optional `--enable-gpl` flag or 
prebuilt binaries with `-gpl` postfix are used then MobileFFmpeg is subject to the GPL v3.0 license.

Source code of FFmpeg and external libraries is included in compliance with their individual licenses.

`strip-frameworks.sh` script included and distributed is published under the [Apache License version 2.0](https://www.apache.org/licenses/LICENSE-2.0).

In test applications, fonts embedded are licensed under the [SIL Open Font License](https://opensource.org/licenses/OFL-1.1); other digital assets are published in the public domain.

Please visit [License](https://github.com/tanersener/mobile-ffmpeg/wiki/License) page for the details.

### 7. See Also

- [libav gas-preprocessor](https://github.com/libav/gas-preprocessor/raw/master/gas-preprocessor.pl)
- [FFmpeg API Documentation](https://ffmpeg.org/doxygen/4.0/index.html)
- [FFmpeg Wiki](https://trac.ffmpeg.org/wiki/WikiStart)
- [FFmpeg License and Legal Considerations](https://ffmpeg.org/legal.html)
- [FFmpeg External Library Licenses](https://www.ffmpeg.org/doxygen/4.0/md_LICENSE.html)