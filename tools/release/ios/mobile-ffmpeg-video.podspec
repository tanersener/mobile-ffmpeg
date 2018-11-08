Pod::Spec.new do |s|  
    s.name              = 'mobile-ffmpeg-video'
    s.version           = 'VERSION'
    s.summary           = 'Mobile FFmpeg Video Dynamic Framework'
    s.description       = 'Includes FFmpeg v4.0.3 with fontconfig v2.13.1, freetype v2.9.1, fribidi v1.0.5, kvazaar v1.2.0, libaom v1.0.0-898, libass v0.14.0, libiconv v1.15, libtheora v1.1.1, libvpx v1.7.0 and snappy v1.1.7 libraries enabled.'
    s.homepage          = 'https://github.com/tanersener/mobile-ffmpeg'

    s.author            = { 'Taner Sener' => 'tanersener@gmail.com' }
    s.license           = { :type => 'LGPL-3.0', :file => 'mobileffmpeg.framework/LICENSE' }

    s.platform          = :ios
    s.requires_arc      = true
    s.ios.deployment_target = '8.0'
    s.ios.frameworks    = 'Foundation', 'AudioToolbox', 'CoreImage'
    s.ios.library       = 'z', 'bz2'
      
    s.source            = { :http => 'https://github.com/tanersener/mobile-ffmpeg/releases/download/v2.1.1/mobile-ffmpeg-video-2.1.1-ios-framework.zip' }
    s.ios.vendored_frameworks = 'mobileffmpeg.framework', 'libavcodec.framework', 'libavdevice.framework', 'libavfilter.framework', 'libavformat.framework', 'libavutil.framework', 'libswresample.framework', 'libswscale.framework'

end  
