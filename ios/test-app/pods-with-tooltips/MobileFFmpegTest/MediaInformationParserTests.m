//
// MediaInformationParserTests.m
//
// Copyright (c) 2018 Taner Sener
//
// This file is part of MobileFFmpeg.
//
// MobileFFmpeg is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// MobileFFmpeg is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
//

#include <UIKit/UIKit.h>

#include <mobileffmpeg/MediaInformationParser.h>

static NSString *MEDIA_INFORMATION_MP3;
static NSString *MEDIA_INFORMATION_JPG;
static NSString *MEDIA_INFORMATION_GIF;
static NSString *MEDIA_INFORMATION_H264;
static NSString *MEDIA_INFORMATION_PNG;
static NSString *MEDIA_INFORMATION_H264_2;
static NSString *MEDIA_INFORMATION_MP4;
static NSString *MEDIA_INFORMATION_MP4_2;
static NSString *MEDIA_INFORMATION_OGG;
static NSString *MEDIA_INFORMATION_RECORDING;

static void initTests() {
    MEDIA_INFORMATION_MP3 = [NSString stringWithFormat:
                         @"Unknown attached picture mimetype: audio/x-wav, skipping.\n"
                         "[mp3 @ 0x7ffb94805800] Estimating duration from bitrate, this may be inaccurate\n"
                         "Input #0, mp3, from 'beethoven_-_symphony_no_9.mp3':\n"
                         "  Metadata:\n"
                         "    comment         :  \n"
                         "    album           : Symphony No.9\n"
                         "    compilation     : 0\n"
                         "    date            : -1\n"
                         "    title           : Symphony No.9\n"
                         "    artist          : Beethoven\n"
                         "    album_artist    : Beethoven\n"
                         "    track           : -1\n"
                         "    lyrics-XXX      : \n"
                         "  Duration: 00:03:33.24, start: 0.000000, bitrate: 320 kb/s\n"
                         "    Stream #0:0: Audio: mp3, 48000 Hz, stereo, fltp, 320 kb/s\n"
                         "Stream mapping:\n"
                         "  Stream #0:0 -> #0:0 (mp3 (mp3float) -> pcm_s16le (native))\n"
                         "Press [q] to stop, [?] for help\n"
                         "Output #0, null, to 'pipe:':\n"
                         "  Metadata:\n"
                         "    comment         :  \n"
                         "    album           : Symphony No.9\n"
                         "    compilation     : 0\n"
                         "    date            : -1\n"
                         "    title           : Symphony No.9\n"
                         "    artist          : Beethoven\n"
                         "    album_artist    : Beethoven\n"
                         "    track           : -1\n"
                         "    lyrics-XXX      : \n"
                         "    encoder         : Lavf58.12.100\n"
                         "    Stream #0:0: Audio: pcm_s16le, 48000 Hz, stereo, s16, 1536 kb/s\n"
                         "    Metadata:\n"
                         "      encoder         : Lavc58.18.100 pcm_s16le\n"
                         "size=N/A time=00:03:33.24 bitrate=N/A speed= 618x    \n"
                         "video:0kB audio:39982kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown"];

    MEDIA_INFORMATION_JPG = [NSString stringWithFormat:
                             @"Input #0, image2, from '/data/user/0/com.arthenica.mobileffmpeg.test/cache/colosseum.jpg':\n"
                             "  Duration: 00:00:00.04, start: 0.000000, bitrate: 391187 kb/s\n"
                             "    Stream #0:0: Video: mjpeg, yuvj420p(pc, bt470bg/unknown/unknown), 2560x1708 [SAR 1:1 DAR 640:427], 25 tbr, 25 tbn, 25 tbc\n"
                             "Stream mapping:\n"
                             "  Stream #0:0 -> #0:0 (mjpeg (native) -> wrapped_avframe (native))\n"
                             "Press [q] to stop, [?] for help\n"
                             "Output #0, null, to 'pipe:':\n"
                             "  Metadata:\n"
                             "    encoder         : Lavf58.18.104\n"
                             "    Stream #0:0: Video: wrapped_avframe, yuvj420p, 2560x1708 [SAR 1:1 DAR 640:427], q=2-31, 200 kb/s, 25 fps, 25 tbn, 25 tbc\n"
                             "    Metadata:\n"
                             "      encoder         : Lavc58.31.102 wrapped_avframe\n"
                             "frame=    1 fps=0.0 q=-0.0 Lsize=N/A time=00:00:00.04 bitrate=N/A speed=0.668x    \n"
                             "video:0kB audio:0kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown\n"];

    MEDIA_INFORMATION_GIF = [NSString stringWithFormat:
                             @"Input #0, gif, from 'advanced_zoom_in_and_pan_with_fade_in_out.gif':\n"
                             "  Duration: N/A, bitrate: N/A\n"
                             "    Stream #0:0: Video: gif, bgra, 420x236, 6 fps, 6 tbr, 100 tbn, 100 tbc\n"
                             "Stream mapping:\n"
                             "  Stream #0:0 -> #0:0 (gif (native) -> wrapped_avframe (native))\n"
                             "Press [q] to stop, [?] for help\n"
                             "Output #0, null, to 'pipe:':\n"
                             "  Metadata:\n"
                             "    encoder         : Lavf58.12.100\n"
                             "    Stream #0:0: Video: wrapped_avframe, bgra, 420x236 [SAR 63:64 DAR 6615:3776], q=2-31, 200 kb/s, 6 fps, 6 tbn, 6 tbc\n"
                             "    Metadata:\n"
                             "      encoder         : Lavc58.18.100 wrapped_avframe\n"
                             "frame=   61 fps=0.0 q=-0.0 Lsize=N/A time=00:00:10.16 bitrate=N/A speed= 219x    \n"
                             "video:32kB audio:0kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown"];

    MEDIA_INFORMATION_H264 = [NSString stringWithFormat:
                              @"Input #0, mov,mp4,m4a,3gp,3g2,mj2, from 'transition_rotate.mp4':\n"
                              "  Metadata:\n"
                              "    major_brand     : isom\n"
                              "    minor_version   : 512\n"
                              "    compatible_brands: isomiso2avc1mp41\n"
                              "    encoder         : Lavf58.12.100\n"
                              "  Duration: 00:00:15.00, start: 0.000000, bitrate: 7764 kb/s\n"
                              "    Stream #0:0(und): Video: h264 (Main) (avc1 / 0x31637661), yuv420p, 1280x720 [SAR 1:1 DAR 16:9], 7762 kb/s, 25 fps, 30 tbr, 15360 tbn, 60 tbc (default)\n"
                              "    Metadata:\n"
                              "      handler_name    : VideoHandler\n"
                              "Stream mapping:\n"
                              "  Stream #0:0 -> #0:0 (h264 (native) -> wrapped_avframe (native))\n"
                              "Press [q] to stop, [?] for help\n"
                              "Output #0, null, to 'pipe:':\n"
                              "  Metadata:\n"
                              "    major_brand     : isom\n"
                              "    minor_version   : 512\n"
                              "    compatible_brands: isomiso2avc1mp41\n"
                              "    encoder         : Lavf58.12.100\n"
                              "    Stream #0:0(und): Video: wrapped_avframe, yuv420p, 1280x720 [SAR 1:1 DAR 16:9], q=2-31, 200 kb/s, 30 fps, 30 tbn, 30 tbc (default)\n"
                              "    Metadata:\n"
                              "      handler_name    : VideoHandler\n"
                              "      encoder         : Lavc58.18.100 wrapped_avframe\n"
                              "frame=  375 fps=0.0 q=-0.0 Lsize=N/A time=00:00:15.00 bitrate=N/A speed=35.9x    \n"
                              "video:196kB audio:0kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown"];

    MEDIA_INFORMATION_PNG = [NSString stringWithFormat:
                             @"Input #0, png_pipe, from 'https://www.google.com/images/branding/googlelogo/2x/googlelogo_color_272x92dp.png':\n"
                             "  Duration: N/A, bitrate: N/A\n"
                             "    Stream #0:0: Video: png, rgba(pc), 544x184, 25 tbr, 25 tbn, 25 tbc\n"
                             "Stream mapping:\n"
                             "  Stream #0:0 -> #0:0 (png (native) -> wrapped_avframe (native))\n"
                             "Press [q] to stop, [?] for help\n"
                             "Output #0, null, to 'pipe:':\n"
                             "  Metadata:\n"
                             "    encoder         : Lavf58.12.100\n"
                             "    Stream #0:0: Video: wrapped_avframe, rgba, 544x184, q=2-31, 200 kb/s, 25 fps, 25 tbn, 25 tbc\n"
                             "    Metadata:\n"
                             "      encoder         : Lavc58.18.100 wrapped_avframe\n"
                             "frame=    1 fps=0.0 q=-0.0 Lsize=N/A time=00:00:00.04 bitrate=N/A speed=27.1x    \n"
                             "video:1kB audio:0kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown"];

    MEDIA_INFORMATION_H264_2 = [NSString stringWithFormat:
                                @"Input #0, h264, from 'test.h264':\n"
                                "  Duration: N/A, bitrate: N/A\n"
                                "    Stream #0:0: Video: h264 (Main), yuv420p(tv, bt709, progressive), 1920x1080, 25 fps, 25 tbr, 1200k tbn, 50 tbc\n"
                                "Stream mapping:\n"
                                "  Stream #0:0 -> #0:0 (h264 (native) -> wrapped_avframe (native))\n"
                                "Press [q] to stop, [?] for help\n"
                                "Output #0, null, to 'pipe:':\n"
                                "  Metadata:\n"
                                "    encoder         : Lavf58.12.100\n"
                                "    Stream #0:0: Video: wrapped_avframe, yuv420p, 1920x1080, q=2-31, 200 kb/s, 25 fps, 25 tbn, 25 tbc\n"
                                "    Metadata:\n"
                                "      encoder         : Lavc58.18.100 wrapped_avframe\n"
                                "frame=  360 fps=0.0 q=-0.0 Lsize=N/A time=00:00:14.40 bitrate=N/A speed=25.5x    \n"
                                "video:188kB audio:0kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown"];

    MEDIA_INFORMATION_MP4 = [NSString stringWithFormat:
                             @"Input #0, mov,mp4,m4a,3gp,3g2,mj2, from 'http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_2160p_30fps_stereo_abl.mp4':\n"
                             "  Metadata:\n"
                             "    major_brand     : isom\n"
                             "    minor_version   : 1\n"
                             "    compatible_brands: isomavc1\n"
                             "    creation_time   : 2013-12-16T17:21:55.000000Z\n"
                             "    title           : Big Buck Bunny, Sunflower version\n"
                             "    artist          : Blender Foundation 2008, Janus Bager Kristensen 2013\n"
                             "    comment         : Creative Commons Attribution 3.0 - http://bbb3d.renderfarming.net\n"
                             "    genre           : Animation\n"
                             "    composer        : Sacha Goedegebure\n"
                             "  Duration: 00:10:34.53, start: 0.000000, bitrate: 10385 kb/s\n"
                             "    Stream #0:0(und): Video: h264 (High) (avc1 / 0x31637661), yuv420p, 3840x4320 [SAR 1:1 DAR 8:9], 9902 kb/s, 30 fps, 30 tbr, 30k tbn, 60 tbc (default)\n"
                             "    Metadata:\n"
                             "      creation_time   : 2013-12-16T17:21:55.000000Z\n"
                             "      handler_name    : GPAC ISO Video Handler\n"
                             "    Stream #0:1(und): Audio: mp3 (mp4a / 0x6134706D), 48000 Hz, stereo, fltp, 160 kb/s (default)\n"
                             "    Metadata:\n"
                             "      creation_time   : 2013-12-16T17:21:58.000000Z\n"
                             "      handler_name    : GPAC ISO Audio Handler\n"
                             "    Stream #0:2(und): Audio: ac3 (ac-3 / 0x332D6361), 48000 Hz, 5.1(side), fltp, 320 kb/s (default)\n"
                             "    Metadata:\n"
                             "      creation_time   : 2013-12-16T17:21:58.000000Z\n"
                             "      handler_name    : GPAC ISO Audio Handler\n"
                             "    Side data:\n"
                             "      audio service type: main\n"
                             "Stream mapping:\n"
                             "  Stream #0:0 -> #0:0 (h264 (native) -> wrapped_avframe (native))\n"
                             "  Stream #0:2 -> #0:1 (ac3 (native) -> pcm_s16le (native))\n"
                             "Press [q] to stop, [?] for help\n"
                             "Output #0, null, to 'pipe:':\n"
                             "  Metadata:\n"
                             "    major_brand     : isom\n"
                             "    minor_version   : 1\n"
                             "    compatible_brands: isomavc1\n"
                             "    composer        : Sacha Goedegebure\n"
                             "    title           : Big Buck Bunny, Sunflower version\n"
                             "    artist          : Blender Foundation 2008, Janus Bager Kristensen 2013\n"
                             "    comment         : Creative Commons Attribution 3.0 - http://bbb3d.renderfarming.net\n"
                             "    genre           : Animation\n"
                             "    encoder         : Lavf58.12.100\n"
                             "    Stream #0:0(und): Video: wrapped_avframe, yuv420p(progressive), 3840x4320 [SAR 1:1 DAR 8:9], q=2-31, 200 kb/s, 30 fps, 30 tbn, 30 tbc (default)\n"
                             "    Metadata:\n"
                             "      creation_time   : 2013-12-16T17:21:55.000000Z\n"
                             "      handler_name    : GPAC ISO Video Handler\n"
                             "      encoder         : Lavc58.18.100 wrapped_avframe\n"
                             "    Stream #0:1(und): Audio: pcm_s16le, 48000 Hz, 5.1(side), s16, 4608 kb/s (default)\n"
                             "    Metadata:\n"
                             "      creation_time   : 2013-12-16T17:21:58.000000Z\n"
                             "      handler_name    : GPAC ISO Audio Handler\n"
                             "      encoder         : Lavc58.18.100 pcm_s16le\n"
                             "    Side data:\n"
                             "      audio service type: main\n"
                             "frame= 2798 fps= 85 q=-0.0 size=N/A time=00:01:33.33 bitrate=N/A speed=2.85x    \n"];

    MEDIA_INFORMATION_MP4_2 = [NSString stringWithFormat:
                               @"Input #0, mov,mp4,m4a,3gp,3g2,mj2, from 'http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_30fps_stereo_arcd.mp4':\n"
                               "  Metadata:\n"
                               "    major_brand     : isom\n"
                               "    minor_version   : 1\n"
                               "    compatible_brands: isomavc1\n"
                               "    creation_time   : 2013-12-16T17:49:59.000000Z\n"
                               "    title           : Big Buck Bunny, Sunflower version\n"
                               "    artist          : Blender Foundation 2008, Janus Bager Kristensen 2013\n"
                               "    comment         : Creative Commons Attribution 3.0 - http://bbb3d.renderfarming.net\n"
                               "    genre           : Animation\n"
                               "    composer        : Sacha Goedegebure\n"
                               "  Duration: 00:10:34.53, start: 0.000000, bitrate: 4474 kb/s\n"
                               "    Stream #0:0(und): Video: h264 (High) (avc1 / 0x31637661), yuv420p, 1920x1080 [SAR 1:1 DAR 16:9], 3992 kb/s, 30 fps, 30 tbr, 30k tbn, 60 tbc (default)\n"
                               "    Metadata:\n"
                               "      creation_time   : 2013-12-16T17:49:59.000000Z\n"
                               "      handler_name    : GPAC ISO Video Handler\n"
                               "    Stream #0:1(und): Audio: mp3 (mp4a / 0x6134706D), 48000 Hz, stereo, fltp, 160 kb/s (default)\n"
                               "    Metadata:\n"
                               "      creation_time   : 2013-12-16T17:50:04.000000Z\n"
                               "      handler_name    : GPAC ISO Audio Handler\n"
                               "    Stream #0:2(und): Audio: ac3 (ac-3 / 0x332D6361), 48000 Hz, 5.1(side), fltp, 320 kb/s (default)\n"
                               "    Metadata:\n"
                               "      creation_time   : 2013-12-16T17:50:04.000000Z\n"
                               "      handler_name    : GPAC ISO Audio Handler\n"
                               "    Side data:\n"
                               "      audio service type: main\n"
                               "Stream mapping:\n"
                               "  Stream #0:0 -> #0:0 (h264 (native) -> wrapped_avframe (native))\n"
                               "  Stream #0:2 -> #0:1 (ac3 (native) -> pcm_s16le (native))\n"
                               "Press [q] to stop, [?] for help\n"
                               "Output #0, null, to 'pipe:':\n"
                               "  Metadata:\n"
                               "    major_brand     : isom\n"
                               "    minor_version   : 1\n"
                               "    compatible_brands: isomavc1\n"
                               "    composer        : Sacha Goedegebure\n"
                               "    title           : Big Buck Bunny, Sunflower version\n"
                               "    artist          : Blender Foundation 2008, Janus Bager Kristensen 2013\n"
                               "    comment         : Creative Commons Attribution 3.0 - http://bbb3d.renderfarming.net\n"
                               "    genre           : Animation\n"
                               "    encoder         : Lavf58.12.100\n"
                               "    Stream #0:0(und): Video: wrapped_avframe, yuv420p(progressive), 1920x1080 [SAR 1:1 DAR 16:9], q=2-31, 200 kb/s, 30 fps, 30 tbn, 30 tbc (default)\n"
                               "    Metadata:\n"
                               "      creation_time   : 2013-12-16T17:49:59.000000Z\n"
                               "      handler_name    : GPAC ISO Video Handler\n"
                               "      encoder         : Lavc58.18.100 wrapped_avframe\n"
                               "    Stream #0:1(und): Audio: pcm_s16le, 48000 Hz, 5.1(side), s16, 4608 kb/s (default)\n"
                               "    Metadata:\n"
                               "      creation_time   : 2013-12-16T17:50:04.000000Z\n"
                               "      handler_name    : GPAC ISO Audio Handler\n"
                               "      encoder         : Lavc58.18.100 pcm_s16le\n"
                               "    Side data:\n"
                               "      audio service type: main\n"
                               "frame=19036 fps=401 q=-0.0 Lsize=N/A time=00:10:34.60 bitrate=N/A speed=13.4x    \n"
                               "video:9964kB audio:356706kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown"];

        MEDIA_INFORMATION_OGG = [NSString stringWithFormat:
                                 @"[theora @ 0x7fa30c026e00] 7 bits left in packet 82\n"
                                 "[ogg @ 0x7fa30c005e00] Broken file, keyframe not correctly marked.\n"
                                 "Input #0, ogg, from 'trailer_400p.ogg':\n"
                                 "  Duration: 00:00:33.00, start: 0.000000, bitrate: 1057 kb/s\n"
                                 "    Stream #0:0: Video: theora, yuv420p(bt470bg/bt470bg/bt709), 720x400, 25 fps, 25 tbr, 25 tbn, 25 tbc\n"
                                 "    Metadata:\n"
                                 "      ENCODER         : ffmpeg2theora 0.19\n"
                                 "    Stream #0:1: Audio: vorbis, 48000 Hz, stereo, fltp, 80 kb/s\n"
                                 "    Metadata:\n"
                                 "      ENCODER         : ffmpeg2theora 0.19\n"
                                 "[theora @ 0x7fa30c1bd600] 7 bits left in packet 82\n"
                                 "Stream mapping:\n"
                                 "  Stream #0:0 -> #0:0 (theora (native) -> wrapped_avframe (native))\n"
                                 "  Stream #0:1 -> #0:1 (vorbis (native) -> pcm_s16le (native))\n"
                                 "Press [q] to stop, [?] for help\n"
                                 "Output #0, null, to 'pipe:':\n"
                                 "  Metadata:\n"
                                 "    encoder         : Lavf58.12.100\n"
                                 "    Stream #0:0: Video: wrapped_avframe, yuv420p(progressive), 720x400, q=2-31, 200 kb/s, 25 fps, 25 tbn, 25 tbc\n"
                                 "    Metadata:\n"
                                 "      encoder         : Lavc58.18.100 wrapped_avframe\n"
                                 "    Stream #0:1: Audio: pcm_s16le, 48000 Hz, stereo, s16, 1536 kb/s\n"
                                 "    Metadata:\n"
                                 "      encoder         : Lavc58.18.100 pcm_s16le\n"
                                 "[ogg @ 0x7fa30c005e00] Broken file, keyframe not correctly marked.\n"
                                 "    Last message repeated 5 times\n"
                                 "frame=  813 fps=0.0 q=-0.0 Lsize=N/A time=00:00:33.01 bitrate=N/A speed= 234x    \n"
                                 "video:426kB audio:6190kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown"];
    
    MEDIA_INFORMATION_RECORDING = [NSString stringWithFormat:
                                   @"Input #0, mov,mp4,m4a,3gp,3g2,mj2, from '/var/mobile/Containers/Data/Application/845A06CD-8427-4D2D-A9A8-F7738063E220/Library/Caches/video.mov':\n"
                                   "Metadata:\n"
                                   "    major_brand     : qt\n"
                                   "    minor_version   : 0\n"
                                   "    compatible_brands: qt\n"
                                   "    creation_time   : 2019-04-18T09:53:38.000000Z\n"
                                   "    com.apple.quicktime.location.ISO6709: +40.9761+029.0949+070.349/\n"
                                   "    com.apple.quicktime.make: Apple\n"
                                   "    com.apple.quicktime.model: iPhone 6\n"
                                   "    com.apple.quicktime.software: 12.2\n"
                                   "    com.apple.quicktime.creationdate: 2019-04-18T12:53:38+0300\n"
                                   "Duration: 00:00:02.30, start: 0.000000, bitrate: 16658 kb/s\n"
                                   "Stream #0:0(und): Video: h264 (avc1 / 0x31637661), yuv420p(tv, bt709), 1920x1080, 16535 kb/s, 29.98 fps, 29.97 tbr, 600 tbn, 1200 tbc (default)\n"
                                   "Metadata:\n"
                                   "    rotate          : 90\n"
                                   "    creation_time   : 2019-04-18T09:53:38.000000Z\n"
                                   "    handler_name    : Core Media Video\n"
                                   "    encoder         : H.264\n"
                                   "Side data:\n"
                                   "    displaymatrix: rotation of -90.00 degrees\n"
                                   "Stream #0:1(und): Audio: aac (mp4a / 0x6134706D), 44100 Hz, mono, fltp, 96 kb/s (default)\n"
                                   "Metadata:\n"
                                   "    creation_time   : 2019-04-18T09:53:38.000000Z\n"
                                   "    handler_name    : Core Media Audio\n"
                                   "    Stream #0:2(und): Data: none (mebx / 0x7862656D), 0 kb/s (default)\n"
                                   "Metadata:\n"
                                   "    creation_time   : 2019-04-18T09:53:38.000000Z\n"
                                   "    handler_name    : Core Media Metadata\n"
                                   "Stream #0:3(und): Data: none (mebx / 0x7862656D), 0 kb/s (default)\n"
                                   "Metadata:\n"
                                   "    creation_time   : 2019-04-18T09:53:38.000000Z\n"
                                   "    handler_name    : Core Media Metadata\n"];
}

void assertNumber(NSNumber *expected, NSNumber *real) {
    if (expected == nil) {
        assert(real == nil);
    } else {
        assert([expected isEqualToNumber:real]);
    }
}

void assertString(NSString *expected, NSString *real) {
    if (expected == nil) {
        assert(real == nil);
    } else {
        assert([expected isEqualToString:real]);
    }
}

void assertVideoStream(StreamInformation *stream, NSNumber *index, NSString *codec, NSString *fullCodec, NSString *format, NSString *fullFormat, NSNumber *width, NSNumber *height, NSString *sampleAspectRatio, NSString *displayAspectRatio, NSNumber *bitrate, NSString *averageFrameRate, NSString *realFrameRate, NSString *timeBase, NSString *codecTimeBase) {

    assert(stream != nil);
    assertNumber(index, [stream getIndex]);
    assertString(@"video", [stream getType]);
    assertString(codec, [stream getCodec]);
    assertString(fullCodec, [stream getFullCodec]);
    
    assertString(format, [stream getFormat]);
    assertString(fullFormat, [stream getFullFormat]);
    
    assertNumber(width, [stream getWidth]);
    assertNumber(height, [stream getHeight]);
    assertString(sampleAspectRatio, [stream getSampleAspectRatio]);
    assertString(displayAspectRatio, [stream getDisplayAspectRatio]);

    assertNumber(bitrate, [stream getBitrate]);
    assertString(averageFrameRate, [stream getAverageFrameRate]);
    assertString(realFrameRate, [stream getRealFrameRate]);
    assertString(timeBase, [stream getTimeBase]);
    assertString(codecTimeBase, [stream getCodecTimeBase]);
}

void parseVideoStreamBlock(NSString *input, NSNumber *index, NSString *codec, NSString *fullCodec, NSString *format, NSString *fullFormat, NSNumber *width, NSNumber *height, NSString *sampleAspectRatio, NSString *displayAspectRatio, NSNumber *bitrate, NSString *averageFrameRate, NSString *realFrameRate, NSString *timeBase, NSString *codecTimeBase) {
    StreamInformation *stream = [MediaInformationParser parseStreamBlock:input];
    
    assertVideoStream(stream, index, codec, fullCodec, format, fullFormat, width, height, sampleAspectRatio, displayAspectRatio, bitrate, averageFrameRate, realFrameRate, timeBase, codecTimeBase);
}

void assertAudioStream(StreamInformation *stream, NSNumber *index, NSString *codec, NSString *fullCodec, NSNumber *sampleRate, NSString *channelLayout, NSString *sampleFormat, NSNumber *bitrate) {

    assert(stream != nil);
    assertNumber(index, [stream getIndex]);
    assertString(@"audio", [stream getType]);
    assertString(codec, [stream getCodec]);
    assertString(fullCodec, [stream getFullCodec]);
    assertNumber(sampleRate, [stream getSampleRate]);
    assertString(channelLayout, [stream getChannelLayout]);
    assertString(sampleFormat, [stream getSampleFormat]);
    assertNumber(bitrate, [stream getBitrate]);
}

void parseAudioStreamBlock(NSString *input, NSNumber *index, NSString *codec, NSString *fullCodec, NSNumber *sampleRate, NSString *channelLayout, NSString *sampleFormat, NSNumber *bitrate) {
    StreamInformation *stream = [MediaInformationParser parseStreamBlock:input];
    
    assertAudioStream(stream, index, codec, fullCodec, sampleRate, channelLayout, sampleFormat, bitrate);
}

void parseVideoDimensions(NSString *input, NSNumber *expectedWidth, NSNumber *expectedHeight) {
    void(^pair)(NSNumber **, NSNumber **) = [MediaInformationParser parseVideoDimensions:input];
    
    NSNumber *width;
    NSNumber *height;
    pair(&width, &height);
    
    assert(pair != nil);
    if (width == nil) {
        assert(expectedWidth == nil);
    } else {
        assert([width isEqualToNumber:expectedWidth]);
    }
    if (height == nil) {
        assert(expectedHeight == nil);
    } else {
        assert([height isEqualToNumber:expectedHeight]);
    }
}

void parseDuration(NSString *input, NSNumber *expectedDuration) {
    NSNumber *duration = [MediaInformationParser parseDuration:input];
    
    if (duration == nil) {
        assert(expectedDuration == nil);
    } else {
        assert([duration isEqualToNumber:expectedDuration]);
    }
}

void parseStartTime(NSString *input, NSNumber *expectedStartTime) {
    NSNumber *startTime = [MediaInformationParser parseStartTime:input];
    
    if (startTime == nil) {
        assert(expectedStartTime == nil);
    } else {
        assert([startTime isEqualToNumber:expectedStartTime]);
    }
}

void parseDurationBlock(NSString *input, NSNumber *expectedDuration, NSNumber *expectedStartTime, NSNumber *expectedBitrate) {
    void(^trio)(NSNumber **, NSNumber **, NSNumber **) = [MediaInformationParser parseDurationBlock:input];
    
    NSNumber *duration;
    NSNumber *startTime;
    NSNumber *bitrate;
    trio(&duration, &startTime, &bitrate);

    assert(trio != nil);
    if (duration == nil) {
        assert(expectedDuration == nil);
    } else {
        assert([duration isEqualToNumber:expectedDuration]);
    }
    if (startTime == nil) {
        assert(expectedStartTime == nil);
    } else {
        assert([startTime isEqualToNumber:expectedStartTime]);
    }
    if (bitrate == nil) {
        assert(expectedBitrate == nil);
    } else {
        assert([bitrate isEqualToNumber:expectedBitrate]);
    }
}

void parseMetadataBlock(NSString *input, NSString *expectedKey, NSString *expectedValue) {
    void(^pair)(NSString **, NSString **) = [MediaInformationParser parseMetadataBlock:input];
    
    NSString *key;
    NSString *value;
    pair(&key, &value);
    
    assert(pair != nil);
    assert(key != nil);
    assert(value != nil);
    assert([key isEqualToString:expectedKey]);
    assert([value isEqualToString:expectedValue]);
}

void parseInputBlock(NSString *input, NSString *expectedFormat, NSString *expectedPath) {
    void(^pair)(NSString **, NSString **) = [MediaInformationParser parseInputBlock:input];
    
    NSString *format;
    NSString *path;
    pair(&format, &path);
    
    assert(pair != nil);
    if (format == nil) {
        assert(expectedFormat == nil);
    } else {
        assert([format isEqualToString:expectedFormat]);
    }
    if (path == nil) {
        assert(expectedPath == nil);
    } else {
        assert([path isEqualToString:expectedPath]);
    }
}

void testParseVideoStream() {
    parseVideoStreamBlock(@"     Stream #0:0: Video: mjpeg, yuvj420p(pc, bt470bg/unknown/unknown), 2560x1708 [SAR 1:1 DAR 640:427], 25 tbr, 25 tbn, 25 tbc", [[NSNumber alloc] initWithInt:0], @"mjpeg", @"mjpeg", @"yuvj420p", @"yuvj420p(pc, bt470bg/unknown/unknown)", [[NSNumber alloc] initWithInt:2560], [[NSNumber alloc] initWithInt:1708], @"1:1", @"640:427", nil, nil, @"25", @"25", @"25");
    parseVideoStreamBlock(@"     Stream #0:0: Video: gif, bgra, 420x236, 6 fps, 6 tbr, 100 tbn, 100 tbc", [[NSNumber alloc] initWithInt:0], @"gif", @"gif", @"bgra", @"bgra", [[NSNumber alloc] initWithInt:420], [[NSNumber alloc] initWithInt:236], nil, nil, nil, @"6", @"6", @"100", @"100");
    parseVideoStreamBlock(@"     Stream #0:0(und): Video: h264 (Main) (avc1 / 0x31637661), yuv420p, 1280x720 [SAR 1:1 DAR 16:9], 7762 kb/s, 25 fps, 30 tbr, 15360 tbn, 60 tbc (default)", [[NSNumber alloc] initWithInt:0], @"h264", @"h264 (main) (avc1 / 0x31637661)", @"yuv420p", @"yuv420p", [[NSNumber alloc] initWithInt:1280], [[NSNumber alloc] initWithInt:720], @"1:1", @"16:9", [[NSNumber alloc] initWithInt:7762], @"25", @"30", @"15360", @"60");
    parseVideoStreamBlock(@"     Stream #0:0: Video: png, rgba(pc), 544x184, 25 tbr, 25 tbn, 25 tbc", [[NSNumber alloc] initWithInt:0], @"png", @"png", @"rgba", @"rgba(pc)", [[NSNumber alloc] initWithInt:544], [[NSNumber alloc] initWithInt:184], nil, nil, nil, nil, @"25", @"25", @"25");
    parseVideoStreamBlock(@"     Stream #0:0: Video: h264 (Main), yuv420p(tv, bt709, progressive), 1920x1080, 25 fps, 25 tbr, 1200k tbn, 50 tbc", [[NSNumber alloc] initWithInt:0], @"h264", @"h264 (main)", @"yuv420p", @"yuv420p(tv, bt709, progressive)", [[NSNumber alloc] initWithInt:1920], [[NSNumber alloc] initWithInt:1080], nil, nil, nil, @"25", @"25", @"1200k", @"50");
    parseVideoStreamBlock(@"     Stream #0:0(und): Video: h264 (High) (avc1 / 0x31637661), yuv420p, 3840x4320 [SAR 1:1 DAR 8:9], 9902 kb/s, 30 fps, 30 tbr, 30k tbn, 60 tbc (default)", [[NSNumber alloc] initWithInt:0], @"h264", @"h264 (high) (avc1 / 0x31637661)", @"yuv420p", @"yuv420p", [[NSNumber alloc] initWithInt:3840], [[NSNumber alloc] initWithInt:4320], @"1:1", @"8:9", [[NSNumber alloc] initWithInt:9902], @"30", @"30", @"30k", @"60");
    parseVideoStreamBlock(@"     Stream #0:0(und): Video: h264 (High) (avc1 / 0x31637661), yuv420p, 1920x1080 [SAR 1:1 DAR 16:9], 3992 kb/s, 30 fps, 30 tbr, 30k tbn, 60 tbc (default)", [[NSNumber alloc] initWithInt:0], @"h264", @"h264 (high) (avc1 / 0x31637661)", @"yuv420p", @"yuv420p", [[NSNumber alloc] initWithInt:1920], [[NSNumber alloc] initWithInt:1080], @"1:1", @"16:9", [[NSNumber alloc] initWithInt:3992], @"30", @"30", @"30k", @"60");
    parseVideoStreamBlock(@"     Stream #0:0: Video: theora, yuv420p(bt470bg/bt470bg/bt709), 720x400, 25 fps, 25 tbr, 25 tbn, 25 tbc", [[NSNumber alloc] initWithInt:0], @"theora", @"theora", @"yuv420p", @"yuv420p(bt470bg/bt470bg/bt709)", [[NSNumber alloc] initWithInt:720], [[NSNumber alloc] initWithInt:400], nil, nil, nil, @"25", @"25", @"25", @"25");
}

void testParseAudioStream() {
    parseAudioStreamBlock(@"Stream #0:0: Audio: adpcm_ms ([2][0][0][0] / 0x0002), 22050 Hz, stereo, s16, 176 kb/s", [[NSNumber alloc] initWithInt:0], @"adpcm_ms", @"adpcm_ms ([2][0][0][0] / 0x0002)", [[NSNumber alloc] initWithInt:22050], @"stereo", @"s16", [[NSNumber alloc] initWithInt:176]);
    parseAudioStreamBlock(@"Stream #0:0: Audio: pcm_s16le ([1][0][0][0] / 0x0001), 44100 Hz, mono, s16, 705 kb/s",[[NSNumber alloc] initWithInt:0], @"pcm_s16le", @"pcm_s16le ([1][0][0][0] / 0x0001)", [[NSNumber alloc] initWithInt:44100], @"mono", @"s16", [[NSNumber alloc] initWithInt:705]);
    parseAudioStreamBlock(@"Stream #0:0: Audio: pcm_s24le ([1][0][0][0] / 0x0001), 48000 Hz, mono, s32 (24 bit), 1152 kb/s", [[NSNumber alloc] initWithInt:0], @"pcm_s24le", @"pcm_s24le ([1][0][0][0] / 0x0001)", [[NSNumber alloc] initWithInt:48000], @"mono", @"s32 (24 bit)", [[NSNumber alloc] initWithInt:1152]);
    parseAudioStreamBlock(@"Stream #0:0: Audio: mp3, 48000 Hz, stereo, fltp, 192 kb/s", [[NSNumber alloc] initWithInt:0], @"mp3", @"mp3", [[NSNumber alloc] initWithInt:48000], @"stereo", @"fltp", [[NSNumber alloc] initWithInt:192]);
    parseAudioStreamBlock(@"Stream #0:0: Audio: vorbis, 44100 Hz, stereo, fltp, 128 kb/s", [[NSNumber alloc] initWithInt:0], @"vorbis", @"vorbis", [[NSNumber alloc] initWithInt:44100], @"stereo", @"fltp", [[NSNumber alloc] initWithInt:128]);
    parseAudioStreamBlock(@"Stream #0:0: Audio: pcm_u8 ([1][0][0][0] / 0x0001), 44100 Hz, stereo, u8, 705 kb/s", [[NSNumber alloc] initWithInt:0], @"pcm_u8", @"pcm_u8 ([1][0][0][0] / 0x0001)", [[NSNumber alloc] initWithInt:44100], @"stereo", @"u8", [[NSNumber alloc] initWithInt:705]);
    parseAudioStreamBlock(@"Stream #0:1(und): Audio: mp3 (mp4a / 0x6134706D), 48000 Hz, stereo, fltp, 160 kb/s (default)", [[NSNumber alloc] initWithInt:1], @"mp3", @"mp3 (mp4a / 0x6134706d)", [[NSNumber alloc] initWithInt:48000], @"stereo", @"fltp", [[NSNumber alloc] initWithInt:160]);
    parseAudioStreamBlock(@"Stream #0:2(und): Audio: ac3 (ac-3 / 0x332D6361), 48000 Hz, 5.1(side), fltp, 320 kb/s (default)", [[NSNumber alloc] initWithInt:2], @"ac3", @"ac3 (ac-3 / 0x332d6361)", [[NSNumber alloc] initWithInt:48000], @"5.1(side)", @"fltp", [[NSNumber alloc] initWithInt:320]);
    parseAudioStreamBlock(@"Stream #0:1(und): Audio: mp3 (mp4a / 0x6134706D), 48000 Hz, stereo, fltp, 160 kb/s (default)", [[NSNumber alloc] initWithInt:1], @"mp3", @"mp3 (mp4a / 0x6134706d)", [[NSNumber alloc] initWithInt:48000], @"stereo", @"fltp", [[NSNumber alloc] initWithInt:160]);
    parseAudioStreamBlock(@"Stream #0:2(und): Audio: ac3 (ac-3 / 0x332D6361), 48000 Hz, 5.1(side), fltp, 320 kb/s (default)", [[NSNumber alloc] initWithInt:2], @"ac3", @"ac3 (ac-3 / 0x332d6361)", [[NSNumber alloc] initWithInt:48000], @"5.1(side)", @"fltp", [[NSNumber alloc] initWithInt:320]);
}

void testIndex() {
    assert(7 == [MediaInformationParser index:@"one:two:three:" of:@":" from:0 times:2]);
    assert(13 == [MediaInformationParser index:@"one:two:three:" of:@":" from:0 times:3]);
    assert(8 == [MediaInformationParser index:@"one::two::three::" of:@"::" from:0 times:2]);
}

void testCount() {
    assert(3 == [MediaInformationParser count:@"one:two:three:" of:@":"]);
    assert(2 == [MediaInformationParser count:@"one,two:three,four" of:@","]);
}

void testParseVideoStreamDisplayAspectRatio() {
    assert(nil == [MediaInformationParser parseVideoStreamDisplayAspectRatio:@""]);
    assert(nil == [MediaInformationParser parseVideoStreamDisplayAspectRatio:@"544x184"]);
    assert([@"640:427" isEqualToString:[MediaInformationParser parseVideoStreamDisplayAspectRatio:@"2560x1708 [SAR 1:1 DAR 640:427]"]]);
    assert([@"8:9" isEqualToString:[MediaInformationParser parseVideoStreamDisplayAspectRatio:@"3840x4320 [SAR 1:1 DAR 8:9]"]]);
}

void testParseVideoStreamSampleAspectRatio() {
    assert(nil == [MediaInformationParser parseVideoStreamSampleAspectRatio:@""]);
    assert(nil == [MediaInformationParser parseVideoStreamSampleAspectRatio:@"544x184"]);
    assert([@"1:1" isEqualToString:[MediaInformationParser parseVideoStreamSampleAspectRatio:@"2560x1708 [SAR 1:1 DAR 640:427]"]]);
    assert([@"1:1" isEqualToString:[MediaInformationParser parseVideoStreamSampleAspectRatio:@"3840x4320 [SAR 1:1 DAR 8:9]"]]);
}

void testParseVideoDimensions() {
    parseVideoDimensions(@"", nil, nil);
    parseVideoDimensions(@"544x184", [[NSNumber alloc] initWithInt:544], [[NSNumber alloc] initWithInt:184]);
    parseVideoDimensions(@"720x400", [[NSNumber alloc] initWithInt:720], [[NSNumber alloc] initWithInt:400]);
    parseVideoDimensions(@"2560x1708 [SAR 1:1 DAR 640:427]", [[NSNumber alloc] initWithInt:2560], [[NSNumber alloc] initWithInt:1708]);
    parseVideoDimensions(@"3840x4320 [SAR 1:1 DAR 8:9]", [[NSNumber alloc] initWithInt:3840], [[NSNumber alloc] initWithInt:4320]);
}

void testParseAudioSampleRate() {
    assert([[[NSNumber alloc] initWithInt:44000] isEqualToNumber:[MediaInformationParser parseAudioStreamSampleRate:@"44000"]]);
    assert([[[NSNumber alloc] initWithInt:44000] isEqualToNumber:[MediaInformationParser parseAudioStreamSampleRate:@"44 khz"]]);
    assert([[[NSNumber alloc] initWithInt:44100] isEqualToNumber:[MediaInformationParser parseAudioStreamSampleRate:@"44100"]]);
    assert([[[NSNumber alloc] initWithInt:5000000] isEqualToNumber:[MediaInformationParser parseAudioStreamSampleRate:@"5 mhz"]]);
}

void testParseAudioStreamType() {
    assert([@"video" isEqualToString:[MediaInformationParser parseStreamType:@"Video: theora"]]);
    assert([@"video" isEqualToString:[MediaInformationParser parseStreamType:@"Video: png"]]);
    assert([@"video" isEqualToString:[MediaInformationParser parseStreamType:@"Video: h264 (Main)"]]);
    assert([@"audio" isEqualToString:[MediaInformationParser parseStreamType:@"Audio: adpcm_ms ([2][0][0][0] / 0x0002)"]]);
    assert([@"audio" isEqualToString:[MediaInformationParser parseStreamType:@"Audio: mp3 (mp4a / 0x6134706D)"]]);
    assert([@"audio" isEqualToString:[MediaInformationParser parseStreamType:@"Audio: pcm_u8 ([1][0][0][0] / 0x0001)"]]);
}

void testParseStreamCodec() {
    assert([@"theora" isEqualToString:[MediaInformationParser parseStreamCodec:@"Video: theora"]]);
    assert([@"png" isEqualToString:[MediaInformationParser parseStreamCodec:@"Video: png"]]);
    assert([@"h264" isEqualToString:[MediaInformationParser parseStreamCodec:@"Video: h264 (Main)"]]);
    assert([@"adpcm_ms" isEqualToString:[MediaInformationParser parseStreamCodec:@"Audio: adpcm_ms ([2][0][0][0] / 0x0002)"]]);
    assert([@"mp3" isEqualToString:[MediaInformationParser parseStreamCodec:@"Audio: mp3 (mp4a / 0x6134706D)"]]);
    assert([@"pcm_u8" isEqualToString:[MediaInformationParser parseStreamCodec:@"Audio: pcm_u8 ([1][0][0][0] / 0x0001)"]]);
}

void testParseStreamFullCodec() {
    assert([@"theora" isEqualToString:[MediaInformationParser parseStreamFullCodec:@"Video: theora"]]);
    assert([@"png" isEqualToString:[MediaInformationParser parseStreamFullCodec:@"Video: png"]]);
    assert([@"h264 (main)" isEqualToString:[MediaInformationParser parseStreamFullCodec:@"Video: h264 (Main)"]]);
    assert([@"adpcm_ms ([2][0][0][0] / 0x0002)" isEqualToString:[MediaInformationParser parseStreamFullCodec:@"Audio: adpcm_ms ([2][0][0][0] / 0x0002)"]]);
    assert([@"mp3 (mp4a / 0x6134706d)" isEqualToString:[MediaInformationParser parseStreamFullCodec:@"Audio: mp3 (mp4a / 0x6134706D)"]]);
    assert([@"pcm_u8 ([1][0][0][0] / 0x0001)" isEqualToString:[MediaInformationParser parseStreamFullCodec:@"Audio: pcm_u8 ([1][0][0][0] / 0x0001)"]]);
}

void testParseStreamIndex() {
    assert([[[NSNumber alloc] initWithInt: 0] isEqualToNumber:[MediaInformationParser parseStreamIndex:@"Stream #0:0(und): Audio"]]);
    assert([[[NSNumber alloc] initWithInt: 1] isEqualToNumber:[MediaInformationParser parseStreamIndex:@"Stream #0:1(und): Video"]]);
    assert([[[NSNumber alloc] initWithInt: 2] isEqualToNumber:[MediaInformationParser parseStreamIndex:@"Stream #0:2(und): Audio"]]);
    assert([[[NSNumber alloc] initWithInt: 0] isEqualToNumber:[MediaInformationParser parseStreamIndex:@"Stream #0:0: Video"]]);
    assert([[[NSNumber alloc] initWithInt: 1] isEqualToNumber:[MediaInformationParser parseStreamIndex:@"Stream #0:1: Audio"]]);
    assert([[[NSNumber alloc] initWithInt: 2] isEqualToNumber:[MediaInformationParser parseStreamIndex:@"Stream #0:2: Video"]]);
}

void testParseDuration() {
    parseDuration(nil, nil);
    parseDuration(@"", nil);
    parseDuration(@"N/A", nil);
    parseDuration(@"00:03:33.24", [[NSNumber alloc] initWithInt: 213240]);
    parseDuration(@"00:10:34.53", [[NSNumber alloc] initWithInt: 634530]);
    parseDuration(@"00:00:00.04", [[NSNumber alloc] initWithInt: 40]);
    parseDuration(@"00:00:15.00", [[NSNumber alloc] initWithInt: 15000]);
}

void testParseStartTime() {
    parseStartTime(nil, nil);
    parseStartTime(@"", nil);
    parseStartTime(@"N/A", nil);
    parseStartTime(@"0.000000", [[NSNumber alloc] initWithInt:0]);
    parseStartTime(@"10.003000", [[NSNumber alloc] initWithInt:10003]);
    parseStartTime(@"324.000000", [[NSNumber alloc] initWithInt:324000]);
    parseStartTime(@"-4.000000", [[NSNumber alloc] initWithInt:-4000]);
    parseStartTime(@"14.00030", [[NSNumber alloc] initWithInt:14001]);
    parseStartTime(@"14.00080", [[NSNumber alloc] initWithInt:14001]);
}

void testParseDurationBlock() {
    parseDurationBlock(@"  Duration: 00:03:33.24, start: 0.000000, bitrate: 320 kb/s", [[NSNumber alloc] initWithInt:213240], [[NSNumber alloc] initWithInt:0], [[NSNumber alloc] initWithInt:320]);
    parseDurationBlock(@"  Duration: 00:00:00.04, start: 0.000000, bitrate: 391187 kb/s", [[NSNumber alloc] initWithInt:40], [[NSNumber alloc] initWithInt:0], [[NSNumber alloc] initWithInt:391187]);
    parseDurationBlock(@"  Duration: N/A, bitrate: N/A", nil, nil, nil);
    parseDurationBlock(@"  Duration: 00:00:15.00, start: 0.000000, bitrate: 7764 kb/s", [[NSNumber alloc] initWithInt:15000], [[NSNumber alloc] initWithInt:0], [[NSNumber alloc] initWithInt:7764]);
    parseDurationBlock(@"  Duration: 00:10:34.53, start: 0.000000, bitrate: 4474 kb/s", [[NSNumber alloc] initWithInt:634530], [[NSNumber alloc] initWithInt:0], [[NSNumber alloc] initWithInt:4474]);
}

void testParseInputBlock() {
    parseInputBlock(@"Input #0,", nil, nil);
    parseInputBlock(@"Input #0, ogg, from 'trailer_400p.ogg':", @"ogg", @"trailer_400p.ogg");
    parseInputBlock(@"Input #0, mp3, from 'beethoven_-_symphony_no_9.mp3':", @"mp3", @"beethoven_-_symphony_no_9.mp3");
    parseInputBlock(@"Input #0, image2, from '/data/user/0/com.arthenica.mobileffmpeg.test/cache/colosseum.jpg':", @"image2", @"/data/user/0/com.arthenica.mobileffmpeg.test/cache/colosseum.jpg");
    parseInputBlock(@"Input #0, gif, from 'advanced_zoom_in_and_pan_with_fade_in_out.gif':", @"gif", @"advanced_zoom_in_and_pan_with_fade_in_out.gif");
    parseInputBlock(@"Input #0, mov,mp4,m4a,3gp,3g2,mj2, from 'transition_rotate.mp4':", @"mov,mp4,m4a,3gp,3g2,mj2", @"transition_rotate.mp4");
    parseInputBlock(@"Input #0, png_pipe, from 'https://www.google.com/images/branding/googlelogo/2x/googlelogo_color_272x92dp.png':", @"png_pipe", @"https://www.google.com/images/branding/googlelogo/2x/googlelogo_color_272x92dp.png");
}

void testParseMetadataBlock() {
    parseMetadataBlock(@"      ENCODER:", @"ENCODER", @"");
    parseMetadataBlock(@"      ENCODER:ffmpeg2theora 0.19", @"ENCODER", @"ffmpeg2theora 0.19");
    parseMetadataBlock(@"      ENCODER         : ffmpeg2theora 0.19", @"ENCODER", @"ffmpeg2theora 0.19");
    parseMetadataBlock(@"      creation_time   : 2013-12-16T17:50:04.000000Z", @"creation_time", @"2013-12-16T17:50:04.000000Z");
    parseMetadataBlock(@"      handler_name    : GPAC ISO Audio Handler", @"handler_name", @"GPAC ISO Audio Handler");
    parseMetadataBlock(@"    comment         : Creative Commons Attribution 3.0 - http://bbb3d.renderfarming.net", @"comment", @"Creative Commons Attribution 3.0 - http://bbb3d.renderfarming.net");
    parseMetadataBlock(@"    minor_version   : 1", @"minor_version", @"1");
    parseMetadataBlock(@"    encoder         : Lavf58.12.100", @"encoder", @"Lavf58.12.100");
    parseMetadataBlock(@"    title           : Planet X", @"title", @"Planet X");
    parseMetadataBlock(@"    compatible_brands: isomiso2avc1mp41", @"compatible_brands", @"isomiso2avc1mp41");
}

void assertNotNull(NSObject *object) {
    assert(object != nil);
}

void assertMediaInput(MediaInformation *mediaInformation, NSString *expectedFormat, NSString *expectedPath) {
    NSString *format = [mediaInformation getFormat];
    NSString *path = [mediaInformation getPath];
    if (format == nil) {
        assert(expectedFormat == nil);
    } else {
        assert([format isEqualToString:expectedFormat]);
    }
    if (path == nil) {
        assert(expectedPath == nil);
    } else {
        assert([path isEqualToString:expectedPath]);
    }
}

void assertMediaDuration(MediaInformation *mediaInformation, NSNumber *expectedDuration, NSNumber *expectedStartTime, NSNumber *expectedBitrate) {
    NSNumber *duration = [mediaInformation getDuration];
    NSNumber *startTime = [mediaInformation getStartTime];
    NSNumber *bitrate = [mediaInformation getBitrate];

    if (duration == nil) {
        assert(expectedDuration == nil);
    } else {
        assert([duration isEqualToNumber:expectedDuration]);
    }
    if (startTime == nil) {
        assert(expectedStartTime == nil);
    } else {
        assert([startTime isEqualToNumber:expectedStartTime]);
    }
    if (bitrate == nil) {
        assert(expectedBitrate == nil);
    } else {
        assert([bitrate isEqualToNumber:expectedBitrate]);
    }
}

void testMediaInformationMp3() {
    MediaInformation *mediaInformation = [MediaInformationParser from:MEDIA_INFORMATION_MP3];
    
    assertNotNull(mediaInformation);
    assertMediaInput(mediaInformation, @"mp3", @"beethoven_-_symphony_no_9.mp3");
    assertMediaDuration(mediaInformation, [[NSNumber alloc] initWithInt: 213240], [[NSNumber alloc] initWithInt: 0], [[NSNumber alloc] initWithInt: 320]);
    
    NSArray *streams = [mediaInformation getStreams];
    assertNotNull(streams);
    assert(1 == [streams count]);

    assertAudioStream([streams objectAtIndex:0], [[NSNumber alloc] initWithInt:0], @"mp3", @"mp3", [[NSNumber alloc] initWithInt:48000], @"stereo", @"fltp", [[NSNumber alloc] initWithInt:320]);
}

void testMediaInformationJpg() {
    MediaInformation *mediaInformation = [MediaInformationParser from:MEDIA_INFORMATION_JPG];
    
    assertNotNull(mediaInformation);
    assertMediaInput(mediaInformation, @"image2", @"/data/user/0/com.arthenica.mobileffmpeg.test/cache/colosseum.jpg");
    assertMediaDuration(mediaInformation, [[NSNumber alloc] initWithInt:40], [[NSNumber alloc] initWithInt:0], [[NSNumber alloc] initWithInt:391187]);

    NSArray *streams = [mediaInformation getStreams];
    assertNotNull(streams);
    assert(1 == [streams count]);

    assertVideoStream([streams objectAtIndex:0], [[NSNumber alloc] initWithInt:0], @"mjpeg", @"mjpeg", @"yuvj420p", @"yuvj420p(pc, bt470bg/unknown/unknown)", [[NSNumber alloc] initWithInt:2560], [[NSNumber alloc] initWithInt:1708], @"1:1", @"640:427", nil, nil, @"25", @"25", @"25");
}

void testMediaInformationGif() {
    MediaInformation *mediaInformation = [MediaInformationParser from:MEDIA_INFORMATION_GIF];
    
    assertNotNull(mediaInformation);
    assertMediaInput(mediaInformation, @"gif", @"advanced_zoom_in_and_pan_with_fade_in_out.gif");
    assertMediaDuration(mediaInformation, nil, nil, nil);

    NSArray *streams = [mediaInformation getStreams];
    assertNotNull(streams);
    assert(1 == [streams count]);

    assertVideoStream([streams objectAtIndex:0], [[NSNumber alloc] initWithInt:0], @"gif", @"gif", @"bgra", @"bgra", [[NSNumber alloc] initWithInt:420], [[NSNumber alloc] initWithInt:236], nil, nil, nil, @"6", @"6", @"100", @"100");
}

void testMediaInformationH264() {
    MediaInformation *mediaInformation = [MediaInformationParser from:MEDIA_INFORMATION_H264];
    
    assertNotNull(mediaInformation);
    assertMediaInput(mediaInformation, @"mov,mp4,m4a,3gp,3g2,mj2", @"transition_rotate.mp4");
    assertMediaDuration(mediaInformation, [[NSNumber alloc] initWithInt:15000], [[NSNumber alloc] initWithInt:0], [[NSNumber alloc] initWithInt:7764]);

    NSArray *streams = [mediaInformation getStreams];
    assertNotNull(streams);
    assert(1 == [streams count]);

    assertVideoStream([streams objectAtIndex:0], [[NSNumber alloc] initWithInt:0], @"h264", @"h264 (main) (avc1 / 0x31637661)", @"yuv420p", @"yuv420p", [[NSNumber alloc] initWithInt:1280], [[NSNumber alloc] initWithInt:720], @"1:1", @"16:9", [[NSNumber alloc] initWithInt:7762], @"25", @"30", @"15360", @"60");
}

void testMediaInformationPng() {
    MediaInformation *mediaInformation = [MediaInformationParser from:MEDIA_INFORMATION_PNG];
    
    assertNotNull(mediaInformation);
    assertMediaInput(mediaInformation, @"png_pipe", @"https://www.google.com/images/branding/googlelogo/2x/googlelogo_color_272x92dp.png");
    assertMediaDuration(mediaInformation, nil, nil, nil);

    NSArray *streams = [mediaInformation getStreams];
    assertNotNull(streams);
    assert(1 == [streams count]);

    assertVideoStream([streams objectAtIndex:0], [[NSNumber alloc] initWithInt:0], @"png", @"png", @"rgba", @"rgba(pc)", [[NSNumber alloc] initWithInt:544], [[NSNumber alloc] initWithInt:184], nil, nil, nil, nil, @"25", @"25", @"25");
}

void testMediaInformationH2642() {
    MediaInformation *mediaInformation = [MediaInformationParser from:MEDIA_INFORMATION_H264_2];
    
    assertNotNull(mediaInformation);
    assertMediaInput(mediaInformation, @"h264", @"test.h264");
    assertMediaDuration(mediaInformation, nil, nil, nil);

    NSArray *streams = [mediaInformation getStreams];
    assertNotNull(streams);
    assert(1 == [streams count]);

    assertVideoStream([streams objectAtIndex:0], [[NSNumber alloc] initWithInt:0], @"h264", @"h264 (main)", @"yuv420p", @"yuv420p(tv, bt709, progressive)", [[NSNumber alloc] initWithInt:1920], [[NSNumber alloc] initWithInt:1080], nil, nil, nil, @"25", @"25", @"1200k", @"50");
}

void testMediaInformationMp4() {
    MediaInformation *mediaInformation = [MediaInformationParser from:MEDIA_INFORMATION_MP4];
    
    assertNotNull(mediaInformation);
    assertMediaInput(mediaInformation, @"mov,mp4,m4a,3gp,3g2,mj2", @"http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_2160p_30fps_stereo_abl.mp4");
    assertMediaDuration(mediaInformation, [[NSNumber alloc] initWithInt:634530], [[NSNumber alloc] initWithInt:0], [[NSNumber alloc] initWithInt:10385]);

    NSArray *streams = [mediaInformation getStreams];
    assertNotNull(streams);
    assert(3 == [streams count]);

    assertVideoStream([streams objectAtIndex:0], [[NSNumber alloc] initWithInt:0], @"h264", @"h264 (high) (avc1 / 0x31637661)", @"yuv420p", @"yuv420p", [[NSNumber alloc] initWithInt:3840], [[NSNumber alloc] initWithInt:4320], @"1:1", @"8:9", [[NSNumber alloc] initWithInt:9902], @"30", @"30", @"30k", @"60");
    assertAudioStream([streams objectAtIndex:1], [[NSNumber alloc] initWithInt:1], @"mp3", @"mp3 (mp4a / 0x6134706d)", [[NSNumber alloc] initWithInt:48000], @"stereo", @"fltp", [[NSNumber alloc] initWithInt:160]);
    assertAudioStream([streams objectAtIndex:2], [[NSNumber alloc] initWithInt:2], @"ac3", @"ac3 (ac-3 / 0x332d6361)", [[NSNumber alloc] initWithInt:48000], @"5.1(side)", @"fltp", [[NSNumber alloc] initWithInt:320]);
}

void testMediaInformationMp42() {
    MediaInformation *mediaInformation = [MediaInformationParser from:MEDIA_INFORMATION_MP4_2];
    
    assertNotNull(mediaInformation);
    assertMediaInput(mediaInformation, @"mov,mp4,m4a,3gp,3g2,mj2", @"http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_30fps_stereo_arcd.mp4");
    assertMediaDuration(mediaInformation, [[NSNumber alloc] initWithInt:634530], [[NSNumber alloc] initWithInt:0], [[NSNumber alloc] initWithInt:4474]);

    NSArray *streams = [mediaInformation getStreams];
    assertNotNull(streams);
    assert(3 == [streams count]);

    assertVideoStream([streams objectAtIndex:0], [[NSNumber alloc] initWithInt:0], @"h264", @"h264 (high) (avc1 / 0x31637661)", @"yuv420p", @"yuv420p", [[NSNumber alloc] initWithInt:1920], [[NSNumber alloc] initWithInt:1080], @"1:1", @"16:9", [[NSNumber alloc] initWithInt:3992], @"30", @"30", @"30k", @"60");
    assertAudioStream([streams objectAtIndex:1], [[NSNumber alloc] initWithInt:1], @"mp3", @"mp3 (mp4a / 0x6134706d)", [[NSNumber alloc] initWithInt:48000], @"stereo", @"fltp", [[NSNumber alloc] initWithInt:160]);
    assertAudioStream([streams objectAtIndex:2], [[NSNumber alloc] initWithInt:2], @"ac3", @"ac3 (ac-3 / 0x332d6361)", [[NSNumber alloc] initWithInt:48000], @"5.1(side)", @"fltp", [[NSNumber alloc] initWithInt:320]);
}

void testMediaInformationOgg() {
    MediaInformation *mediaInformation = [MediaInformationParser from:MEDIA_INFORMATION_OGG];
    
    assertNotNull(mediaInformation);
    assertMediaInput(mediaInformation, @"ogg", @"trailer_400p.ogg");
    assertMediaDuration(mediaInformation, [[NSNumber alloc] initWithInt:33000], [[NSNumber alloc] initWithInt:0], [[NSNumber alloc] initWithInt:1057]);

    NSArray *streams = [mediaInformation getStreams];
    assertNotNull(streams);
    assert(2 == [streams count]);

    assertVideoStream([streams objectAtIndex:0], [[NSNumber alloc] initWithInt:0], @"theora", @"theora", @"yuv420p", @"yuv420p(bt470bg/bt470bg/bt709)", [[NSNumber alloc] initWithInt:720], [[NSNumber alloc] initWithInt:400], nil, nil, nil, @"25", @"25", @"25", @"25");
    assertAudioStream([streams objectAtIndex:1], [[NSNumber alloc] initWithInt:1], @"vorbis", @"vorbis", [[NSNumber alloc] initWithInt:48000], @"stereo", @"fltp", [[NSNumber alloc] initWithInt:80]);
}

void testMediaInformationRecording() {
    MediaInformation *mediaInformation = [MediaInformationParser from:MEDIA_INFORMATION_RECORDING];
    
    assertNotNull(mediaInformation);
    assertMediaInput(mediaInformation, @"mov,mp4,m4a,3gp,3g2,mj2", @"/var/mobile/Containers/Data/Application/845A06CD-8427-4D2D-A9A8-F7738063E220/Library/Caches/video.mov");
    assertMediaDuration(mediaInformation, [[NSNumber alloc] initWithInt:2300], [[NSNumber alloc] initWithInt:0], [[NSNumber alloc] initWithInt:16658]);
    
    NSArray *streams = [mediaInformation getStreams];
    assertNotNull(streams);
    assert(4 == [streams count]);
    
    assertVideoStream([streams objectAtIndex:0], [[NSNumber alloc] initWithInt:0], @"h264", @"h264 (avc1 / 0x31637661)", @"yuv420p", @"yuv420p(tv, bt709)", [[NSNumber alloc] initWithInt:1920], [[NSNumber alloc] initWithInt:1080], nil, nil, [[NSNumber alloc] initWithInt:16535], @"29.98", @"29.97", @"600", @"1200");
    assertAudioStream([streams objectAtIndex:1], [[NSNumber alloc] initWithInt:1], @"aac", @"aac (mp4a / 0x6134706d)", [[NSNumber alloc] initWithInt:44100], @"mono", @"fltp", [[NSNumber alloc] initWithInt:96]);
}

/**
 * All parsing tests are initiated from this method
 */
void runMediaInformationParserTests() {

    initTests();

    testParseMetadataBlock();
    testParseInputBlock();

    testParseDurationBlock();
    testParseStartTime();
    testParseDuration();

    testParseStreamIndex();
    testParseStreamFullCodec();
    testParseStreamCodec();

    testParseAudioStreamType();
    testParseAudioSampleRate();
    testParseVideoDimensions();

    testParseVideoStreamSampleAspectRatio();
    testParseVideoStreamDisplayAspectRatio();

    testCount();
    testIndex();

    testParseAudioStream();
    testParseVideoStream();

    testMediaInformationMp3();
    testMediaInformationJpg();
    testMediaInformationGif();
    testMediaInformationH264();
    testMediaInformationPng();
    testMediaInformationH2642();
    testMediaInformationMp4();
    testMediaInformationMp42();
    testMediaInformationOgg();
    testMediaInformationRecording();
    
    
    NSLog(@"MediaInformationParserTests passed.");
}
