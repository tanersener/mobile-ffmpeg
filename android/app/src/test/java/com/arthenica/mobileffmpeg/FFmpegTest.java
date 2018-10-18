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

import org.junit.Assert;
import org.junit.Test;

/**
 * <p>Tests for {@link FFmpeg} class.
 */
public class FFmpegTest {

    public static final String MEDIA_INFORMATION_MP3 =
            "Unknown attached picture mimetype: audio/x-wav, skipping.\n" +
                    "[mp3 @ 0x7ffb94805800] Estimating duration from bitrate, this may be inaccurate\n" +
                    "Input #0, mp3, from 'cold_stone_-_planet_x.mp3':\n" +
                    "  Metadata:\n" +
                    "    comment         :  \n" +
                    "    album           : Planet X\n" +
                    "    compilation     : 0\n" +
                    "    date            : -1\n" +
                    "    title           : Planet X\n" +
                    "    artist          : Cold Stone\n" +
                    "    album_artist    : Cold Stone\n" +
                    "    track           : -1\n" +
                    "    lyrics-XXX      : \n" +
                    "  Duration: 00:03:33.24, start: 0.000000, bitrate: 320 kb/s\n" +
                    "    Stream #0:0: Audio: mp3, 48000 Hz, stereo, fltp, 320 kb/s\n" +
                    "Stream mapping:\n" +
                    "  Stream #0:0 -> #0:0 (mp3 (mp3float) -> pcm_s16le (native))\n" +
                    "Press [q] to stop, [?] for help\n" +
                    "Output #0, null, to 'pipe:':\n" +
                    "  Metadata:\n" +
                    "    comment         :  \n" +
                    "    album           : Planet X\n" +
                    "    compilation     : 0\n" +
                    "    date            : -1\n" +
                    "    title           : Planet X\n" +
                    "    artist          : Cold Stone\n" +
                    "    album_artist    : Cold Stone\n" +
                    "    track           : -1\n" +
                    "    lyrics-XXX      : \n" +
                    "    encoder         : Lavf58.12.100\n" +
                    "    Stream #0:0: Audio: pcm_s16le, 48000 Hz, stereo, s16, 1536 kb/s\n" +
                    "    Metadata:\n" +
                    "      encoder         : Lavc58.18.100 pcm_s16le\n" +
                    "size=N/A time=00:03:33.24 bitrate=N/A speed= 618x    \n" +
                    "video:0kB audio:39982kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown";

    public static final String MEDIA_INFORMATION_JPG =
            "Input #0, image2, from '/data/user/0/com.arthenica.mobileffmpeg.test/cache/colosseum.jpg':\n" +
                    "  Duration: 00:00:00.04, start: 0.000000, bitrate: 391187 kb/s\n" +
                    "    Stream #0:0: Video: mjpeg, yuvj420p(pc, bt470bg/unknown/unknown), 2560x1708 [SAR 1:1 DAR 640:427], 25 tbr, 25 tbn, 25 tbc\n" +
                    "Stream mapping:\n" +
                    "  Stream #0:0 -> #0:0 (mjpeg (native) -> wrapped_avframe (native))\n" +
                    "Press [q] to stop, [?] for help\n" +
                    "Output #0, null, to 'pipe:':\n" +
                    "  Metadata:\n" +
                    "    encoder         : Lavf58.18.104\n" +
                    "    Stream #0:0: Video: wrapped_avframe, yuvj420p, 2560x1708 [SAR 1:1 DAR 640:427], q=2-31, 200 kb/s, 25 fps, 25 tbn, 25 tbc\n" +
                    "    Metadata:\n" +
                    "      encoder         : Lavc58.31.102 wrapped_avframe\n" +
                    "frame=    1 fps=0.0 q=-0.0 Lsize=N/A time=00:00:00.04 bitrate=N/A speed=0.668x    \n" +
                    "video:0kB audio:0kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown\n";

    public static final String MEDIA_INFORMATION_GIF =
            "Input #0, gif, from 'advanced_zoom_in_and_pan_with_fade_in_out.gif':\n" +
                    "  Duration: N/A, bitrate: N/A\n" +
                    "    Stream #0:0: Video: gif, bgra, 420x236, 6 fps, 6 tbr, 100 tbn, 100 tbc\n" +
                    "Stream mapping:\n" +
                    "  Stream #0:0 -> #0:0 (gif (native) -> wrapped_avframe (native))\n" +
                    "Press [q] to stop, [?] for help\n" +
                    "Output #0, null, to 'pipe:':\n" +
                    "  Metadata:\n" +
                    "    encoder         : Lavf58.12.100\n" +
                    "    Stream #0:0: Video: wrapped_avframe, bgra, 420x236 [SAR 63:64 DAR 6615:3776], q=2-31, 200 kb/s, 6 fps, 6 tbn, 6 tbc\n" +
                    "    Metadata:\n" +
                    "      encoder         : Lavc58.18.100 wrapped_avframe\n" +
                    "frame=   61 fps=0.0 q=-0.0 Lsize=N/A time=00:00:10.16 bitrate=N/A speed= 219x    \n" +
                    "video:32kB audio:0kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown";

    public static final String MEDIA_INFORMATION_H264 =
            "Input #0, mov,mp4,m4a,3gp,3g2,mj2, from 'transition_rotate.mp4':\n" +
                    "  Metadata:\n" +
                    "    major_brand     : isom\n" +
                    "    minor_version   : 512\n" +
                    "    compatible_brands: isomiso2avc1mp41\n" +
                    "    encoder         : Lavf58.12.100\n" +
                    "  Duration: 00:00:15.00, start: 0.000000, bitrate: 7764 kb/s\n" +
                    "    Stream #0:0(und): Video: h264 (Main) (avc1 / 0x31637661), yuv420p, 1280x720 [SAR 1:1 DAR 16:9], 7762 kb/s, 25 fps, 30 tbr, 15360 tbn, 60 tbc (default)\n" +
                    "    Metadata:\n" +
                    "      handler_name    : VideoHandler\n" +
                    "Stream mapping:\n" +
                    "  Stream #0:0 -> #0:0 (h264 (native) -> wrapped_avframe (native))\n" +
                    "Press [q] to stop, [?] for help\n" +
                    "Output #0, null, to 'pipe:':\n" +
                    "  Metadata:\n" +
                    "    major_brand     : isom\n" +
                    "    minor_version   : 512\n" +
                    "    compatible_brands: isomiso2avc1mp41\n" +
                    "    encoder         : Lavf58.12.100\n" +
                    "    Stream #0:0(und): Video: wrapped_avframe, yuv420p, 1280x720 [SAR 1:1 DAR 16:9], q=2-31, 200 kb/s, 30 fps, 30 tbn, 30 tbc (default)\n" +
                    "    Metadata:\n" +
                    "      handler_name    : VideoHandler\n" +
                    "      encoder         : Lavc58.18.100 wrapped_avframe\n" +
                    "frame=  375 fps=0.0 q=-0.0 Lsize=N/A time=00:00:15.00 bitrate=N/A speed=35.9x    \n" +
                    "video:196kB audio:0kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown";

    public static final String MEDIA_INFORMATION_PNG =
            "Input #0, png_pipe, from 'https://www.google.com/images/branding/googlelogo/2x/googlelogo_color_272x92dp.png':\n" +
                    "  Duration: N/A, bitrate: N/A\n" +
                    "    Stream #0:0: Video: png, rgba(pc), 544x184, 25 tbr, 25 tbn, 25 tbc\n" +
                    "Stream mapping:\n" +
                    "  Stream #0:0 -> #0:0 (png (native) -> wrapped_avframe (native))\n" +
                    "Press [q] to stop, [?] for help\n" +
                    "Output #0, null, to 'pipe:':\n" +
                    "  Metadata:\n" +
                    "    encoder         : Lavf58.12.100\n" +
                    "    Stream #0:0: Video: wrapped_avframe, rgba, 544x184, q=2-31, 200 kb/s, 25 fps, 25 tbn, 25 tbc\n" +
                    "    Metadata:\n" +
                    "      encoder         : Lavc58.18.100 wrapped_avframe\n" +
                    "frame=    1 fps=0.0 q=-0.0 Lsize=N/A time=00:00:00.04 bitrate=N/A speed=27.1x    \n" +
                    "video:1kB audio:0kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown";

    public static final String MEDIA_INFORMATION_H264_2 =
            "Input #0, h264, from 'test.h264':\n" +
                    "  Duration: N/A, bitrate: N/A\n" +
                    "    Stream #0:0: Video: h264 (Main), yuv420p(tv, bt709, progressive), 1920x1080, 25 fps, 25 tbr, 1200k tbn, 50 tbc\n" +
                    "Stream mapping:\n" +
                    "  Stream #0:0 -> #0:0 (h264 (native) -> wrapped_avframe (native))\n" +
                    "Press [q] to stop, [?] for help\n" +
                    "Output #0, null, to 'pipe:':\n" +
                    "  Metadata:\n" +
                    "    encoder         : Lavf58.12.100\n" +
                    "    Stream #0:0: Video: wrapped_avframe, yuv420p, 1920x1080, q=2-31, 200 kb/s, 25 fps, 25 tbn, 25 tbc\n" +
                    "    Metadata:\n" +
                    "      encoder         : Lavc58.18.100 wrapped_avframe\n" +
                    "frame=  360 fps=0.0 q=-0.0 Lsize=N/A time=00:00:14.40 bitrate=N/A speed=25.5x    \n" +
                    "video:188kB audio:0kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown";

    public static final String MEDIA_INFORMATION_MP4 =
            "Input #0, mov,mp4,m4a,3gp,3g2,mj2, from 'http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_2160p_30fps_stereo_abl.mp4':\n" +
                    "  Metadata:\n" +
                    "    major_brand     : isom\n" +
                    "    minor_version   : 1\n" +
                    "    compatible_brands: isomavc1\n" +
                    "    creation_time   : 2013-12-16T17:21:55.000000Z\n" +
                    "    title           : Big Buck Bunny, Sunflower version\n" +
                    "    artist          : Blender Foundation 2008, Janus Bager Kristensen 2013\n" +
                    "    comment         : Creative Commons Attribution 3.0 - http://bbb3d.renderfarming.net\n" +
                    "    genre           : Animation\n" +
                    "    composer        : Sacha Goedegebure\n" +
                    "  Duration: 00:10:34.53, start: 0.000000, bitrate: 10385 kb/s\n" +
                    "    Stream #0:0(und): Video: h264 (High) (avc1 / 0x31637661), yuv420p, 3840x4320 [SAR 1:1 DAR 8:9], 9902 kb/s, 30 fps, 30 tbr, 30k tbn, 60 tbc (default)\n" +
                    "    Metadata:\n" +
                    "      creation_time   : 2013-12-16T17:21:55.000000Z\n" +
                    "      handler_name    : GPAC ISO Video Handler\n" +
                    "    Stream #0:1(und): Audio: mp3 (mp4a / 0x6134706D), 48000 Hz, stereo, fltp, 160 kb/s (default)\n" +
                    "    Metadata:\n" +
                    "      creation_time   : 2013-12-16T17:21:58.000000Z\n" +
                    "      handler_name    : GPAC ISO Audio Handler\n" +
                    "    Stream #0:2(und): Audio: ac3 (ac-3 / 0x332D6361), 48000 Hz, 5.1(side), fltp, 320 kb/s (default)\n" +
                    "    Metadata:\n" +
                    "      creation_time   : 2013-12-16T17:21:58.000000Z\n" +
                    "      handler_name    : GPAC ISO Audio Handler\n" +
                    "    Side data:\n" +
                    "      audio service type: main\n" +
                    "Stream mapping:\n" +
                    "  Stream #0:0 -> #0:0 (h264 (native) -> wrapped_avframe (native))\n" +
                    "  Stream #0:2 -> #0:1 (ac3 (native) -> pcm_s16le (native))\n" +
                    "Press [q] to stop, [?] for help\n" +
                    "Output #0, null, to 'pipe:':\n" +
                    "  Metadata:\n" +
                    "    major_brand     : isom\n" +
                    "    minor_version   : 1\n" +
                    "    compatible_brands: isomavc1\n" +
                    "    composer        : Sacha Goedegebure\n" +
                    "    title           : Big Buck Bunny, Sunflower version\n" +
                    "    artist          : Blender Foundation 2008, Janus Bager Kristensen 2013\n" +
                    "    comment         : Creative Commons Attribution 3.0 - http://bbb3d.renderfarming.net\n" +
                    "    genre           : Animation\n" +
                    "    encoder         : Lavf58.12.100\n" +
                    "    Stream #0:0(und): Video: wrapped_avframe, yuv420p(progressive), 3840x4320 [SAR 1:1 DAR 8:9], q=2-31, 200 kb/s, 30 fps, 30 tbn, 30 tbc (default)\n" +
                    "    Metadata:\n" +
                    "      creation_time   : 2013-12-16T17:21:55.000000Z\n" +
                    "      handler_name    : GPAC ISO Video Handler\n" +
                    "      encoder         : Lavc58.18.100 wrapped_avframe\n" +
                    "    Stream #0:1(und): Audio: pcm_s16le, 48000 Hz, 5.1(side), s16, 4608 kb/s (default)\n" +
                    "    Metadata:\n" +
                    "      creation_time   : 2013-12-16T17:21:58.000000Z\n" +
                    "      handler_name    : GPAC ISO Audio Handler\n" +
                    "      encoder         : Lavc58.18.100 pcm_s16le\n" +
                    "    Side data:\n" +
                    "      audio service type: main\n" +
                    "frame= 2798 fps= 85 q=-0.0 size=N/A time=00:01:33.33 bitrate=N/A speed=2.85x    \n";

    public static final String MEDIA_INFORMATION_MP4_2 =
            "Input #0, mov,mp4,m4a,3gp,3g2,mj2, from 'http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_30fps_stereo_arcd.mp4':\n" +
                    "  Metadata:\n" +
                    "    major_brand     : isom\n" +
                    "    minor_version   : 1\n" +
                    "    compatible_brands: isomavc1\n" +
                    "    creation_time   : 2013-12-16T17:49:59.000000Z\n" +
                    "    title           : Big Buck Bunny, Sunflower version\n" +
                    "    artist          : Blender Foundation 2008, Janus Bager Kristensen 2013\n" +
                    "    comment         : Creative Commons Attribution 3.0 - http://bbb3d.renderfarming.net\n" +
                    "    genre           : Animation\n" +
                    "    composer        : Sacha Goedegebure\n" +
                    "  Duration: 00:10:34.53, start: 0.000000, bitrate: 4474 kb/s\n" +
                    "    Stream #0:0(und): Video: h264 (High) (avc1 / 0x31637661), yuv420p, 1920x1080 [SAR 1:1 DAR 16:9], 3992 kb/s, 30 fps, 30 tbr, 30k tbn, 60 tbc (default)\n" +
                    "    Metadata:\n" +
                    "      creation_time   : 2013-12-16T17:49:59.000000Z\n" +
                    "      handler_name    : GPAC ISO Video Handler\n" +
                    "    Stream #0:1(und): Audio: mp3 (mp4a / 0x6134706D), 48000 Hz, stereo, fltp, 160 kb/s (default)\n" +
                    "    Metadata:\n" +
                    "      creation_time   : 2013-12-16T17:50:04.000000Z\n" +
                    "      handler_name    : GPAC ISO Audio Handler\n" +
                    "    Stream #0:2(und): Audio: ac3 (ac-3 / 0x332D6361), 48000 Hz, 5.1(side), fltp, 320 kb/s (default)\n" +
                    "    Metadata:\n" +
                    "      creation_time   : 2013-12-16T17:50:04.000000Z\n" +
                    "      handler_name    : GPAC ISO Audio Handler\n" +
                    "    Side data:\n" +
                    "      audio service type: main\n" +
                    "Stream mapping:\n" +
                    "  Stream #0:0 -> #0:0 (h264 (native) -> wrapped_avframe (native))\n" +
                    "  Stream #0:2 -> #0:1 (ac3 (native) -> pcm_s16le (native))\n" +
                    "Press [q] to stop, [?] for help\n" +
                    "Output #0, null, to 'pipe:':\n" +
                    "  Metadata:\n" +
                    "    major_brand     : isom\n" +
                    "    minor_version   : 1\n" +
                    "    compatible_brands: isomavc1\n" +
                    "    composer        : Sacha Goedegebure\n" +
                    "    title           : Big Buck Bunny, Sunflower version\n" +
                    "    artist          : Blender Foundation 2008, Janus Bager Kristensen 2013\n" +
                    "    comment         : Creative Commons Attribution 3.0 - http://bbb3d.renderfarming.net\n" +
                    "    genre           : Animation\n" +
                    "    encoder         : Lavf58.12.100\n" +
                    "    Stream #0:0(und): Video: wrapped_avframe, yuv420p(progressive), 1920x1080 [SAR 1:1 DAR 16:9], q=2-31, 200 kb/s, 30 fps, 30 tbn, 30 tbc (default)\n" +
                    "    Metadata:\n" +
                    "      creation_time   : 2013-12-16T17:49:59.000000Z\n" +
                    "      handler_name    : GPAC ISO Video Handler\n" +
                    "      encoder         : Lavc58.18.100 wrapped_avframe\n" +
                    "    Stream #0:1(und): Audio: pcm_s16le, 48000 Hz, 5.1(side), s16, 4608 kb/s (default)\n" +
                    "    Metadata:\n" +
                    "      creation_time   : 2013-12-16T17:50:04.000000Z\n" +
                    "      handler_name    : GPAC ISO Audio Handler\n" +
                    "      encoder         : Lavc58.18.100 pcm_s16le\n" +
                    "    Side data:\n" +
                    "      audio service type: main\n" +
                    "frame=19036 fps=401 q=-0.0 Lsize=N/A time=00:10:34.60 bitrate=N/A speed=13.4x    \n" +
                    "video:9964kB audio:356706kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown";

    public static final String MEDIA_INFORMATION_OGG =
            "[theora @ 0x7fa30c026e00] 7 bits left in packet 82\n" +
                    "[ogg @ 0x7fa30c005e00] Broken file, keyframe not correctly marked.\n" +
                    "Input #0, ogg, from 'trailer_400p.ogg':\n" +
                    "  Duration: 00:00:33.00, start: 0.000000, bitrate: 1057 kb/s\n" +
                    "    Stream #0:0: Video: theora, yuv420p(bt470bg/bt470bg/bt709), 720x400, 25 fps, 25 tbr, 25 tbn, 25 tbc\n" +
                    "    Metadata:\n" +
                    "      ENCODER         : ffmpeg2theora 0.19\n" +
                    "    Stream #0:1: Audio: vorbis, 48000 Hz, stereo, fltp, 80 kb/s\n" +
                    "    Metadata:\n" +
                    "      ENCODER         : ffmpeg2theora 0.19\n" +
                    "[theora @ 0x7fa30c1bd600] 7 bits left in packet 82\n" +
                    "Stream mapping:\n" +
                    "  Stream #0:0 -> #0:0 (theora (native) -> wrapped_avframe (native))\n" +
                    "  Stream #0:1 -> #0:1 (vorbis (native) -> pcm_s16le (native))\n" +
                    "Press [q] to stop, [?] for help\n" +
                    "Output #0, null, to 'pipe:':\n" +
                    "  Metadata:\n" +
                    "    encoder         : Lavf58.12.100\n" +
                    "    Stream #0:0: Video: wrapped_avframe, yuv420p(progressive), 720x400, q=2-31, 200 kb/s, 25 fps, 25 tbn, 25 tbc\n" +
                    "    Metadata:\n" +
                    "      encoder         : Lavc58.18.100 wrapped_avframe\n" +
                    "    Stream #0:1: Audio: pcm_s16le, 48000 Hz, stereo, s16, 1536 kb/s\n" +
                    "    Metadata:\n" +
                    "      encoder         : Lavc58.18.100 pcm_s16le\n" +
                    "[ogg @ 0x7fa30c005e00] Broken file, keyframe not correctly marked.\n" +
                    "    Last message repeated 5 times\n" +
                    "frame=  813 fps=0.0 q=-0.0 Lsize=N/A time=00:00:33.01 bitrate=N/A speed= 234x    \n" +
                    "video:426kB audio:6190kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown";

    @Test
    public void mediaInformationMp3() {
        MediaInformation mediaInformation = MediaInformation.from(MEDIA_INFORMATION_MP3);

        Assert.assertNotNull(mediaInformation);
    }

}
