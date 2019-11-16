Pod::Spec.new do |s|  
    s.name              = "mobile-ffmpeg-full-gpl"
    s.version           = "VERSION"
    s.summary           = "Mobile FFmpeg Full GPL Static Framework"
    s.description       = <<-DESC
    Includes FFmpeg v4.3-dev-1181 with fontconfig v2.13.92, freetype v2.10.1, fribidi v1.0.7, gmp v6.1.2, gnutls v3.6.10, kvazaar v1.3.0, lame v3.100, libaom v1.0.0-dev-2567, libass v0.14.0, libiconv v1.16, libilbc v2.0.2, libtheora v1.1.1, libvid.stab v1.1.0, libvorbis v1.3.6, libvpx v1.8.1, libwebp v1.0.3, libxml2 v2.9.9, opencore-amr v0.1.5, opus v1.3.1, shine v3.1.1, snappy v1.1.7, soxr v0.1.3, speex v1.2.0, twolame v0.4, wavpack v5.1.0, x264 v20191024-2245-stable, x265 v3.2 and xvidcore v1.3.5 libraries enabled.
    DESC

    s.homepage          = "https://github.com/tanersener/mobile-ffmpeg"

    s.author            = { "Taner Sener" => "tanersener@gmail.com" }
    s.license           = { :type => "GPL-3.0", :file => "mobileffmpeg.framework/LICENSE" }

    s.platform          = :ios
    s.requires_arc      = true
    s.libraries         = 'z', 'bz2', 'c++'

    s.source            = { :http => "https://github.com/tanersener/mobile-ffmpeg/releases/download/vVERSION/mobile-ffmpeg-full-gpl-VERSION-ios-framework.zip" }

    s.ios.deployment_target = '9.3'
    s.ios.frameworks    = 'AudioToolbox','AVFoundation','CoreMedia','VideoToolbox'
    s.ios.vendored_frameworks = 'mobileffmpeg.framework', 'libavcodec.framework', 'libavdevice.framework', 'libavfilter.framework', 'libavformat.framework', 'libavutil.framework', 'libswresample.framework', 'libswscale.framework', 'expat.framework', 'fontconfig.framework', 'freetype.framework', 'fribidi.framework', 'giflib.framework', 'gmp.framework', 'gnutls.framework', 'jpeg.framework', 'kvazaar.framework', 'lame.framework', 'libaom.framework', 'libass.framework', 'libcharset.framework', 'libhogweed.framework', 'libiconv.framework', 'libilbc.framework', 'libnettle.framework', 'libogg.framework', 'libopencore-amrnb.framework', 'libopencore-amrwb.framework', 'libpng.framework', 'libsndfile.framework', 'libtheora.framework', 'libtheoradec.framework', 'libtheoraenc.framework', 'libuuid.framework', 'libvorbis.framework', 'libvorbisenc.framework', 'libvorbisfile.framework', 'libvpx.framework', 'libwebp.framework', 'libwebpdecoder.framework', 'libwebpdemux.framework', 'libxml2.framework', 'opus.framework', 'shine.framework', 'snappy.framework', 'soxr.framework', 'speex.framework', 'tiff.framework', 'twolame.framework', 'wavpack.framework', 'libvidstab.framework', 'x264.framework', 'x265.framework', 'xvidcore.framework'

end  
