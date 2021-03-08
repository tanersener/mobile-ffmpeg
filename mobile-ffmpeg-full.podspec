Pod::Spec.new do |s|  
    s.name              = "mobile-ffmpeg-full"
    s.version           = "4.4"
    s.summary           = "Mobile FFmpeg Full Static Framework"
    s.description       = <<-DESC
    Includes FFmpeg v4.4-dev-416 with fontconfig v2.13.92, freetype v2.10.2, fribidi v1.0.9, gmp v6.2.0, gnutls v3.6.13, kvazaar v2.0.0, lame v3.100, libaom v1.0.0-errata1-avif-110, libass v0.14.0, libilbc v2.0.2, libtheora v1.1.1, libvorbis v1.3.7, libvpx v1.8.2, libwebp v1.1.0, libxml2 v2.9.10, opencore-amr v0.1.5, opus v1.3.1, shine v3.1.1, snappy v1.1.8, soxr v0.1.3, speex v1.2.0, twolame v0.4, vo-amrwbenc v0.1.3 and wavpack v5.3.0 libraries enabled.
    DESC

    s.homepage          = "https://github.com/tanersener/mobile-ffmpeg"

    s.author            = { "Taner Sener" => "tanersener@gmail.com" }
    s.license           = { :type => "LGPL-3.0", :file => "mobileffmpeg.framework/LICENSE" }

    s.platform          = :ios
    s.requires_arc      = true
    s.libraries         = 'z', 'bz2', 'c++', 'iconv'
    s.source            = { :http => "https://github.com/Kannelle-io/mobile-ffmpeg/releases/download/v4.4.M1/mobile-ffmpeg-full-4.4-ios-xcframework.zip" }

    s.ios.deployment_target = '9.3'
    s.ios.frameworks    = 'AudioToolbox','AVFoundation','CoreMedia','VideoToolbox'
    spec.preserve_paths      = 'mobileffmpeg.xcframework', 'libavcodec.xcframework', 'libavdevice.xcframework', 'libavfilter.xcframework', 'libavformat.xcframework', 'libavutil.xcframework', 'libswresample.xcframework', 'libswscale.xcframework', 'expat.xcframework', 'fontconfig.xcframework', 'freetype.xcframework', 'fribidi.xcframework', 'giflib.xcframework', 'gmp.xcframework', 'gnutls.xcframework', 'jpeg.xcframework', 'kvazaar.xcframework', 'lame.xcframework', 'libaom.xcframework', 'libass.xcframework', 'libhogweed.xcframework', 'libilbc.xcframework', 'libnettle.xcframework', 'libogg.xcframework', 'libopencore-amrnb.xcframework', 'libpng.xcframework', 'libsndfile.xcframework', 'libtheora.xcframework', 'libtheoradec.xcframework', 'libtheoraenc.xcframework', 'libvorbis.xcframework', 'libvorbisenc.xcframework', 'libvorbisfile.xcframework', 'libvpx.xcframework', 'libwebp.xcframework', 'libwebpmux.xcframework', 'libwebpdemux.xcframework', 'libxml2.xcframework', 'opus.xcframework', 'shine.xcframework', 'snappy.xcframework', 'soxr.xcframework', 'speex.xcframework', 'tiff.xcframework', 'twolame.xcframework', 'vo-amrwbenc.xcframework', 'wavpack.xcframework'
    spec.vendored_frameworks = 'mobileffmpeg.xcframework', 'libavcodec.xcframework', 'libavdevice.xcframework', 'libavfilter.xcframework', 'libavformat.xcframework', 'libavutil.xcframework', 'libswresample.xcframework', 'libswscale.xcframework', 'expat.xcframework', 'fontconfig.xcframework', 'freetype.xcframework', 'fribidi.xcframework', 'giflib.xcframework', 'gmp.xcframework', 'gnutls.xcframework', 'jpeg.xcframework', 'kvazaar.xcframework', 'lame.xcframework', 'libaom.xcframework', 'libass.xcframework', 'libhogweed.xcframework', 'libilbc.xcframework', 'libnettle.xcframework', 'libogg.xcframework', 'libopencore-amrnb.xcframework', 'libpng.xcframework', 'libsndfile.xcframework', 'libtheora.xcframework', 'libtheoradec.xcframework', 'libtheoraenc.xcframework', 'libvorbis.xcframework', 'libvorbisenc.xcframework', 'libvorbisfile.xcframework', 'libvpx.xcframework', 'libwebp.xcframework', 'libwebpmux.xcframework', 'libwebpdemux.xcframework', 'libxml2.xcframework', 'opus.xcframework', 'shine.xcframework', 'snappy.xcframework', 'soxr.xcframework', 'speex.xcframework', 'tiff.xcframework', 'twolame.xcframework', 'vo-amrwbenc.xcframework', 'wavpack.xcframework'

end  
