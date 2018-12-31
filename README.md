# MobileFFmpeg [![Join the chat at https://gitter.im/mobile-ffmpeg/Lobby](https://badges.gitter.im/mobile-ffmpeg/Lobby.svg)](https://gitter.im/mobile-ffmpeg/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge) ![GitHub release](https://img.shields.io/badge/release-v4.2-blue.svg) ![Bintray](https://img.shields.io/badge/bintray-v4.2-blue.svg) ![CocoaPods](https://img.shields.io/badge/pod-v4.2-blue.svg)

FFmpeg for Android and IOS

<img src="https://github.com/tanersener/mobile-ffmpeg/blob/dev-v3.x/docs/assets/mobile-ffmpeg-logo-v7.png" width="320">

### 1. Features
- Use binaries available at `Github`/`JCenter`/`CocoaPods` or build your own version with external libraries you need
- Supports
    - Both Android and IOS
    - FFmpeg `v3.4.x`, `v4.0.x`, `v4.1` and `v4.2-dev` releases
    - 27 external libraries
    
        `chromaprint`, `fontconfig`, `freetype`, `fribidi`, `gmp`, `gnutls`, `kvazaar`, `lame`, `libaom`, `libass`, `libiconv`, `libilbc`, `libtheora`, `libvorbis`, `libvpx`, `libwebp`, `libxml2`, `opencore-amr`, `opus`, `sdl`, `shine`, `snappy`, `soxr`, `speex`, `tesseract`, `twolame`, `wavpack`
    
    - 4 external libraries with GPL license
    
        `vid.stab`, `x264`, `x265`, `xvidcore`

- Exposes both FFmpeg library and MobileFFmpeg wrapper library capabilities
- Includes cross-compile instructions for 43 open-source libraries
    
    `chromaprint`, `expat`, `ffmpeg`, `fontconfig`, `freetype`, `fribidi`, `giflib`, `gmp`, `gnutls`, `kvazaar`, `lame`, `leptonica`, `libaom`, `libass`, `libiconv`, `libilbc`, `libjpeg`, `libjpeg-turbo`, `libogg`, `libpng`, `libsndfile`, `libtheora`, `libuuid`, `libvorbis`, `libvpx`, `libwebp`, `libxml2`, `nettle`, `opencore-amr`, `opus`, `sdl`, `shine`, `snappy`, `soxr`, `speex`, `tesseract`, `tiff`, `twolame`, `vid.stab`, `wavpack`, `x264`, `x265`, `xvidcore`

- Licensed under LGPL 3.0, can be customized to support GPL v3.0

#### 1.1 Android
- Builds `arm-v7a`, `arm-v7a-neon`, `arm64-v8a`, `x86` and `x86_64` architectures
- Supports `zlib` and `MediaCodec` system libraries
- Camera access on [supported devices](https://developer.android.com/ndk/guides/stable_apis#camera)
- Builds shared native libraries (.so)
- Creates Android archive with .aar extension

#### 1.2 IOS
- Builds `armv7`, `armv7s`, `arm64`, `arm64e`, `i386` and `x86_64` architectures
- Supports `bzip2`, `zlib` system libraries and `AudioToolbox`, `CoreImage`, `VideoToolbox`, `AVFoundation` system frameworks
- Objective-C API
- Camera access
- `ARC` enabled library
- Built with `-fembed-bitcode` flag
- Creates static framework and static universal (fat) library (.a) 
- Supports Xcode 7.3.1 or later

### 2. Using
Binaries are available at [Github](https://github.com/tanersener/mobile-ffmpeg/releases), [JCenter](https://bintray.com/bintray/jcenter) and [CocoaPods](https://cocoapods.org). 

Refer to [Using IOS Universal Binaries](https://github.com/tanersener/mobile-ffmpeg/wiki/Using-IOS-Universal-Binaries) guide to import IOS universal binaries released at [Github](https://github.com/tanersener/mobile-ffmpeg/releases).

There are eight different binary packages. Below you can see which system libraries and external libraries are enabled in each of them.

<table>
<thead>
<tr>
<th align="center"></th>
<th align="center">min</th>
<th align="center">min-gpl</th>
<th align="center">https</th>
<th align="center">https-gpl</th>
<th align="center">audio</th>
<th align="center">video</th>
<th align="center">full</th>
<th align="center">full-gpl</th>
</tr>
</thead>
<tbody>
<tr>
<td align="center"><sup>external libraries</sup></td>
<td align="center">-</td>
<td align="center"><sup>vid.stab</sup><br><sup>x264</sup><br><sup>x265</sup><br><sup>xvidcore</sup></td>
<td align="center"><sup>gmp</sup><br><sup>gnutls</sup></td>
<td align="center"><sup>gmp</sup><br><sup>gnutls</sup><br><sup>vid.stab</sup><br><sup>x264</sup><br><sup>x265</sup><br><sup>xvidcore</sup></td>
<td align="center"><sup>lame</sup><br><sup>libilbc</sup><br><sup>libvorbis</sup><br><sup>opencore-amr</sup><br><sup>opus</sup><br><sup>shine</sup><br><sup>soxr</sup><br><sup>speex</sup><br><sup>twolame</sup><br><sup>wavpack</sup></td>
<td align="center"><sup>fontconfig</sup><br><sup>freetype</sup><br><sup>fribidi</sup><br><sup>kvazaar</sup><br><sup>libaom</sup><br><sup>libass</sup><br><sup>libiconv</sup><br><sup>libtheora</sup><br><sup>libvpx</sup><br><sup>libwebp</sup><br><sup>snappy</sup></td>
<td align="center"><sup>fontconfig</sup><br><sup>freetype</sup><br><sup>fribidi</sup><br><sup>gmp</sup><br><sup>gnutls</sup><br><sup>kvazaar</sup><br><sup>lame</sup><br><sup>libaom</sup><br><sup>libass</sup><br><sup>libiconv</sup><br><sup>libilbc</sup><br><sup>libtheora</sup><br><sup>libvorbis</sup><br><sup>libvpx</sup><br><sup>libwebp</sup><br><sup>libxml2</sup><br><sup>opencore-amr</sup><br><sup>opus</sup><br><sup>shine</sup><br><sup>snappy</sup><br><sup>soxr</sup><br><sup>speex</sup><br><sup>twolame</sup><br><sup>wavpack</sup></td>
<td align="center"><sup>fontconfig</sup><br><sup>freetype</sup><br><sup>fribidi</sup><br><sup>gmp</sup><br><sup>gnutls</sup><br><sup>kvazaar</sup><br><sup>lame</sup><br><sup>libaom</sup><br><sup>libass</sup><br><sup>libiconv</sup><br><sup>libilbc</sup><br><sup>libtheora</sup><br><sup>libvorbis</sup><br><sup>libvpx</sup><br><sup>libwebp</sup><br><sup>libxml2</sup><br><sup>opencore-amr</sup><br><sup>opus</sup><br><sup>shine</sup><br><sup>snappy</sup><br><sup>soxr</sup><br><sup>speex</sup><br><sup>twolame</sup><br><sup>vid.stab</sup><br><sup>wavpack</sup><br><sup>x264</sup><br><sup>x265</sup><br><sup>xvidcore</sup></td>
</tr>
<tr>
<td align="center"><sup>android system libraries</sup></td>
<td align="center" colspan=8><sup>zlib</sup><br><sup>MediaCodec</sup></td>
</tr>
<tr>
<td align="center"><sup>ios system libraries</sup></td>
<td align="center" colspan=8><sup>zlib</sup><br><sup>AudioToolbox</sup><br><sup>AVFoundation</sup><br><sup>CoreImage</sup><br><sup>VideoToolbox</sup><br><sup>bzip2</sup></td>
</tr>
</tbody>
</table>

 - `libilbc`, `opus`, `snappy`, `x264` and `xvidcore` are supported since `v1.1`

 - `libaom` and `soxr` are supported since `v2.0`

 - `chromaprint`, `vid.stab` and `x265` are supported since `v2.1`

 - `sdl`, `tesseract`, `twolame` external libraries; `zlib`, `MediaCodec` Android system libraries; `bzip2`, `zlib` IOS system libraries and `AudioToolbox`, `CoreImage`, `VideoToolbox`, `AVFoundation` IOS system frameworks are supported since `v3.0`
 - Since `v4.2`, `chromaprint`, `sdl` and `tesseract` libraries are not included in binary releases. You can still build them and include in your releases

#### 2.1 Android
1. Add MobileFFmpeg dependency from `jcenter()`
    ```
    dependencies {
        implementation 'com.arthenica:mobile-ffmpeg-full:4.2'
    }
    ```

2. Execute commands.
    ```
    import com.arthenica.mobileffmpeg.FFmpeg;

    FFmpeg.execute("-i file1.mp4 -c:v mpeg4 file2.mp4");
    ```

3. Check execution output.
    ```
    int rc = FFmpeg.getLastReturnCode();
    String output = FFmpeg.getLastCommandOutput();
 
    if (rc == RETURN_CODE_SUCCESS) {
        Log.i(Config.TAG, "Command execution completed successfully.");
    } else if (rc == RETURN_CODE_CANCEL) {
        Log.i(Config.TAG, "Command execution cancelled by user.");
    } else {
        Log.i(Config.TAG, String.format("Command execution failed with rc=%d and output=%s.", rc, output));
    }
    ```

4. Stop an ongoing operation.
    ```
    FFmpeg.cancel();
    ```

5. Get media information for a file.
    ```
    MediaInformation info = FFmpeg.getMediaInformation("<file path or uri>");
    ```

6. Record video and audio using Android camera.
    ```
    FFmpeg.execute("-f android_camera -i 0:0 -r 30 -pixel_format bgr0 -t 00:00:05 <record file path>");
    ```

7. List enabled external libraries.
    ```
    List<String> externalLibraries = Config.getExternalLibraries();
    ```

8. Enable log callback.
    ```
    Config.enableLogCallback(new LogCallback() {
        public void apply(LogMessage message) {
            Log.d(Config.TAG, message.getText());
        }
    });
    ```

9. Enable statistics callback.
    ```
    Config.enableStatisticsCallback(new StatisticsCallback() {
        public void apply(Statistics newStatistics) {
            Log.d(Config.TAG, String.format("frame: %d, time: %d", newStatistics.getVideoFrameNumber(), newStatistics.getTime()));
        }
    });
    ```

10. Set log level.
    ```
    Config.setLogLevel(Level.AV_LOG_FATAL);
    ```

11. Register custom fonts directory.
    ```
    Config.setFontDirectory(this, "<folder with fonts>", Collections.EMPTY_MAP);
    ```

#### 2.2 IOS
1. Add MobileFFmpeg pod to your `Podfile`
    ```
    pod 'mobile-ffmpeg-full', '~> 4.2'
    ```

2. Execute commands.
    ```
    #import <mobileffmpeg/MobileFFmpeg.h>

    [MobileFFmpeg execute: @"-i file1.mp4 -c:v mpeg4 file2.mp4"];
    ```
    
3. Check execution output.
    ```
    int rc = [MobileFFmpeg getLastReturnCode];
    NSString *output = [MobileFFmpeg getLastCommandOutput];

    if (rc == RETURN_CODE_SUCCESS) {
        NSLog(@"Command execution completed successfully.\n");
    } else if (rc == RETURN_CODE_CANCEL) {
        NSLog(@"Command execution cancelled by user.\n");
    } else {
        NSLog(@"Command execution failed with rc=%d and output=%@.\n", rc, output);
    }
    ```

4. Stop an ongoing operation.
    ```
    [MobileFFmpeg cancel];
    ```

5. Get media information for a file.
    ```
    MediaInformation *mediaInformation = [MobileFFmpeg getMediaInformation:@"<file path or uri>"];
    ```

6. Record video and audio using IOS camera

    ```
    [MobileFFmpeg execute: @"-f avfoundation -r 30 -video_size 1280x720 -pixel_format bgr0 -i 0:0 -vcodec h264_videotoolbox -vsync 2 -f h264 -t 00:00:05 %@", recordFilePath];
    ```

7. List enabled external libraries.
    ```
    NSArray *externalLibraries = [MobileFFmpegConfig getExternalLibraries];
    ```

8. Enable log callback.
    ```
    [MobileFFmpegConfig setLogDelegate:self];

    - (void)logCallback: (int)level :(NSString*)message {
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"%@", message);
        });
    }
    ```

9. Enable statistics callback.
    ```
    [MobileFFmpegConfig setStatisticsDelegate:self];

    - (void)statisticsCallback:(Statistics *)newStatistics {
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"frame: %d, time: %d\n", newStatistics.getVideoFrameNumber, newStatistics.getTime);
        });
    }
    ```

10. Set log level.
    ```
    [MobileFFmpegConfig setLogLevel:AV_LOG_FATAL];
    ```

11. Register custom fonts directory.
    ```
    [MobileFFmpegConfig setFontDirectory:@"<folder with fonts>" with:nil];
    ```
    
#### 2.3 Test Application
You can see how MobileFFmpeg is used inside an application by running test applications provided.
There is an Android test application under the `android/test-app` folder and an IOS test application, which requires 
`Xcode 9.x` or later, under the `ios/test-app` folder. Both applications are identical and supports command 
execution, video encoding, accessing https, encoding audio, burning subtitles and video stabilization.

<img src="https://github.com/tanersener/mobile-ffmpeg/blob/master/docs/assets/ios_test_app.gif" width="240">

### 3. Versions

`MobileFFmpeg` uses the same version number as `FFmpeg` since `v4.2`. Before that, `MobileFFmpeg` version of a release and `FFmpeg` version included in that release was different, as shown in the following table. 

|        | v1.0 | v1.1 | v1.2 | v2.0 | v2.1 | v2.2 | v3.0 | v3.1 | v4.2 |
| :----: | :----: | :----: | :----: | :----: | :----: | :----: | :----: | :----: | :----: |
| FFmpeg | 3.4.2 | 3.4.2 | 3.4.4 | 4.0.1 | 4.0.2 | 4.0.3 | 4.1-dev-1517 | v4.1-10 | v4.2-dev-480 |
| packages | min<br/>full | min<br/>min-gpl<br/>https<br/>https-gpl<br/>full<br/>full-gpl | min<br/>min-gpl<br/>https<br/>https-gpl<br/>full<br/>full-gpl | min<br/>min-gpl<br/>https<br/>https-gpl<br/>full<br/>full-gpl | min<br/>min-gpl<br/>https<br/>https-gpl<br/>audio<br/>video<br/>full<br/>full-gpl | min<br/>min-gpl<br/>https<br/>https-gpl<br/>audio<br/>video<br/>full<br/>full-gpl | min<br/>min-gpl<br/>https<br/>https-gpl<br/>audio<br/>video<br/>full<br/>full-gpl | min<br/>min-gpl<br/>https<br/>https-gpl<br/>audio<br/>video<br/>full<br/>full-gpl | min<br/>min-gpl<br/>https<br/>https-gpl<br/>audio<br/>video<br/>full<br/>full-gpl |

### 4. LTS Releases

Starting from `v4.2`, `MobileFFmpeg` binaries are published in two different variants: `Main Release` and `LTS Release`. 

- Main releases include complete functionality of the library and support the latest SDK/API features

- LTS releases are customized to support a wide range of devices. They are built using older API/SDK versions, so some features are not available for them

This table shows the differences between two variants.

|        | Main Release | LTS Release |
| :----: | :----: | :----: |
| Android API Level | 24 | 21 | 
| Android Camera Access | x | - |
| Android Architectures | arm-v7a-neon<br/>arm64-v8a<br/>x86<br/>x86-64</br> | arm-v7a<br/>arm-v7a-neon<br/>arm64-v8a<br/>x86<br/>x86-64</br> |
| IOS SDK | 12.1 | 9.3 |
| Xcode Support | 10.1 | 7.3.1 |
| IOS Architectures | arm64<br/>arm64e<br/>x86-64</br> | armv7<br/>arm64<br/>i386<br/>x86-64</br> |

### 5. Building
#### 5.1 Prerequisites
1. Use your package manager (apt, yum, dnf, brew, etc.) to install the following packages.

    ```
    autoconf automake libtool pkg-config curl cmake gcc gperf texinfo yasm nasm bison autogen patch
    ```
Some of these packages are not mandatory for the default build.
Please visit [Android Prerequisites](https://github.com/tanersener/mobile-ffmpeg/wiki/Android-Prerequisites) and
[IOS Prerequisites](https://github.com/tanersener/mobile-ffmpeg/wiki/IOS-Prerequisites) for the details.

2. Android builds require these additional packages.
    - **Android SDK 5.0 Lollipop (API Level 21)** or later
    - **Android NDK r17c** or later with LLDB and CMake

3. IOS builds need these extra packages and tools.
    - **IOS SDK 8.0.x** or later
    - **Xcode 7.3.1** or later
    - **Command Line Tools**

#### 5.2 Build Scripts
Use `android.sh` and `ios.sh` to build MobileFFmpeg for each platform.
After a successful build, compiled FFmpeg and MobileFFmpeg libraries can be found under `prebuilt` directory.

Both `android.sh` and `ios.sh` can be customized to override default settings,
[android.sh](https://github.com/tanersener/mobile-ffmpeg/wiki/android.sh) and
[ios.sh](https://github.com/tanersener/mobile-ffmpeg/wiki/ios.sh) wiki pages include all available build options.
##### 5.2.1 Android 
```
export ANDROID_NDK_ROOT=<Android NDK Path>
./android.sh
```

<img src="https://github.com/tanersener/mobile-ffmpeg/blob/master/docs/assets/android_custom.gif" width="600">

##### 5.2.2 IOS
```
./ios.sh    
```

<img src="https://github.com/tanersener/mobile-ffmpeg/blob/master/docs/assets/ios_custom.gif" width="600">

##### 5.2.3 Building LTS Binaries

Use `--lts` option to build lts binaries for each platform.

#### 5.3 GPL Support
It is possible to enable GPL licensed libraries `x264`, `xvidcore` since `v1.1` and `vid.stab`, `x265` since `v2.1` 
from the top level build scripts.
Their source code is not included in the repository and downloaded when enabled.

#### 5.4 External Libraries
`build` directory includes build scripts for external libraries. There are two scripts for each library, one for Android
and one for IOS. They include all options/flags used to cross-compile the libraries. `ASM` is enabled by most of them, 
exceptions are listed under the [ASM Support](https://github.com/tanersener/mobile-ffmpeg/wiki/ASM-Support) page.

### 6. Documentation

A more detailed documentation is available at [Wiki](https://github.com/tanersener/mobile-ffmpeg/wiki).

### 7. License

This project is licensed under the LGPL v3.0. However, if source code is built using optional `--enable-gpl` flag or 
prebuilt binaries with `-gpl` postfix are used then MobileFFmpeg is subject to the GPL v3.0 license.

Source code of FFmpeg and external libraries is included in compliance with their individual licenses.

`strip-frameworks.sh` script included and distributed is published under the [Apache License version 2.0](https://www.apache.org/licenses/LICENSE-2.0).

In test applications, fonts embedded are licensed under the [SIL Open Font License](https://opensource.org/licenses/OFL-1.1); other digital assets are published in the public domain.

Please visit [License](https://github.com/tanersener/mobile-ffmpeg/wiki/License) page for the details.

### 8. Contributing

If you have any recommendations or ideas to improve it, please feel free to submit issues or pull requests. Any help is appreciated.

### 9. See Also

- [libav gas-preprocessor](https://github.com/libav/gas-preprocessor/raw/master/gas-preprocessor.pl)
- [FFmpeg API Documentation](https://ffmpeg.org/doxygen/4.0/index.html)
- [FFmpeg Wiki](https://trac.ffmpeg.org/wiki/WikiStart)
- [FFmpeg License and Legal Considerations](https://ffmpeg.org/legal.html)
- [FFmpeg External Library Licenses](https://www.ffmpeg.org/doxygen/4.0/md_LICENSE.html)
