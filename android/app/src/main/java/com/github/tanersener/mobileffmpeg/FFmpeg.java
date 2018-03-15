package com.github.tanersener.mobileffmpeg;

public class FFmpeg {

    public native String getFFmpegVersion();

    public native String getVersion();

    public native String getABI();

    public native String execute(final String ... arguments);

}
