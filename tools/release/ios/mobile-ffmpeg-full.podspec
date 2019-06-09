Pod::Spec.new do |s|  
    s.name              = "mobile-ffmpeg-full"
    s.version           = "VERSION"
    s.summary           = "Mobile FFmpeg Full Static Framework"
    s.description       = <<-DESC
    Includes FFmpeg v4.2-dev-1156 with fontconfig v2.13.1, freetype v2.10.0, fribidi v1.0.5, gmp v6.1.2, gnutls v3.5.19, kvazaar v1.2.0, lame v3.100, libaom v1.0.0-dev-1544, libass v0.14.0, libiconv v1.15, libilbc v2.0.2, libtheora v1.1.1, libvorbis v1.3.6, libvpx v1.8.0, libwebp v1.0.2, libxml2 v2.9.9, opencore-amr v0.1.5, opus v1.3, shine v3.1.1, snappy v1.1.7, soxr v0.1.3, speex v1.2.0, twolame v0.3.13 and wavpack v5.1.0 libraries enabled.
    DESC

    s.homepage          = "https://github.com/tanersener/mobile-ffmpeg"

    s.author            = { "Taner Sener" => "tanersener@gmail.com" }
    s.license           = { :type => "LGPL-3.0", :file => "mobileffmpeg.framework/LICENSE" }

    s.platform          = :ios
    s.platform          = :tvos

    s.requires_arc      = true
    s.libraries         = 'z', 'bz2', 'c++'

    s.source            = { :http => "https://github.com/tanersener/mobile-ffmpeg/releases/download/vVERSION/mobile-ffmpeg-full-VERSION-ios-tvos-framework.zip" }

    s.ios.deployment_target = '9.3'
    s.ios.frameworks    = 'AudioToolbox','CoreMedia','VideoToolbox'
    s.ios.vendored_frameworks  = 'ios/mobileffmpeg.framework', 'ios/libavcodec.framework', 'ios/libavdevice.framework', 'ios/libavfilter.framework', 'ios/libavformat.framework', 'ios/libavutil.framework', 'ios/libswresample.framework', 'ios/libswscale.framework', 'ios/expat.framework', 'ios/fontconfig.framework', 'ios/freetype.framework', 'ios/fribidi.framework', 'ios/giflib.framework', 'ios/gmp.framework', 'ios/gnutls.framework', 'ios/jpeg.framework', 'ios/kvazaar.framework', 'ios/lame.framework', 'ios/libaom.framework', 'ios/libass.framework', 'ios/libcharset.framework', 'ios/libhogweed.framework', 'ios/libiconv.framework', 'ios/libilbc.framework', 'ios/libnettle.framework', 'ios/libogg.framework', 'ios/libopencore-amrnb.framework', 'ios/libopencore-amrwb.framework', 'ios/libpng.framework', 'ios/libsndfile.framework', 'ios/libtheora.framework', 'ios/libtheoradec.framework', 'ios/libtheoraenc.framework', 'ios/libuuid.framework', 'ios/libvorbis.framework', 'ios/libvorbisenc.framework', 'ios/libvorbisfile.framework', 'ios/libvpx.framework', 'ios/libwebp.framework', 'ios/libwebpdecoder.framework', 'ios/libwebpdemux.framework', 'ios/libxml2.framework', 'ios/opus.framework', 'ios/shine.framework', 'ios/snappy.framework', 'ios/soxr.framework', 'ios/speex.framework', 'ios/tiff.framework', 'ios/twolame.framework', 'ios/wavpack.framework'
    s.ios.preserve_paths       = 'ios/*.framework'

    s.tvos.deployment_target = '9.2'
    s.tvos.frameworks   = 'AudioToolbox','CoreMedia','VideoToolbox'
    s.tvos.vendored_frameworks  = 'tvos/mobileffmpeg.framework', 'tvos/libavcodec.framework', 'tvos/libavdevice.framework', 'tvos/libavfilter.framework', 'tvos/libavformat.framework', 'tvos/libavutil.framework', 'tvos/libswresample.framework', 'tvos/libswscale.framework', 'tvos/expat.framework', 'tvos/fontconfig.framework', 'tvos/freetype.framework', 'tvos/fribidi.framework', 'tvos/giflib.framework', 'tvos/gmp.framework', 'tvos/gnutls.framework', 'tvos/jpeg.framework', 'tvos/kvazaar.framework', 'tvos/lame.framework', 'tvos/libaom.framework', 'tvos/libass.framework', 'tvos/libcharset.framework', 'tvos/libhogweed.framework', 'tvos/libiconv.framework', 'tvos/libilbc.framework', 'tvos/libnettle.framework', 'tvos/libogg.framework', 'tvos/libopencore-amrnb.framework', 'tvos/libopencore-amrwb.framework', 'tvos/libpng.framework', 'tvos/libsndfile.framework', 'tvos/libtheora.framework', 'tvos/libtheoradec.framework', 'tvos/libtheoraenc.framework', 'tvos/libuuid.framework', 'tvos/libvorbis.framework', 'tvos/libvorbisenc.framework', 'tvos/libvorbisfile.framework', 'tvos/libvpx.framework', 'tvos/libwebp.framework', 'tvos/libwebpdecoder.framework', 'tvos/libwebpdemux.framework', 'tvos/libxml2.framework', 'tvos/opus.framework', 'tvos/shine.framework', 'tvos/snappy.framework', 'tvos/soxr.framework', 'tvos/speex.framework', 'tvos/tiff.framework', 'tvos/twolame.framework', 'tvos/wavpack.framework'
    s.tvos.preserve_paths       = 'tvos/*.framework'

end  
