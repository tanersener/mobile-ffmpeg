Pod::Spec.new do |s|  
    s.name              = "mobile-ffmpeg-tvos-min-gpl"
    s.version           = "VERSION"
    s.summary           = "Mobile FFmpeg tvOS Min GPL Static Framework"
    s.description       = <<-DESC
    Includes FFmpeg v4.2-dev-1824 with libvid.stab v1.1.0, x264 20190701-2245-stable, x265 v3.0 and xvidcore v1.3.5 libraries enabled.
    DESC

    s.homepage          = "https://github.com/tanersener/mobile-ffmpeg"

    s.author            = { "Taner Sener" => "tanersener@gmail.com" }
    s.license           = { :type => "GPL-3.0", :file => "mobileffmpeg.framework/LICENSE" }

    s.platform          = :tvos
    s.requires_arc      = true
    s.libraries         = 'z', 'bz2', 'c++'

    s.source            = { :http => "https://github.com/tanersener/mobile-ffmpeg/releases/download/vVERSION/mobile-ffmpeg-min-gpl-VERSION-tvos-framework.zip" }

    s.tvos.deployment_target = '9.2'
    s.tvos.frameworks   = 'AudioToolbox','VideoToolbox','CoreMedia'
    s.tvos.vendored_frameworks = 'mobileffmpeg.framework', 'libavcodec.framework', 'libavdevice.framework', 'libavfilter.framework', 'libavformat.framework', 'libavutil.framework', 'libswresample.framework', 'libswscale.framework', 'libvidstab.framework', 'x264.framework', 'x265.framework', 'xvidcore.framework'

end  
