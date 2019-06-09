Pod::Spec.new do |s|  
    s.name              = "mobile-ffmpeg-video"
    s.version           = "VERSION"
    s.summary           = "Mobile FFmpeg Video Static Framework"
    s.description       = <<-DESC
    Includes FFmpeg v4.2-dev-1156 with fontconfig v2.13.1, freetype v2.10.0, fribidi v1.0.5, kvazaar v1.2.0, libaom v1.0.0-dev-1544, libass v0.14.0, libiconv v1.15, libtheora v1.1.1, libvpx v1.8.0, snappy v1.1.7 and libwebp v1.0.2 libraries enabled.
    DESC

    s.homepage          = "https://github.com/tanersener/mobile-ffmpeg"

    s.author            = { "Taner Sener" => "tanersener@gmail.com" }
    s.license           = { :type => "LGPL-3.0", :file => "mobileffmpeg.framework/LICENSE" }

    s.platform          = :ios
    s.platform          = :tvos

    s.requires_arc      = true
    s.libraries         = 'z', 'bz2', 'c++'

    s.source            = { :http => "https://github.com/tanersener/mobile-ffmpeg/releases/download/vVERSION/mobile-ffmpeg-video-VERSION-ios-tvos-framework.zip" }

    s.ios.deployment_target = '9.3'
    s.ios.frameworks    = 'AudioToolbox','CoreMedia','VideoToolbox'
    s.ios.vendored_frameworks  = 'ios/mobileffmpeg.framework', 'ios/libavcodec.framework', 'ios/libavdevice.framework', 'ios/libavfilter.framework', 'ios/libavformat.framework', 'ios/libavutil.framework', 'ios/libswresample.framework', 'ios/libswscale.framework', 'ios/expat.framework', 'ios/fontconfig.framework', 'ios/freetype.framework', 'ios/fribidi.framework', 'ios/giflib.framework', 'ios/jpeg.framework', 'ios/kvazaar.framework', 'ios/libaom.framework', 'ios/libass.framework', 'ios/libcharset.framework', 'ios/libiconv.framework', 'ios/libogg.framework', 'ios/libpng.framework', 'ios/libtheora.framework', 'ios/libtheoradec.framework', 'ios/libtheoraenc.framework', 'ios/libuuid.framework', 'ios/libvorbis.framework', 'ios/libvorbisenc.framework', 'ios/libvorbisfile.framework', 'ios/libvpx.framework', 'ios/libwebp.framework', 'ios/libwebpdecoder.framework', 'ios/libwebpdemux.framework', 'ios/snappy.framework', 'ios/tiff.framework'
    s.ios.preserve_paths       = 'ios/*.framework'

    s.tvos.deployment_target = '9.2'
    s.tvos.frameworks   = 'AudioToolbox','CoreMedia','VideoToolbox'
    s.tvos.vendored_frameworks  = 'tvos/mobileffmpeg.framework', 'tvos/libavcodec.framework', 'tvos/libavdevice.framework', 'tvos/libavfilter.framework', 'tvos/libavformat.framework', 'tvos/libavutil.framework', 'tvos/libswresample.framework', 'tvos/libswscale.framework', 'tvos/expat.framework', 'tvos/fontconfig.framework', 'tvos/freetype.framework', 'tvos/fribidi.framework', 'tvos/giflib.framework', 'tvos/jpeg.framework', 'tvos/kvazaar.framework', 'tvos/libaom.framework', 'tvos/libass.framework', 'tvos/libcharset.framework', 'tvos/libiconv.framework', 'tvos/libogg.framework', 'tvos/libpng.framework', 'tvos/libtheora.framework', 'tvos/libtheoradec.framework', 'tvos/libtheoraenc.framework', 'tvos/libuuid.framework', 'tvos/libvorbis.framework', 'tvos/libvorbisenc.framework', 'tvos/libvorbisfile.framework', 'tvos/libvpx.framework', 'tvos/libwebp.framework', 'tvos/libwebpdecoder.framework', 'tvos/libwebpdemux.framework', 'tvos/snappy.framework', 'tvos/tiff.framework'
    s.tvos.preserve_paths       = 'tvos/*.framework'

end  
