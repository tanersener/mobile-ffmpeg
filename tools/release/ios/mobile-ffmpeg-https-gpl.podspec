Pod::Spec.new do |s|  
    s.name              = 'mobile-ffmpeg-https-gpl'
    s.version           = 'VERSION'
    s.summary           = 'Mobile FFmpeg Https GPL Dynamic Framework'
    s.description       = 'Includes FFmpeg v4.0.1 with gnutls v3.5.18, x264 v20180627-2245-stable and xvidcore v1.3.5 libraries enabled.'
    s.homepage          = 'https://github.com/tanersener/mobile-ffmpeg'

    s.author            = { 'Taner Sener' => 'tanersener@gmail.com' }
    s.license           = { :type => 'GPL-3.0', :file => 'mobileffmpeg.framework/LICENSE' }

    s.platform          = :ios
    s.requires_arc      = true
    s.ios.deployment_target = '8.0'
    s.ios.frameworks    = 'Foundation'
      
    s.source            = { :http => 'https://github.com/tanersener/mobile-ffmpeg/releases/download/v2.0/mobile-ffmpeg-https-gpl-2.0-ios-framework.zip' }
    s.ios.vendored_frameworks = 'mobileffmpeg.framework', 'libavcodec.framework', 'libavdevice.framework', 'libavfilter.framework', 'libavformat.framework', 'libavutil.framework', 'libswresample.framework', 'libswscale.framework'

end  
