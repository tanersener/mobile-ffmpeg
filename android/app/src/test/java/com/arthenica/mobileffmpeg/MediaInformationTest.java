/*
 * Copyright (c) 2018 Taner Sener
 *
 * This file is part of MobileFFmpeg.
 *
 * MobileFFmpeg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MobileFFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.arthenica.mobileffmpeg;

import com.arthenica.mobileffmpeg.util.Pair;
import com.arthenica.mobileffmpeg.util.Trio;

import junit.framework.Assert;

import org.junit.Test;

public class MediaInformationTest {

    @Test
    public void parseVideoStream() {
        parseVideoStreamBlock("     Stream #0:0: Video: mjpeg, yuvj420p(pc, bt470bg/unknown/unknown), 2560x1708 [SAR 1:1 DAR 640:427], 25 tbr, 25 tbn, 25 tbc", 0L, "mjpeg", "mjpeg", "yuvj420p", "yuvj420p(pc, bt470bg/unknown/unknown)", 2560L, 1708L, "1:1", "640:427", null, null, "25", "25", "25");
        parseVideoStreamBlock("     Stream #0:0: Video: gif, bgra, 420x236, 6 fps, 6 tbr, 100 tbn, 100 tbc", 0L, "gif",
                "gif", "bgra", "bgra", 420L, 236L, null, null, null, "6", "6", "100", "100");
        parseVideoStreamBlock("     Stream #0:0(und): Video: h264 (Main) (avc1 / 0x31637661), yuv420p, 1280x720 [SAR 1:1 DAR 16:9], 7762 kb/s, 25 fps, 30 tbr, 15360 tbn, 60 tbc (default)", 0L, "h264", "h264 (main) (avc1 / 0x31637661)", "yuv420p", "yuv420p", 1280L, 720L, "1:1", "16:9", 7762L, "25", "30", "15360", "60");
        parseVideoStreamBlock("     Stream #0:0: Video: png, rgba(pc), 544x184, 25 tbr, 25 tbn, 25 tbc", 0L, "png", "png", "rgba", "rgba(pc)", 544L, 184L, null, null, null, null, "25", "25", "25");
        parseVideoStreamBlock("     Stream #0:0: Video: h264 (Main), yuv420p(tv, bt709, progressive), 1920x1080, 25 fps, 25 tbr, 1200k tbn, 50 tbc", 0L, "h264", "h264 (main)", "yuv420p", "yuv420p(tv, bt709, progressive)", 1920L, 1080L, null, null, null, "25", "25", "1200k", "50");
        parseVideoStreamBlock("     Stream #0:0(und): Video: h264 (High) (avc1 / 0x31637661), yuv420p, 3840x4320 [SAR 1:1 DAR 8:9], 9902 kb/s, 30 fps, 30 tbr, 30k tbn, 60 tbc (default)", 0L, "h264", "h264 (high) (avc1 / 0x31637661)", "yuv420p", "yuv420p", 3840L, 4320L, "1:1", "8:9", 9902L, "30", "30", "30k", "60");
        parseVideoStreamBlock("     Stream #0:0(und): Video: h264 (High) (avc1 / 0x31637661), yuv420p, 1920x1080 [SAR 1:1 DAR 16:9], 3992 kb/s, 30 fps, 30 tbr, 30k tbn, 60 tbc (default)", 0L, "h264", "h264 (high) (avc1 / 0x31637661)", "yuv420p", "yuv420p", 1920L, 1080L, "1:1", "16:9", 3992L, "30", "30", "30k", "60");
        parseVideoStreamBlock("     Stream #0:0: Video: theora, yuv420p(bt470bg/bt470bg/bt709), 720x400, 25 fps, 25 tbr, 25 tbn, 25 tbc", 0L, "theora", "theora", "yuv420p", "yuv420p(bt470bg/bt470bg/bt709)", 720L, 400L, null, null, null, "25", "25", "25", "25");
    }

    @Test
    public void parseAudioStream() {
        parseAudioStreamBlock("Stream #0:0: Audio: adpcm_ms ([2][0][0][0] / 0x0002), 22050 Hz, stereo, s16, 176 kb/s", 0L, "adpcm_ms", "adpcm_ms ([2][0][0][0] / 0x0002)", 22050L, "stereo", "s16", 176L);
        parseAudioStreamBlock("Stream #0:0: Audio: pcm_s16le ([1][0][0][0] / 0x0001), 44100 Hz, mono, s16, 705 kb/s", 0L, "pcm_s16le", "pcm_s16le ([1][0][0][0] / 0x0001)", 44100L, "mono", "s16", 705L);
        parseAudioStreamBlock("Stream #0:0: Audio: pcm_s24le ([1][0][0][0] / 0x0001), 48000 Hz, mono, s32 (24 bit), 1152 kb/s", 0L, "pcm_s24le", "pcm_s24le ([1][0][0][0] / 0x0001)", 48000L, "mono", "s32 (24 bit)", 1152L);
        parseAudioStreamBlock("Stream #0:0: Audio: mp3, 48000 Hz, stereo, fltp, 192 kb/s", 0L, "mp3", "mp3", 48000L, "stereo", "fltp", 192L);
        parseAudioStreamBlock("Stream #0:0: Audio: vorbis, 44100 Hz, stereo, fltp, 128 kb/s", 0L, "vorbis", "vorbis", 44100L, "stereo", "fltp", 128L);
        parseAudioStreamBlock("Stream #0:0: Audio: pcm_u8 ([1][0][0][0] / 0x0001), 44100 Hz, stereo, u8, 705 kb/s", 0L, "pcm_u8", "pcm_u8 ([1][0][0][0] / 0x0001)", 44100L, "stereo", "u8", 705L);
        parseAudioStreamBlock("Stream #0:1(und): Audio: mp3 (mp4a / 0x6134706D), 48000 Hz, stereo, fltp, 160 kb/s (default)", 1L, "mp3", "mp3 (mp4a / 0x6134706d)", 48000L, "stereo", "fltp", 160L);
        parseAudioStreamBlock("Stream #0:2(und): Audio: ac3 (ac-3 / 0x332D6361), 48000 Hz, 5.1(side), fltp, 320 kb/s (default)", 2L, "ac3", "ac3 (ac-3 / 0x332d6361)", 48000L, "5.1(side)", "fltp", 320L);
        parseAudioStreamBlock("Stream #0:1(und): Audio: mp3 (mp4a / 0x6134706D), 48000 Hz, stereo, fltp, 160 kb/s (default)", 1L, "mp3", "mp3 (mp4a / 0x6134706d)", 48000L, "stereo", "fltp", 160L);
        parseAudioStreamBlock("Stream #0:2(und): Audio: ac3 (ac-3 / 0x332D6361), 48000 Hz, 5.1(side), fltp, 320 kb/s (default)", 2L, "ac3", "ac3 (ac-3 / 0x332d6361)", 48000L, "5.1(side)", "fltp", 320L);
    }

    @Test
    public void index() {
        Assert.assertEquals(7, MediaInformationParser.index("one:two:three:", ":", 0, 2));
        Assert.assertEquals(13, MediaInformationParser.index("one:two:three:", ":", 0, 3));
        Assert.assertEquals(8, MediaInformationParser.index("one::two::three::", "::", 0, 2));
    }

    @Test
    public void count() {
        Assert.assertEquals(3, MediaInformationParser.count("one:two:three:", ":"));
        Assert.assertEquals(2, MediaInformationParser.count("one,two:three,four", ","));
    }

    @Test
    public void parseAverageFrameRate() {
        Assert.assertEquals("24", "24 fps".replaceAll("fps", "").trim());
        Assert.assertEquals("30", "30 fps".replaceAll("fps", "").trim());
    }

    @Test
    public void parseBitrate() {
        Assert.assertEquals((Long) 3992L, MediaInformationParser.toLongObject("3992 kb/s".replaceAll("kb/s", "").trim()));
        Assert.assertEquals((Long) 7762L, MediaInformationParser.toLongObject("7762 kb/s".replaceAll("kb/s", "").trim()));
    }

    @Test
    public void parseVideoStreamDisplayAspectRatio() {
        Assert.assertEquals(null, MediaInformationParser.parseVideoStreamDisplayAspectRatio(""));
        Assert.assertEquals(null, MediaInformationParser.parseVideoStreamDisplayAspectRatio("544x184"));
        Assert.assertEquals("640:427", MediaInformationParser.parseVideoStreamDisplayAspectRatio("2560x1708 [SAR 1:1 DAR 640:427]"));
        Assert.assertEquals("8:9", MediaInformationParser.parseVideoStreamDisplayAspectRatio("3840x4320 [SAR 1:1 DAR 8:9]"));
    }

    @Test
    public void parseVideoStreamSampleAspectRatio() {
        Assert.assertEquals(null, MediaInformationParser.parseVideoStreamSampleAspectRatio(""));
        Assert.assertEquals(null, MediaInformationParser.parseVideoStreamSampleAspectRatio("544x184"));
        Assert.assertEquals("1:1", MediaInformationParser.parseVideoStreamSampleAspectRatio("2560x1708 [SAR 1:1 DAR 640:427]"));
        Assert.assertEquals("1:1", MediaInformationParser.parseVideoStreamSampleAspectRatio("3840x4320 [SAR 1:1 DAR 8:9]"));
    }

    @Test
    public void parseVideoDimensions() {
        parseVideoDimensions("", null, null);
        parseVideoDimensions("544x184", 544L, 184L);
        parseVideoDimensions("720x400", 720L, 400L);
        parseVideoDimensions("2560x1708 [SAR 1:1 DAR 640:427]", 2560L, 1708L);
        parseVideoDimensions("3840x4320 [SAR 1:1 DAR 8:9]", 3840L, 4320L);
    }

    @Test
    public void parseAudioSampleRate() {
        Assert.assertEquals((Long) 44000L, MediaInformationParser.parseAudioStreamSampleRate("44000"));
        Assert.assertEquals((Long) 44000L, MediaInformationParser.parseAudioStreamSampleRate("44 khz"));
        Assert.assertEquals((Long) 44100L, MediaInformationParser.parseAudioStreamSampleRate("44100"));
        Assert.assertEquals((Long) 5000000L, MediaInformationParser.parseAudioStreamSampleRate("5 mhz"));
    }

    @Test
    public void parseAudioStreamType() {
        Assert.assertEquals("video", MediaInformationParser.parseStreamType("Video: theora"));
        Assert.assertEquals("video", MediaInformationParser.parseStreamType("Video: png"));
        Assert.assertEquals("video", MediaInformationParser.parseStreamType("Video: h264 (Main)"));
        Assert.assertEquals("audio", MediaInformationParser.parseStreamType("Audio: adpcm_ms ([2][0][0][0] / 0x0002)"));
        Assert.assertEquals("audio", MediaInformationParser.parseStreamType("Audio: mp3 (mp4a / 0x6134706D)"));
        Assert.assertEquals("audio", MediaInformationParser.parseStreamType("Audio: pcm_u8 ([1][0][0][0] / 0x0001)"));
    }

    @Test
    public void parseStreamCodec() {
        Assert.assertEquals("theora", MediaInformationParser.parseStreamCodec("Video: theora"));
        Assert.assertEquals("png", MediaInformationParser.parseStreamCodec("Video: png"));
        Assert.assertEquals("h264", MediaInformationParser.parseStreamCodec("Video: h264 (Main)"));
        Assert.assertEquals("adpcm_ms", MediaInformationParser.parseStreamCodec("Audio: adpcm_ms ([2][0][0][0] / 0x0002)"));
        Assert.assertEquals("mp3", MediaInformationParser.parseStreamCodec("Audio: mp3 (mp4a / 0x6134706D)"));
        Assert.assertEquals("pcm_u8", MediaInformationParser.parseStreamCodec("Audio: pcm_u8 ([1][0][0][0] / 0x0001)"));
    }

    @Test
    public void parseStreamFullCodec() {
        Assert.assertEquals("theora", MediaInformationParser.parseStreamFullCodec("Video: theora"));
        Assert.assertEquals("png", MediaInformationParser.parseStreamFullCodec("Video: png"));
        Assert.assertEquals("h264 (main)", MediaInformationParser.parseStreamFullCodec("Video: h264 (Main)"));
        Assert.assertEquals("adpcm_ms ([2][0][0][0] / 0x0002)", MediaInformationParser.parseStreamFullCodec("Audio: adpcm_ms ([2][0][0][0] / 0x0002)"));
        Assert.assertEquals("mp3 (mp4a / 0x6134706d)", MediaInformationParser.parseStreamFullCodec("Audio: mp3 (mp4a / 0x6134706D)"));
        Assert.assertEquals("pcm_u8 ([1][0][0][0] / 0x0001)", MediaInformationParser.parseStreamFullCodec("Audio: pcm_u8 ([1][0][0][0] / 0x0001)"));
    }

    @Test
    public void parseStreamIndex() {
        Assert.assertEquals((Long) 0L, MediaInformationParser.parseStreamIndex("Stream #0:0(und): Audio"));
        Assert.assertEquals((Long) 1L, MediaInformationParser.parseStreamIndex("Stream #0:1(und): Video"));
        Assert.assertEquals((Long) 2L, MediaInformationParser.parseStreamIndex("Stream #0:2(und): Audio"));
        Assert.assertEquals((Long) 0L, MediaInformationParser.parseStreamIndex("Stream #0:0: Video"));
        Assert.assertEquals((Long) 1L, MediaInformationParser.parseStreamIndex("Stream #0:1: Audio"));
        Assert.assertEquals((Long) 2L, MediaInformationParser.parseStreamIndex("Stream #0:2: Video"));
    }

    @Test
    public void parseDuration() {
        parseDuration(null, null);
        parseDuration("", null);
        parseDuration("N/A", null);
        parseDuration("00:03:33.24", 213240L);
        parseDuration("00:10:34.53", 634530L);
        parseDuration("00:00:00.04", 40L);
        parseDuration("00:00:15.00", 15000L);
    }

    @Test
    public void parseStartTime() {
        parseStartTime(null, null);
        parseStartTime("", null);
        parseStartTime("0.000000", 0L);
        parseStartTime("10.003000", 10003L);
        parseStartTime("324.000000", 324000L);
        parseStartTime("-4.000000", -4000L);
        parseStartTime("14.00030", 14000L);
        parseStartTime("14.00080", 14001L);
    }

    @Test
    public void parseDurationBlock() {
        parseDurationBlock("  Duration: 00:03:33.24, start: 0.000000, bitrate: 320 kb/s", 213240L, 0L, 320L);
        parseDurationBlock("  Duration: 00:00:00.04, start: 0.000000, bitrate: 391187 kb/s", 40L, 0L, 391187L);
        parseDurationBlock("  Duration: N/A, bitrate: N/A", null, null, null);
        parseDurationBlock("  Duration: 00:00:15.00, start: 0.000000, bitrate: 7764 kb/s", 15000L, 0L, 7764L);
        parseDurationBlock("  Duration: 00:10:34.53, start: 0.000000, bitrate: 4474 kb/s", 634530L, 0L, 4474L);
    }

    @Test
    public void parseInputBlock() {
        parseInputBlock("Input #0,", null, null);
        parseInputBlock("Input #0, ogg, from 'trailer_400p.ogg':", "ogg", "trailer_400p.ogg");
        parseInputBlock("Input #0, mp3, from 'cold_stone_-_planet_x.mp3':", "mp3", "cold_stone_-_planet_x.mp3");
        parseInputBlock("Input #0, image2, from '/data/user/0/com.arthenica.mobileffmpeg.test/cache/colosseum.jpg':", "image2", "/data/user/0/com.arthenica.mobileffmpeg.test/cache/colosseum.jpg");
        parseInputBlock("Input #0, gif, from 'advanced_zoom_in_and_pan_with_fade_in_out.gif':", "gif", "advanced_zoom_in_and_pan_with_fade_in_out.gif");
        parseInputBlock("Input #0, mov,mp4,m4a,3gp,3g2,mj2, from 'transition_rotate.mp4':", "mov,mp4,m4a,3gp,3g2,mj2", "transition_rotate.mp4");
        parseInputBlock("Input #0, png_pipe, from 'https://www.google.com/images/branding/googlelogo/2x/googlelogo_color_272x92dp.png':", "png_pipe", "https://www.google.com/images/branding/googlelogo/2x/googlelogo_color_272x92dp.png");
    }

    @Test
    public void parseMetadataBlock() {
        parseMetadataBlock("      ENCODER:", "ENCODER", "");
        parseMetadataBlock("      ENCODER:ffmpeg2theora 0.19", "ENCODER", "ffmpeg2theora 0.19");
        parseMetadataBlock("      ENCODER         : ffmpeg2theora 0.19", "ENCODER", "ffmpeg2theora 0.19");
        parseMetadataBlock("      creation_time   : 2013-12-16T17:50:04.000000Z", "creation_time", "2013-12-16T17:50:04.000000Z");
        parseMetadataBlock("      handler_name    : GPAC ISO Audio Handler", "handler_name", "GPAC ISO Audio Handler");
        parseMetadataBlock("    comment         : Creative Commons Attribution 3.0 - http://bbb3d.renderfarming.net", "comment", "Creative Commons Attribution 3.0 - http://bbb3d.renderfarming.net");
        parseMetadataBlock("    minor_version   : 1", "minor_version", "1");
        parseMetadataBlock("    encoder         : Lavf58.12.100", "encoder", "Lavf58.12.100");
        parseMetadataBlock("    title           : Planet X", "title", "Planet X");
        parseMetadataBlock("    compatible_brands: isomiso2avc1mp41", "compatible_brands", "isomiso2avc1mp41");
    }

    private void parseVideoStreamBlock(String input, Long index, String codec, String fullCodec, String format, String fullFormat, Long width, Long height, String sampleAspectRatio, String displayAspectRatio, Long bitrate, String averageFrameRate, String realFrameRate, String timeBase, String codecTimeBase) {
        StreamInformation stream = MediaInformationParser.parseStreamBlock(input);

        Assert.assertNotNull(stream);
        Assert.assertEquals(index, stream.getIndex());
        Assert.assertEquals("video", stream.getType());
        Assert.assertEquals(codec, stream.getCodec());
        Assert.assertEquals(fullCodec, stream.getFullCodec());

        Assert.assertEquals(format, stream.getFormat());
        Assert.assertEquals(fullFormat, stream.getFullFormat());

        Assert.assertEquals(width, stream.getWidth());
        Assert.assertEquals(height, stream.getHeight());
        Assert.assertEquals(sampleAspectRatio, stream.getSampleAspectRatio());
        Assert.assertEquals(displayAspectRatio, stream.getDisplayAspectRatio());

        Assert.assertEquals(bitrate, stream.getBitrate());

        Assert.assertEquals(averageFrameRate, stream.getAverageFrameRate());
        Assert.assertEquals(realFrameRate, stream.getRealFrameRate());
        Assert.assertEquals(timeBase, stream.getTimeBase());
        Assert.assertEquals(codecTimeBase, stream.getCodecTimeBase());
    }

    private void parseAudioStreamBlock(String input, Long index, String codec, String fullCodec, Long sampleRate, String channelLayout, String sampleFormat, Long bitrate) {
        StreamInformation stream = MediaInformationParser.parseStreamBlock(input);

        Assert.assertNotNull(stream);
        Assert.assertEquals(index, stream.getIndex());
        Assert.assertEquals("audio", stream.getType());
        Assert.assertEquals(codec, stream.getCodec());
        Assert.assertEquals(fullCodec, stream.getFullCodec());
        Assert.assertEquals(sampleRate, stream.getSampleRate());
        Assert.assertEquals(channelLayout, stream.getChannelLayout());
        Assert.assertEquals(sampleFormat, stream.getSampleFormat());
        Assert.assertEquals(bitrate, stream.getBitrate());
    }

    private void parseVideoDimensions(String value, Long width, Long height) {
        Pair<Long, Long> videoDimensions = MediaInformationParser.parseVideoDimensions(value);

        Assert.assertNotNull(videoDimensions);
        Assert.assertEquals(width, videoDimensions.getFirst());
        Assert.assertEquals(height, videoDimensions.getSecond());
    }

    private void parseDuration(String value, Long expected) {
        Long duration = MediaInformationParser.parseDuration(value);

        Assert.assertEquals(expected, duration);
    }

    private void parseStartTime(String value, Long expected) {
        Long duration = MediaInformationParser.parseStartTime(value);

        Assert.assertEquals(expected, duration);
    }

    private void parseDurationBlock(String value, Long first, Long second, Long third) {
        Trio<Long, Long, Long> pair = MediaInformationParser.parseDurationBlock(value);

        Assert.assertNotNull(pair);
        Assert.assertEquals(first, pair.getFirst());
        Assert.assertEquals(second, pair.getSecond());
        Assert.assertEquals(third, pair.getThird());
    }

    private void parseInputBlock(String value, String first, String second) {
        Pair<String, String> pair = MediaInformationParser.parseInputBlock(value);

        Assert.assertNotNull(pair);
        Assert.assertEquals(first, pair.getFirst());
        Assert.assertEquals(second, pair.getSecond());
    }

    private void parseMetadataBlock(String value, String first, String second) {
        Pair<String, String> pair = MediaInformationParser.parseMetadataBlock(value);

        Assert.assertNotNull(pair);
        Assert.assertNotNull(pair.getFirst());
        Assert.assertNotNull(pair.getSecond());
        Assert.assertEquals(first, pair.getFirst());
        Assert.assertEquals(second, pair.getSecond());
    }

}
