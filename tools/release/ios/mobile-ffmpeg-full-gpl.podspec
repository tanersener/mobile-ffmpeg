Pod::Spec.new do |s|  
    s.name              = 'mobile-ffmpeg-full-gpl'
    s.version           = 'VERSION'
    s.summary           = 'Mobile FFmpeg Full GPL Dynamic Framework'
    s.description       = 'Includes FFmpeg v3.4.4 with fontconfig v2.12.93, freetype v2.9.1, fribidi v1.0.5, gmp v6.1.2, gnutls v3.5.19, kvazaar v1.2.0, libiconv v1.15, lame v3.100, libass v0.14.0, libilbc v2.0.2, libtheora v1.1.1, libvorbis v1.3.5, libvpx v1.7.0, libwebp v0.6.1, libxml2 v2.9.7, opencore-amr v0.1.5, opus v1.2.1, shine v3.1.1, snappy v1.1.7, speex v1.2.0, wavpack v5.1.0, x264 v20180815-2245-stable and xvidcore v1.3.5 libraries enabled.'
    s.homepage          = 'https://github.com/tanersener/mobile-ffmpeg'

    s.author            = { 'Taner Sener' => 'tanersener@gmail.com' }
    s.license           = { :type => 'GPL-3.0', :file => 'mobileffmpeg.framework/LICENSE' }

    s.platform          = :ios
    s.requires_arc      = true
    s.ios.deployment_target = '8.0'
    s.ios.frameworks    = 'Foundation', 'AudioToolbox', 'CoreImage'
    s.ios.library       = 'z', 'bz2'
      
    s.source            = { :http => 'https://github.com/tanersener/mobile-ffmpeg/releases/download/v1.2/mobile-ffmpeg-full-gpl-1.2-ios-framework.zip' }
    s.ios.vendored_frameworks = 'mobileffmpeg.framework', 'libavcodec.framework', 'libavdevice.framework', 'libavfilter.framework', 'libavformat.framework', 'libavutil.framework', 'libswresample.framework', 'libswscale.framework'

end
