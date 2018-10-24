Pod::Spec.new do |s|  
    s.name              = 'mobile-ffmpeg-https-gpl'
    s.version           = 'VERSION'
    s.summary           = 'Mobile FFmpeg Https GPL Dynamic Framework'
    s.description       = 'Includes FFmpeg v4.1-dev-1517 with gmp v6.1.2, gnutls v3.5.19, libvid.stab v1.1.0, x264 20181015-2245-stable, x265 v2.9 and xvidcore v1.3.5 libraries enabled.'
    s.homepage          = 'https://github.com/tanersener/mobile-ffmpeg'

    s.author            = { 'Taner Sener' => 'tanersener@gmail.com' }
    s.license           = { :type => 'GPL-3.0', :file => 'mobileffmpeg.framework/LICENSE' }

    s.platform          = :ios
    s.requires_arc      = true
    s.ios.deployment_target = '8.0'
    s.ios.frameworks    = 'Foundation', 'AudioToolbox', 'CoreImage', 'CoreMotion', 'CoreVideo', 'GameController', 'VideoToolbox'
    s.ios.library       = 'z', 'bz2', 'c++'
      
    s.source            = { :http => 'https://github.com/tanersener/mobile-ffmpeg/releases/download/v3.0/mobile-ffmpeg-https-gpl-3.0-ios-framework.zip' }
    s.ios.vendored_frameworks = 'mobileffmpeg.framework', 'libavcodec.framework', 'libavdevice.framework', 'libavfilter.framework', 'libavformat.framework', 'libavutil.framework', 'libswresample.framework', 'libswscale.framework'

end  
