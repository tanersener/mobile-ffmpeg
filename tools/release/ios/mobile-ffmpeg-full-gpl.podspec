Pod::Spec.new do |s|  
    s.name              = 'mobile-ffmpeg-full-gpl'
    s.version           = 'VERSION'
    s.summary           = 'Mobile FFmpeg Full GPL Dynamic Framework'
    s.description       = 'Includes FFmpeg v4.1-dev-1517 with chromaprint v1.4.3, fontconfig v2.13.1, freetype v2.9.1, fribidi v1.0.5, gmp v6.1.2, gnutls v3.5.19, kvazaar v1.2.0, lame v3.100, libaom v1.0.0-dev-820, libass v0.14.0, libiconv v1.15, libilbc v2.0.2, libtheora v1.1.1, libvid.stab v1.1.0, libvorbis v1.3.6, libvpx v1.7.0, libwebp v1.0.0, libxml2 v2.9.8, opencore-amr v0.1.5, opus v1.3, sdl v2.0.8, shine v3.1.1, snappy v1.1.7, soxr v0.1.3, speex v1.2.0, tesseract v3.05.02, twolame v0.3.13, wavpack v5.1.0, x264 20181015-2245-stable, x265 v2.9 and xvidcore v1.3.5 libraries enabled.'
    s.homepage          = 'https://github.com/tanersener/mobile-ffmpeg'

    s.author            = { 'Taner Sener' => 'tanersener@gmail.com' }
    s.license           = { :type => 'GPL-3.0', :file => 'mobileffmpeg.framework/LICENSE' }

    s.platform          = :ios
    s.requires_arc      = true
    s.ios.deployment_target = '8.0'
    s.ios.frameworks    = 'Foundation', 'AudioToolbox', 'CoreImage', 'CoreMotion', 'CoreVideo', 'GameController', 'VideoToolbox'
    s.ios.library       = 'z', 'bz2', 'c++'
      
    s.source            = { :http => 'https://github.com/tanersener/mobile-ffmpeg/releases/download/v3.0/mobile-ffmpeg-full-gpl-3.0-ios-framework.zip' }
    s.ios.vendored_frameworks = 'mobileffmpeg.framework', 'libavcodec.framework', 'libavdevice.framework', 'libavfilter.framework', 'libavformat.framework', 'libavutil.framework', 'libswresample.framework', 'libswscale.framework'

end  
