Pod::Spec.new do |s|  
    s.name              = 'mobile-ffmpeg-audio'
    s.version           = 'VERSION'
    s.summary           = 'Mobile FFmpeg Audio Dynamic Framework'
    s.description       = 'Includes FFmpeg v4.0.2 with chromaprint v1.4.3, lame v3.100, libilbc v2.0.2, libvorbis v1.3.6, opencore-amr v0.1.5, opus v1.2.1, shine v3.1.1, soxr v0.1.3, speex v1.2.0 and wavpack v5.1.0 libraries enabled.'
    s.homepage          = 'https://github.com/tanersener/mobile-ffmpeg'

    s.author            = { 'Taner Sener' => 'tanersener@gmail.com' }
    s.license           = { :type => 'LGPL-3.0', :file => 'mobileffmpeg.framework/LICENSE' }

    s.platform          = :ios
    s.requires_arc      = true
    s.ios.deployment_target = '8.0'
    s.ios.frameworks    = 'Foundation', 'AudioToolbox', 'CoreImage'
    s.ios.library       = 'z', 'bz2'
      
    s.source            = { :http => 'https://github.com/tanersener/mobile-ffmpeg/releases/download/v2.1/mobile-ffmpeg-audio-2.1-ios-framework.zip' }
    s.ios.vendored_frameworks = 'mobileffmpeg.framework', 'libavcodec.framework', 'libavdevice.framework', 'libavfilter.framework', 'libavformat.framework', 'libavutil.framework', 'libswresample.framework', 'libswscale.framework'

end  
