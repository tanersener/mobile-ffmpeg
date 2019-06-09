Pod::Spec.new do |s|  
    s.name              = "mobile-ffmpeg-audio"
    s.version           = "VERSION"
    s.summary           = "Mobile FFmpeg Audio Static Framework"
    s.description       = <<-DESC
    Includes FFmpeg v4.2-dev-1156 with lame v3.100, libilbc v2.0.2, libvorbis v1.3.6, opencore-amr v0.1.5, opus v1.3, shine v3.1.1, soxr v0.1.3, speex v1.2.0, twolame v0.3.13 and wavpack v5.1.0 libraries enabled.
    DESC

    s.homepage          = "https://github.com/tanersener/mobile-ffmpeg"

    s.author            = { "Taner Sener" => "tanersener@gmail.com" }
    s.license           = { :type => "LGPL-3.0", :file => "mobileffmpeg.framework/LICENSE" }

    s.platform          = :ios
    s.platform          = :tvos

    s.requires_arc      = true
    s.libraries         = 'z', 'bz2', 'c++'

    s.source            = { :http => "https://github.com/tanersener/mobile-ffmpeg/releases/download/vVERSION/mobile-ffmpeg-audio-VERSION-ios-tvos-framework.zip" }

    s.ios.deployment_target = '9.3'
    s.ios.frameworks    = 'AudioToolbox','CoreMedia','VideoToolbox'
    s.ios.vendored_frameworks  = 'ios/mobileffmpeg.framework', 'ios/libavcodec.framework', 'ios/libavdevice.framework', 'ios/libavfilter.framework', 'ios/libavformat.framework', 'ios/libavutil.framework', 'ios/libswresample.framework', 'ios/libswscale.framework', 'ios/lame.framework', 'ios/libcharset.framework', 'ios/libiconv.framework', 'ios/libilbc.framework', 'ios/libogg.framework', 'ios/libopencore-amrnb.framework', 'ios/libopencore-amrwb.framework', 'ios/libsndfile.framework', 'ios/libvorbis.framework', 'ios/libvorbisenc.framework', 'ios/libvorbisfile.framework', 'ios/opus.framework', 'ios/shine.framework', 'ios/soxr.framework', 'ios/speex.framework', 'ios/twolame.framework', 'ios/wavpack.framework'
    s.ios.preserve_paths       = 'ios/*.framework'

    s.tvos.deployment_target = '9.2'
    s.tvos.frameworks   = 'AudioToolbox','CoreMedia','VideoToolbox'
    s.tvos.vendored_frameworks  = 'tvos/mobileffmpeg.framework', 'tvos/libavcodec.framework', 'tvos/libavdevice.framework', 'tvos/libavfilter.framework', 'tvos/libavformat.framework', 'tvos/libavutil.framework', 'tvos/libswresample.framework', 'tvos/libswscale.framework', 'tvos/lame.framework', 'tvos/libcharset.framework', 'tvos/libiconv.framework', 'tvos/libilbc.framework', 'tvos/libogg.framework', 'tvos/libopencore-amrnb.framework', 'tvos/libopencore-amrwb.framework', 'tvos/libsndfile.framework', 'tvos/libvorbis.framework', 'tvos/libvorbisenc.framework', 'tvos/libvorbisfile.framework', 'tvos/opus.framework', 'tvos/shine.framework', 'tvos/soxr.framework', 'tvos/speex.framework', 'tvos/twolame.framework', 'tvos/wavpack.framework'
    s.tvos.preserve_paths       = 'tvos/*.framework'

end  
