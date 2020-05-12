//
// MediaInformationParserTest.m
//
// Copyright (c) 2018-2020 Taner Sener
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
static NSString *MEDIA_INFORMATION_MP4;
static NSString *MEDIA_INFORMATION_OGG;

static void initTests() {
    MEDIA_INFORMATION_MP3 = [NSString stringWithFormat:
                         @"{\n"
                          "     \"streams\": [\n"
                          "         {\n"
                          "             \"index\": 0,\n"
                          "             \"codec_name\": \"mp3\",\n"
                          "             \"codec_long_name\": \"MP3 (MPEG audio layer 3)\",\n"
                          "             \"codec_type\": \"audio\",\n"
                          "             \"codec_time_base\": \"1/44100\",\n"
                          "             \"codec_tag_string\": \"[0][0][0][0]\",\n"
                          "             \"codec_tag\": \"0x0000\",\n"
                          "             \"sample_fmt\": \"fltp\",\n"
                          "             \"sample_rate\": \"44100\",\n"
                          "             \"channels\": 2,\n"
                          "             \"channel_layout\": \"stereo\",\n"
                          "             \"bits_per_sample\": 0,\n"
                          "             \"r_frame_rate\": \"0/0\",\n"
                          "             \"avg_frame_rate\": \"0/0\",\n"
                          "             \"time_base\": \"1/14112000\",\n"
                          "             \"start_pts\": 169280,\n"
                          "             \"start_time\": \"0.011995\",\n"
                          "             \"duration_ts\": 4622376960,\n"
                          "             \"duration\": \"327.549388\",\n"
                          "             \"bit_rate\": \"320000\",\n"
                          "             \"disposition\": {\n"
                          "                 \"default\": 0,\n"
                          "                 \"dub\": 0,\n"
                          "                 \"original\": 0,\n"
                          "                 \"comment\": 0,\n"
                          "                 \"lyrics\": 0,\n"
                          "                 \"karaoke\": 0,\n"
                          "                 \"forced\": 0,\n"
                          "                 \"hearing_impaired\": 0,\n"
                          "                 \"visual_impaired\": 0,\n"
                          "                 \"clean_effects\": 0,\n"
                          "                 \"attached_pic\": 0,\n"
                          "                 \"timed_thumbnails\": 0\n"
                          "             },\n"
                          "             \"tags\": {\n"
                          "                 \"encoder\": \"Lavf\"\n"
                          "             }\n"
                          "         }\n"
                          "     ],\n"
                          "     \"format\": {\n"
                          "         \"filename\": \"sample.mp3\",\n"
                          "         \"nb_streams\": 1,\n"
                          "         \"nb_programs\": 0,\n"
                          "         \"format_name\": \"mp3\",\n"
                          "         \"format_long_name\": \"MP2/3 (MPEG audio layer 2/3)\",\n"
                          "         \"start_time\": \"0.011995\",\n"
                          "         \"duration\": \"327.549388\",\n"
                          "         \"size\": \"13103064\",\n"
                          "         \"bit_rate\": \"320026\",\n"
                          "         \"probe_score\": 51,\n"
                          "         \"tags\": {\n"
                          "             \"encoder\": \"Lavf58.20.100\",\n"
                          "             \"album\": \"Impact\",\n"
                          "             \"artist\": \"Kevin MacLeod\",\n"
                          "             \"comment\": \"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Lorem ipsum dolor sit amet, consectetur adipiscing elit finito.\",\n"
                          "             \"genre\": \"Cinematic\",\n"
                          "             \"title\": \"Impact Moderato\"\n"
                          "         }\n"
                          "     }\n"
                          "}"];

    MEDIA_INFORMATION_JPG = [NSString stringWithFormat:
                         @"{\n"
                          "     \"streams\": [\n"
                          "         {\n"
                          "             \"index\": 0,\n"
                          "             \"codec_name\": \"mjpeg\",\n"
                          "             \"codec_long_name\": \"Motion JPEG\",\n"
                          "             \"profile\": \"Baseline\",\n"
                          "             \"codec_type\": \"video\",\n"
                          "             \"codec_time_base\": \"0/1\",\n"
                          "             \"codec_tag_string\": \"[0][0][0][0]\",\n"
                          "             \"codec_tag\": \"0x0000\",\n"
                          "             \"width\": 1496,\n"
                          "             \"height\": 1729,\n"
                          "             \"coded_width\": 1496,\n"
                          "             \"coded_height\": 1729,\n"
                          "             \"has_b_frames\": 0,\n"
                          "             \"sample_aspect_ratio\": \"1:1\",\n"
                          "             \"display_aspect_ratio\": \"1496:1729\",\n"
                          "             \"pix_fmt\": \"yuvj444p\",\n"
                          "             \"level\": -99,\n"
                          "             \"color_range\": \"pc\",\n"
                          "             \"color_space\": \"bt470bg\",\n"
                          "             \"chroma_location\": \"center\",\n"
                          "             \"refs\": 1,\n"
                          "             \"r_frame_rate\": \"25/1\",\n"
                          "             \"avg_frame_rate\": \"0/0\",\n"
                          "             \"time_base\": \"1/25\",\n"
                          "             \"start_pts\": 0,\n"
                          "             \"start_time\": \"0.000000\",\n"
                          "             \"duration_ts\": 1,\n"
                          "             \"duration\": \"0.040000\",\n"
                          "             \"bits_per_raw_sample\": \"8\",\n"
                          "             \"disposition\": {\n"
                          "                 \"default\": 0,\n"
                          "                 \"dub\": 0,\n"
                          "                 \"original\": 0,\n"
                          "                 \"comment\": 0,\n"
                          "                 \"lyrics\": 0,\n"
                          "                 \"karaoke\": 0,\n"
                          "                 \"forced\": 0,\n"
                          "                 \"hearing_impaired\": 0,\n"
                          "                 \"visual_impaired\": 0,\n"
                          "                 \"clean_effects\": 0,\n"
                          "                 \"attached_pic\": 0,\n"
                          "                 \"timed_thumbnails\": 0\n"
                          "             }\n"
                          "         }\n"
                          "     ],\n"
                          "     \"format\": {\n"
                          "         \"filename\": \"sample.jpg\",\n"
                          "         \"nb_streams\": 1,\n"
                          "         \"nb_programs\": 0,\n"
                          "         \"format_name\": \"image2\",\n"
                          "         \"format_long_name\": \"image2 sequence\",\n"
                          "         \"start_time\": \"0.000000\",\n"
                          "         \"duration\": \"0.040000\",\n"
                          "         \"size\": \"1659050\",\n"
                          "         \"bit_rate\": \"331810000\",\n"
                          "         \"probe_score\": 50\n"
                          "     }\n"
                          "}"];

    MEDIA_INFORMATION_GIF = [NSString stringWithFormat:
                         @"{\n"
                          "     \"streams\": [\n"
                          "         {\n"
                          "             \"index\": 0,\n"
                          "             \"codec_name\": \"gif\",\n"
                          "             \"codec_long_name\": \"CompuServe GIF (Graphics Interchange Format)\",\n"
                          "             \"codec_type\": \"video\",\n"
                          "             \"codec_time_base\": \"12/133\",\n"
                          "             \"codec_tag_string\": \"[0][0][0][0]\",\n"
                          "             \"codec_tag\": \"0x0000\",\n"
                          "             \"width\": 400,\n"
                          "             \"height\": 400,\n"
                          "             \"coded_width\": 400,\n"
                          "             \"coded_height\": 400,\n"
                          "             \"has_b_frames\": 0,\n"
                          "             \"pix_fmt\": \"bgra\",\n"
                          "             \"level\": -99,\n"
                          "             \"refs\": 1,\n"
                          "             \"r_frame_rate\": \"100/9\",\n"
                          "             \"avg_frame_rate\": \"133/12\",\n"
                          "             \"time_base\": \"1/100\",\n"
                          "             \"start_pts\": 0,\n"
                          "             \"start_time\": \"0.000000\",\n"
                          "             \"duration_ts\": 396,\n"
                          "             \"duration\": \"3.960000\",\n"
                          "             \"nb_frames\": \"44\",\n"
                          "             \"disposition\": {\n"
                          "                 \"default\": 0,\n"
                          "                 \"dub\": 0,\n"
                          "                 \"original\": 0,\n"
                          "                 \"comment\": 0,\n"
                          "                 \"lyrics\": 0,\n"
                          "                 \"karaoke\": 0,\n"
                          "                 \"forced\": 0,\n"
                          "                 \"hearing_impaired\": 0,\n"
                          "                 \"visual_impaired\": 0,\n"
                          "                 \"clean_effects\": 0,\n"
                          "                 \"attached_pic\": 0,\n"
                          "                 \"timed_thumbnails\": 0\n"
                          "             }\n"
                          "         }\n"
                          "     ],\n"
                          "     \"format\": {\n"
                          "         \"filename\": \"sample.gif\",\n"
                          "         \"nb_streams\": 1,\n"
                          "         \"nb_programs\": 0,\n"
                          "         \"format_name\": \"gif\",\n"
                          "         \"format_long_name\": \"CompuServe Graphics Interchange Format (GIF)\",\n"
                          "         \"start_time\": \"0.000000\",\n"
                          "         \"duration\": \"3.960000\",\n"
                          "         \"size\": \"1001718\",\n"
                          "         \"bit_rate\": \"2023672\",\n"
                          "         \"probe_score\": 100\n"
                          "     }\n"
                          "}"];

    MEDIA_INFORMATION_MP4 = [NSString stringWithFormat:
                         @"{\n"
                          " \"streams\": [\n"
                          "      {\n"
                          "          \"index\": 0,\n"
                          "          \"codec_name\": \"h264\",\n"
                          "          \"codec_long_name\": \"H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10\",\n"
                          "          \"profile\": \"Main\",\n"
                          "          \"codec_type\": \"video\",\n"
                          "          \"codec_time_base\": \"1/60\",\n"
                          "          \"codec_tag_string\": \"avc1\",\n"
                          "          \"codec_tag\": \"0x31637661\",\n"
                          "          \"width\": 1280,\n"
                          "          \"height\": 720,\n"
                          "          \"coded_width\": 1280,\n"
                          "          \"coded_height\": 720,\n"
                          "          \"has_b_frames\": 0,\n"
                          "          \"sample_aspect_ratio\": \"1:1\",\n"
                          "          \"display_aspect_ratio\": \"16:9\",\n"
                          "          \"pix_fmt\": \"yuv420p\",\n"
                          "          \"level\": 42,\n"
                          "          \"chroma_location\": \"left\",\n"
                          "          \"refs\": 1,\n"
                          "          \"is_avc\": \"true\",\n"
                          "          \"nal_length_size\": \"4\",\n"
                          "          \"r_frame_rate\": \"30/1\",\n"
                          "          \"avg_frame_rate\": \"30/1\",\n"
                          "          \"time_base\": \"1/15360\",\n"
                          "          \"start_pts\": 0,\n"
                          "          \"start_time\": \"0.000000\",\n"
                          "          \"duration_ts\": 215040,\n"
                          "          \"duration\": \"14.000000\",\n"
                          "          \"bit_rate\": \"9166570\",\n"
                          "          \"bits_per_raw_sample\": \"8\",\n"
                          "          \"nb_frames\": \"420\",\n"
                          "          \"disposition\": {\n"
                          "              \"default\": 1,\n"
                          "              \"dub\": 0,\n"
                          "              \"original\": 0,\n"
                          "              \"comment\": 0,\n"
                          "              \"lyrics\": 0,\n"
                          "              \"karaoke\": 0,\n"
                          "              \"forced\": 0,\n"
                          "              \"hearing_impaired\": 0,\n"
                          "              \"visual_impaired\": 0,\n"
                          "              \"clean_effects\": 0,\n"
                          "              \"attached_pic\": 0,\n"
                          "              \"timed_thumbnails\": 0\n"
                          "          },\n"
                          "          \"tags\": {\n"
                          "              \"language\": \"und\",\n"
                          "              \"handler_name\": \"VideoHandler\"\n"
                          "          }\n"
                          "      }\n"
                          "  ],\n"
                          "  \"format\": {\n"
                          "      \"filename\": \"sample.mp4\",\n"
                          "      \"nb_streams\": 1,\n"
                          "      \"nb_programs\": 0,\n"
                          "      \"format_name\": \"mov,mp4,m4a,3gp,3g2,mj2\",\n"
                          "      \"format_long_name\": \"QuickTime / MOV\",\n"
                          "      \"start_time\": \"0.000000\",\n"
                          "      \"duration\": \"14.000000\",\n"
                          "      \"size\": \"16044159\",\n"
                          "      \"bit_rate\": \"9168090\",\n"
                          "      \"probe_score\": 100,\n"
                          "      \"tags\": {\n"
                          "          \"major_brand\": \"isom\",\n"
                          "          \"minor_version\": \"512\",\n"
                          "          \"compatible_brands\": \"isomiso2avc1mp41\",\n"
                          "          \"encoder\": \"Lavf58.33.100\"\n"
                          "      }\n"
                          "  }\n"
                          "}"];

    MEDIA_INFORMATION_PNG = [NSString stringWithFormat:
                         @"{\n"
                          "     \"streams\": [\n"
                          "         {\n"
                          "             \"index\": 0,\n"
                          "             \"codec_name\": \"png\",\n"
                          "             \"codec_long_name\": \"PNG (Portable Network Graphics) image\",\n"
                          "             \"codec_type\": \"video\",\n"
                          "             \"codec_time_base\": \"0/1\",\n"
                          "             \"codec_tag_string\": \"[0][0][0][0]\",\n"
                          "             \"codec_tag\": \"0x0000\",\n"
                          "             \"width\": 1198,\n"
                          "             \"height\": 1198,\n"
                          "             \"coded_width\": 1198,\n"
                          "             \"coded_height\": 1198,\n"
                          "             \"has_b_frames\": 0,\n"
                          "             \"sample_aspect_ratio\": \"1:1\",\n"
                          "             \"display_aspect_ratio\": \"1:1\",\n"
                          "             \"pix_fmt\": \"pal8\",\n"
                          "             \"level\": -99,\n"
                          "             \"color_range\": \"pc\",\n"
                          "             \"refs\": 1,\n"
                          "             \"r_frame_rate\": \"25/1\",\n"
                          "             \"avg_frame_rate\": \"0/0\",\n"
                          "             \"time_base\": \"1/25\",\n"
                          "             \"disposition\": {\n"
                          "                 \"default\": 0,\n"
                          "                 \"dub\": 0,\n"
                          "                 \"original\": 0,\n"
                          "                 \"comment\": 0,\n"
                          "                 \"lyrics\": 0,\n"
                          "                 \"karaoke\": 0,\n"
                          "                 \"forced\": 0,\n"
                          "                 \"hearing_impaired\": 0,\n"
                          "                 \"visual_impaired\": 0,\n"
                          "                 \"clean_effects\": 0,\n"
                          "                 \"attached_pic\": 0,\n"
                          "                 \"timed_thumbnails\": 0\n"
                          "             }\n"
                          "         }\n"
                          "     ],\n"
                          "     \"format\": {\n"
                          "         \"filename\": \"sample.png\",\n"
                          "         \"nb_streams\": 1,\n"
                          "         \"nb_programs\": 0,\n"
                          "         \"format_name\": \"png_pipe\",\n"
                          "         \"format_long_name\": \"piped png sequence\",\n"
                          "         \"size\": \"31533\",\n"
                          "         \"probe_score\": 99\n"
                          "     }\n"
                          "}"];

    MEDIA_INFORMATION_OGG = [NSString stringWithFormat:
                         @"{\n"
                           "    \"streams\": [\n"
                           "        {\n"
                           "            \"index\": 0,\n"
                           "            \"codec_name\": \"theora\",\n"
                           "            \"codec_long_name\": \"Theora\",\n"
                           "            \"codec_type\": \"video\",\n"
                           "            \"codec_time_base\": \"1/25\",\n"
                           "            \"codec_tag_string\": \"[0][0][0][0]\",\n"
                           "            \"codec_tag\": \"0x0000\",\n"
                           "            \"width\": 1920,\n"
                           "            \"height\": 1080,\n"
                           "            \"coded_width\": 1920,\n"
                           "            \"coded_height\": 1088,\n"
                           "            \"has_b_frames\": 0,\n"
                           "            \"pix_fmt\": \"yuv420p\",\n"
                           "            \"level\": -99,\n"
                           "            \"color_space\": \"bt470bg\",\n"
                           "            \"color_transfer\": \"bt709\",\n"
                           "            \"color_primaries\": \"bt470bg\",\n"
                           "            \"chroma_location\": \"center\",\n"
                           "            \"refs\": 1,\n"
                           "            \"r_frame_rate\": \"25/1\",\n"
                           "            \"avg_frame_rate\": \"25/1\",\n"
                           "            \"time_base\": \"1/25\",\n"
                           "            \"start_pts\": 0,\n"
                           "            \"start_time\": \"0.000000\",\n"
                           "            \"duration_ts\": 813,\n"
                           "            \"duration\": \"32.520000\",\n"
                           "            \"disposition\": {\n"
                           "                \"default\": 0,\n"
                           "                \"dub\": 0,\n"
                           "                \"original\": 0,\n"
                           "                \"comment\": 0,\n"
                           "                \"lyrics\": 0,\n"
                           "                \"karaoke\": 0,\n"
                           "                \"forced\": 0,\n"
                           "                \"hearing_impaired\": 0,\n"
                           "                \"visual_impaired\": 0,\n"
                           "                \"clean_effects\": 0,\n"
                           "                \"attached_pic\": 0,\n"
                           "                \"timed_thumbnails\": 0\n"
                           "            },\n"
                           "            \"tags\": {\n"
                           "                \"ENCODER\": \"ffmpeg2theora 0.19\"\n"
                           "            }\n"
                           "        },\n"
                           "        {\n"
                           "            \"index\": 1,\n"
                           "            \"codec_name\": \"vorbis\",\n"
                           "            \"codec_long_name\": \"Vorbis\",\n"
                           "            \"codec_type\": \"audio\",\n"
                           "            \"codec_time_base\": \"1/48000\",\n"
                           "            \"codec_tag_string\": \"[0][0][0][0]\",\n"
                           "            \"codec_tag\": \"0x0000\",\n"
                           "            \"sample_fmt\": \"fltp\",\n"
                           "            \"sample_rate\": \"48000\",\n"
                           "            \"channels\": 2,\n"
                           "            \"channel_layout\": \"stereo\",\n"
                           "            \"bits_per_sample\": 0,\n"
                           "            \"r_frame_rate\": \"0/0\",\n"
                           "            \"avg_frame_rate\": \"0/0\",\n"
                           "            \"time_base\": \"1/48000\",\n"
                           "            \"start_pts\": 0,\n"
                           "            \"start_time\": \"0.000000\",\n"
                           "            \"duration_ts\": 1583850,\n"
                           "            \"duration\": \"32.996875\",\n"
                           "            \"bit_rate\": \"80000\",\n"
                           "            \"disposition\": {\n"
                           "                \"default\": 0,\n"
                           "                \"dub\": 0,\n"
                           "                \"original\": 0,\n"
                           "                \"comment\": 0,\n"
                           "                \"lyrics\": 0,\n"
                           "                \"karaoke\": 0,\n"
                           "                \"forced\": 0,\n"
                           "                \"hearing_impaired\": 0,\n"
                           "                \"visual_impaired\": 0,\n"
                           "                \"clean_effects\": 0,\n"
                           "                \"attached_pic\": 0,\n"
                           "                \"timed_thumbnails\": 0\n"
                           "            },\n"
                           "            \"tags\": {\n"
                           "                \"ENCODER\": \"ffmpeg2theora 0.19\"\n"
                           "            }\n"
                           "        }\n"
                           "    ],\n"
                           "    \"format\": {\n"
                           "        \"filename\": \"sample.ogg\",\n"
                           "        \"nb_streams\": 2,\n"
                           "        \"nb_programs\": 0,\n"
                           "        \"format_name\": \"ogg\",\n"
                           "        \"format_long_name\": \"Ogg\",\n"
                           "        \"start_time\": \"0.000000\",\n"
                           "        \"duration\": \"32.996875\",\n"
                           "        \"size\": \"27873937\",\n"
                           "        \"bit_rate\": \"6757958\",\n"
                           "        \"probe_score\": 100\n"
                           "    }\n"
                           "}"];

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

void assertVideoStream(StreamInformation *stream, NSNumber *index, NSString *codec, NSString *fullCodec, NSString *format, NSNumber *width, NSNumber *height, NSString *sampleAspectRatio, NSString *displayAspectRatio, NSString *bitrate, NSString *averageFrameRate, NSString *realFrameRate, NSString *timeBase, NSString *codecTimeBase) {
    assert(stream != nil);
    assertNumber(index, [stream getIndex]);
    assertString(@"video", [stream getType]);

    assertString(codec, [stream getCodec]);
    assertString(fullCodec, [stream getFullCodec]);
    
    assertString(format, [stream getFormat]);
    
    assertNumber(width, [stream getWidth]);
    assertNumber(height, [stream getHeight]);
    assertString(sampleAspectRatio, [stream getSampleAspectRatio]);
    assertString(displayAspectRatio, [stream getDisplayAspectRatio]);

    assertString(bitrate, [stream getBitrate]);

    assertString(averageFrameRate, [stream getAverageFrameRate]);
    assertString(realFrameRate, [stream getRealFrameRate]);
    assertString(timeBase, [stream getTimeBase]);
    assertString(codecTimeBase, [stream getCodecTimeBase]);
}

void assertAudioStream(StreamInformation *stream, NSNumber *index, NSString *codec, NSString *fullCodec, NSString *sampleRate, NSString *channelLayout, NSString *sampleFormat, NSString *bitrate) {
    assert(stream != nil);
    assertNumber(index, [stream getIndex]);
    assertString(@"audio", [stream getType]);

    assertString(codec, [stream getCodec]);
    assertString(fullCodec, [stream getFullCodec]);

    assertString(sampleRate, [stream getSampleRate]);
    assertString(channelLayout, [stream getChannelLayout]);
    assertString(sampleFormat, [stream getSampleFormat]);
    assertString(bitrate, [stream getBitrate]);
}

void assertNotNull(NSObject *object) {
    assert(object != nil);
}

void assertMediaInput(MediaInformation *mediaInformation, NSString *expectedFormat, NSString *expectedFilename) {
    NSString *format = [mediaInformation getFormat];
    NSString *filename = [mediaInformation getFilename];
    if (format == nil) {
        assert(expectedFormat == nil);
    } else {
        assert([format isEqualToString:expectedFormat]);
    }
    if (filename == nil) {
        assert(expectedFilename == nil);
    } else {
        assert([filename isEqualToString:expectedFilename]);
    }
}

void assertMediaDuration(MediaInformation *mediaInformation, NSString *expectedDuration, NSString *expectedStartTime, NSString *expectedBitrate) {
    NSString *duration = [mediaInformation getDuration];
    NSString *startTime = [mediaInformation getStartTime];
    NSString *bitrate = [mediaInformation getBitrate];

    if (duration == nil) {
        assert(expectedDuration == nil);
    } else {
        assert([duration isEqualToString:expectedDuration]);
    }
    if (startTime == nil) {
        assert(expectedStartTime == nil);
    } else {
        assert([startTime isEqualToString:expectedStartTime]);
    }
    if (bitrate == nil) {
        assert(expectedBitrate == nil);
    } else {
        assert([bitrate isEqualToString:expectedBitrate]);
    }
}

void assertTag(MediaInformation *mediaInformation, NSString *expectedKey, NSString *expectedValue) {
    NSDictionary *tags = [mediaInformation getTags];
    assertNotNull(tags);
    
    NSString *value = [tags valueForKey:expectedKey];
    assertNotNull(value);
    
    assert([value isEqualToString:expectedValue]);
}

void assertStreamTag(StreamInformation *streamInformation, NSString *expectedKey, NSString *expectedValue) {
    NSDictionary *tags = [streamInformation getTags];
    assertNotNull(tags);
    
    NSString *value = [tags valueForKey:expectedKey];
    assertNotNull(value);
    
    assert([value isEqualToString:expectedValue]);
}

void testMediaInformationMp3() {
    MediaInformation *mediaInformation = [MediaInformationParser from:MEDIA_INFORMATION_MP3];
    
    assertNotNull(mediaInformation);
    assertMediaInput(mediaInformation, @"mp3", @"sample.mp3");
    assertMediaDuration(mediaInformation, @"327.549388", @"0.011995", @"320026");
    
    assertTag(mediaInformation, @"comment", @"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Lorem ipsum dolor sit amet, consectetur adipiscing elit finito.");
    assertTag(mediaInformation, @"album", @"Impact");
    assertTag(mediaInformation, @"title", @"Impact Moderato");
    assertTag(mediaInformation, @"artist", @"Kevin MacLeod");
    
    NSArray *streams = [mediaInformation getStreams];
    assertNotNull(streams);
    assert(1 == [streams count]);

    assertAudioStream([streams objectAtIndex:0], [[NSNumber alloc] initWithInt:0], @"mp3", @"MP3 (MPEG audio layer 3)", @"44100", @"stereo", @"fltp", @"320000");
}

void testMediaInformationJpg() {
    MediaInformation *mediaInformation = [MediaInformationParser from:MEDIA_INFORMATION_JPG];
    
    assertNotNull(mediaInformation);
    assertMediaInput(mediaInformation, @"image2", @"sample.jpg");
    assertMediaDuration(mediaInformation, @"0.040000", @"0.000000", @"331810000");

    NSArray *streams = [mediaInformation getStreams];
    assertNotNull(streams);
    assert(1 == [streams count]);

    assertVideoStream([streams objectAtIndex:0], [[NSNumber alloc] initWithInt:0], @"mjpeg", @"Motion JPEG", @"yuvj444p", [[NSNumber alloc] initWithInt:1496], [[NSNumber alloc] initWithInt:1729], @"1:1", @"1496:1729", nil, @"0/0", @"25/1", @"1/25", @"0/1");
}

void testMediaInformationGif() {
    MediaInformation *mediaInformation = [MediaInformationParser from:MEDIA_INFORMATION_GIF];
    
    assertNotNull(mediaInformation);
    assertMediaInput(mediaInformation, @"gif", @"sample.gif");
    assertMediaDuration(mediaInformation, @"3.960000", @"0.000000", @"2023672");

    NSArray *streams = [mediaInformation getStreams];
    assertNotNull(streams);
    assert(1 == [streams count]);

    assertVideoStream([streams objectAtIndex:0], [[NSNumber alloc] initWithInt:0], @"gif", @"CompuServe GIF (Graphics Interchange Format)", @"bgra", [[NSNumber alloc] initWithInt:400], [[NSNumber alloc] initWithInt:400], nil, nil, nil, @"133/12", @"100/9", @"1/100", @"12/133");
}

void testMediaInformationMp4() {
    MediaInformation *mediaInformation = [MediaInformationParser from:MEDIA_INFORMATION_MP4];
    
    assertNotNull(mediaInformation);
    assertMediaInput(mediaInformation, @"mov,mp4,m4a,3gp,3g2,mj2", @"sample.mp4");
    assertMediaDuration(mediaInformation, @"14.000000", @"0.000000", @"9168090");

    assertTag(mediaInformation, @"major_brand", @"isom");
    assertTag(mediaInformation, @"minor_version", @"512");
    assertTag(mediaInformation, @"compatible_brands", @"isomiso2avc1mp41");
    assertTag(mediaInformation, @"encoder", @"Lavf58.33.100");

    NSArray *streams = [mediaInformation getStreams];
    assertNotNull(streams);
    assert(1 == [streams count]);

    assertVideoStream([streams objectAtIndex:0], [[NSNumber alloc] initWithInt:0], @"h264", @"H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10", @"yuv420p", [[NSNumber alloc] initWithInt:1280], [[NSNumber alloc] initWithInt:720], @"1:1", @"16:9", @"9166570", @"30/1", @"30/1", @"1/15360", @"1/60");
    
    assertStreamTag([streams objectAtIndex:0], @"language", @"und");
    assertStreamTag([streams objectAtIndex:0], @"handler_name", @"VideoHandler");
}

void testMediaInformationPng() {
    MediaInformation *mediaInformation = [MediaInformationParser from:MEDIA_INFORMATION_PNG];
    
    assertNotNull(mediaInformation);
    assertMediaInput(mediaInformation, @"png_pipe", @"sample.png");
    assertMediaDuration(mediaInformation, nil, nil, nil);

    NSArray *streams = [mediaInformation getStreams];
    assertNotNull(streams);
    assert(1 == [streams count]);

    assertVideoStream([streams objectAtIndex:0], [[NSNumber alloc] initWithInt:0], @"png", @"PNG (Portable Network Graphics) image", @"pal8", [[NSNumber alloc] initWithInt:1198], [[NSNumber alloc] initWithInt:1198], @"1:1", @"1:1", nil, @"0/0", @"25/1", @"1/25", @"0/1");
}

void testMediaInformationOgg() {
    MediaInformation *mediaInformation = [MediaInformationParser from:MEDIA_INFORMATION_OGG];
    
    assertNotNull(mediaInformation);
    assertMediaInput(mediaInformation, @"ogg", @"sample.ogg");
    assertMediaDuration(mediaInformation, @"32.996875", @"0.000000", @"6757958");

    NSArray *streams = [mediaInformation getStreams];
    assertNotNull(streams);
    assert(2 == [streams count]);

    assertVideoStream([streams objectAtIndex:0], [[NSNumber alloc] initWithInt:0], @"theora", @"Theora", @"yuv420p", [[NSNumber alloc] initWithInt:1920], [[NSNumber alloc] initWithInt:1080], nil, nil, nil, @"25/1", @"25/1", @"1/25", @"1/25");
    assertAudioStream([streams objectAtIndex:1], [[NSNumber alloc] initWithInt:1], @"vorbis", @"Vorbis", @"48000", @"stereo", @"fltp", @"80000");

    assertStreamTag([streams objectAtIndex:0], @"ENCODER", @"ffmpeg2theora 0.19");
    assertStreamTag([streams objectAtIndex:1], @"ENCODER", @"ffmpeg2theora 0.19");
}

/**
 * All parser tests are initiated from this method
 */
void testMediaInformationParser(void) {

    initTests();

    testMediaInformationMp3();
    testMediaInformationJpg();
    testMediaInformationGif();
    testMediaInformationMp4();
    testMediaInformationPng();
    testMediaInformationOgg();
    
    NSLog(@"MediaInformationParserTest passed.");
}
