Pod::Spec.new do |s|  
    s.name              = "mobile-ffmpeg-https-gpl"
    s.version           = "VERSION"
    s.summary           = "Mobile FFmpeg Https GPL Static Framework"
    s.description       = <<-DESC
    Includes FFmpeg v4.2-dev-1156 with gmp v6.1.2, gnutls v3.5.19, libvid.stab v1.1.0, x264 20190328-2245-stable, x265 v3.0 and xvidcore v1.3.5 libraries enabled.
    DESC

    s.homepage          = "https://github.com/tanersener/mobile-ffmpeg"

    s.author            = { "Taner Sener" => "tanersener@gmail.com" }
    s.license           = { :type => "GPL-3.0", :file => "mobileffmpeg.framework/LICENSE" }

    s.platform          = :ios
    s.platform          = :tvos

    s.requires_arc      = true
    s.libraries         = 'z', 'bz2', 'c++'

    s.source            = { :http => "https://github.com/tanersener/mobile-ffmpeg/releases/download/vVERSION/mobile-ffmpeg-https-gpl-VERSION-ios-tvos-framework.zip" }

    s.ios.deployment_target = '9.3'
    s.ios.frameworks    = 'AudioToolbox','CoreMedia','VideoToolbox'
    s.ios.vendored_frameworks  = 'ios/mobileffmpeg.framework', 'ios/libavcodec.framework', 'ios/libavdevice.framework', 'ios/libavfilter.framework', 'ios/libavformat.framework', 'ios/libavutil.framework', 'ios/libswresample.framework', 'ios/libswscale.framework', 'ios/gmp.framework', 'ios/gnutls.framework', 'ios/libcharset.framework', 'ios/libhogweed.framework', 'ios/libiconv.framework', 'ios/libnettle.framework', 'ios/libvidstab.framework', 'ios/x264.framework', 'ios/x265.framework', 'ios/xvidcore.framework'
    s.ios.preserve_paths       = 'ios/*.framework'

    s.tvos.deployment_target = '9.2'
    s.tvos.frameworks   = 'AudioToolbox','CoreMedia','VideoToolbox'
    s.tvos.vendored_frameworks  = 'tvos/mobileffmpeg.framework', 'tvos/libavcodec.framework', 'tvos/libavdevice.framework', 'tvos/libavfilter.framework', 'tvos/libavformat.framework', 'tvos/libavutil.framework', 'tvos/libswresample.framework', 'tvos/libswscale.framework', 'tvos/gmp.framework', 'tvos/gnutls.framework', 'tvos/libcharset.framework', 'tvos/libhogweed.framework', 'tvos/libiconv.framework', 'tvos/libnettle.framework', 'tvos/libvidstab.framework', 'tvos/x264.framework', 'tvos/x265.framework', 'tvos/xvidcore.framework'
    s.tvos.preserve_paths       = 'tvos/*.framework'

end  
