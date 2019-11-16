Pod::Spec.new do |s|  
    s.name              = "mobile-ffmpeg-video"
    s.version           = "VERSION"
    s.summary           = "Mobile FFmpeg Video Static Framework"
    s.description       = <<-DESC
    Includes FFmpeg v4.3-dev-1181 with fontconfig v2.13.92, freetype v2.10.1, fribidi v1.0.7, kvazaar v1.3.0, libaom v1.0.0-dev-2567, libass v0.14.0, libiconv v1.16, libtheora v1.1.1, libvpx v1.8.1, snappy v1.1.7 and libwebp v1.0.3 libraries enabled.
    DESC

    s.homepage          = "https://github.com/tanersener/mobile-ffmpeg"

    s.author            = { "Taner Sener" => "tanersener@gmail.com" }
    s.license           = { :type => "LGPL-3.0", :file => "mobileffmpeg.framework/LICENSE" }

    s.platform          = :ios
    s.requires_arc      = true
    s.libraries         = 'z', 'bz2', 'c++'

    s.source            = { :http => "https://github.com/tanersener/mobile-ffmpeg/releases/download/vVERSION/mobile-ffmpeg-video-VERSION-ios-framework.zip" }

    s.ios.deployment_target = '9.3'
    s.ios.frameworks    = 'AudioToolbox','AVFoundation','CoreMedia','VideoToolbox'
    s.ios.vendored_frameworks = 'mobileffmpeg.framework', 'libavcodec.framework', 'libavdevice.framework', 'libavfilter.framework', 'libavformat.framework', 'libavutil.framework', 'libswresample.framework', 'libswscale.framework', 'expat.framework', 'fontconfig.framework', 'freetype.framework', 'fribidi.framework', 'giflib.framework', 'jpeg.framework', 'kvazaar.framework', 'libaom.framework', 'libass.framework', 'libcharset.framework', 'libiconv.framework', 'libogg.framework', 'libpng.framework', 'libtheora.framework', 'libtheoradec.framework', 'libtheoraenc.framework', 'libuuid.framework', 'libvorbis.framework', 'libvorbisenc.framework', 'libvorbisfile.framework', 'libvpx.framework', 'libwebp.framework', 'libwebpdecoder.framework', 'libwebpdemux.framework', 'snappy.framework', 'tiff.framework'

end  
