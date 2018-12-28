Pod::Spec.new do |s|  
    s.name              = 'mobile-ffmpeg-audio'
    s.version           = 'VERSION'
    s.summary           = 'Mobile FFmpeg Audio Static Framework'
    s.description       = 'Includes FFmpeg v4.2-dev-480 with lame v3.100, libilbc v2.0.2, libvorbis v1.3.6, opencore-amr v0.1.5, opus v1.3, shine v3.1.1, soxr v0.1.3, speex v1.2.0, twolame v0.3.13 and wavpack v5.1.0 libraries enabled.'
    s.homepage          = 'https://github.com/tanersener/mobile-ffmpeg'

    s.author            = { 'Taner Sener' => 'tanersener@gmail.com' }
    s.license           = { :type => 'LGPL-3.0', :file => 'mobileffmpeg.framework/LICENSE' }

    s.platform          = :ios
    s.requires_arc      = true
    s.ios.deployment_target = '9.3'
    s.ios.frameworks    = 'AudioToolbox','AVFoundation','CoreMedia','VideoToolbox'
    s.libraries         = 'z', 'bz2', 'c++'
      
    s.source            = { :http => 'https://github.com/tanersener/mobile-ffmpeg/releases/download/vVERSION/mobile-ffmpeg-audio-VERSION-ios-framework.zip' }
    s.ios.vendored_frameworks = 'mobileffmpeg.framework', 'libavcodec.framework', 'libavdevice.framework', 'libavfilter.framework', 'libavformat.framework', 'libavutil.framework', 'libswresample.framework', 'libswscale.framework', 'lame.framework', 'libcharset.framework', 'libiconv.framework', 'libilbc.framework', 'libogg.framework', 'libopencore-amrnb.framework', 'libopencore-amrwb.framework', 'libsndfile.framework', 'libvorbis.framework', 'libvorbisenc.framework', 'libvorbisfile.framework', 'opus.framework', 'shine.framework', 'soxr.framework', 'speex.framework', 'twolame.framework', 'wavpack.framework'

end  
