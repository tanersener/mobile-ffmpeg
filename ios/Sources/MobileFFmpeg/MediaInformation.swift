//
//  MediaInformation.swift
//  FFmpegKit
//
//  Created by Johnny Vega on 11/4/20.
//

import Foundation

@objcMembers public class MediaInformation: NSObject, Decodable {
    
    @objcMembers public class Stream: NSObject, Decodable {
        
        @objcMembers public class Disposition: NSObject, Decodable {
            public var `default`: Int
            public var dub: Int
            public var original: Int
            public var comment: Int
            public var lyrics: Int
            public var karaoke: Int
            public var forced: Int
            public var hearingImpaired: Int
            public var visualImpaired: Int
            public var cleanEffects: Int
            public var attachedPic: Int
            public var timedThumbnails: Int
            
            enum CodingKeys: String, CodingKey {
                case `default`
                case dub
                case original
                case comment
                case lyrics
                case karaoke
                case forced
                case hearingImpaired = "hearing_impaired"
                case visualImpaired = "visual_impaired"
                case cleanEffects = "clean_effects"
                case attachedPic = "attached_pic"
                case timedThumbnails = "timed_thumbnails"
            }
        }
        
        public var index: Int = 0
        public var codecName: String = ""
        public var codecLongName: String?
        public var codecType: String?
        public var profile: String?
        public var codecTimeBase: String?
        public var codecTagString: String?
        public var codecTag: String?
        public var width: Int?
        public var height: Int?
        public var codedWidth: Int?
        public var codedHeight: Int?
        public var closedCaptions: Bool?
        public var hasBFrames: Int?
        public var pixFmt: String?
        public var level: Int?
        public var chromaLocation: String?
        public var fieldOrder: String?
        public var refs: Int?
        public var isAvc: Bool?
        public var nalLengthSize: Int?

        public var id: String?
        public var rFrameRate: String?
        public var avgFrameRate: String?
        public var timeBase: String?
        public var startPts: Double?
        public var startTime: TimeInterval?
        public var durationTs: TimeInterval?
        public var duration: TimeInterval?
        public var bitsPerRawSample: Int?
        public var displayAspectRatio: String?
        
        public var sampleFmt: String?
        public var sampleRate: String?
        public var channels: Int?
        public var channelLayout: String?
        public var bitsPerSample: Int?
        public var bitRate: Int?
        public var disposition: Disposition?
        
        private enum CodingKeys: String, CodingKey {
            case index
            case codecName = "codec_name"
            case codecLongName = "codec_long_name"
            case codecType = "codec_type"
            case codecTimeBase =  "codec_time_base"
            case codecTagString = "codec_tag_string"
            case codecTag = "codec_tag"
            case profile
            case width
            case height
            case codedWidth = "codec_witdth"
            case codedHeight = "codec_height"
            case closedCaptions = "closed_captions"
            case hasBFrames =  "has_b_frames"
            case pixFmt = "pix_fmt"
            case level
            case chromaLocation = "chroma_location"
            case fieldOrder = "field_order"
            case refs
            case isAvc = "is_avc"
            case nalLengthSize = "nal_length_size"
            case id
            case rFrameRate = "r_frame_rate"
            case avgFrameRate = "avg_frame_rate"
            case timeBase = "time_base"
            case startPts = "start_pts"
            case startTime = "start_time"
            case durationTs = "duration_ts"
            case duration
            case bitsPerRawSample = "bits_per_raw_sample"
            case disposition
            case sampleFmt = "sample_fmt"
            case sampleRate = "sample_rate"
            case channels
            case channelLayout = "channel_layout"
            case bitsPerSample = "bits_per_sample"
            case bitRate = "bit_rate"
            case displayAspectRatio = "display_aspect_ratio"
        }
        
        public override init() {
            super.init()
        }

        required public init(from decoder: Decoder) throws {
            let values = try decoder.container(keyedBy: CodingKeys.self)
            index = try values.decode(Int.self, forKey: .index)
            codecName = try values.decode(String.self, forKey: .codecName)
            codecLongName = try values.decode(String.self, forKey: .codecLongName)
            codecType = try values.decode(String.self, forKey: .codecType)
            codecTagString = try values.decode(String.self, forKey: .codecTagString)
            codecTag = try values.decode(String.self, forKey: .codecTag)
            id = try? values.decode(String.self, forKey: .id)
            rFrameRate = try values.decode(String.self, forKey: .rFrameRate)
            avgFrameRate = try values.decode(String.self, forKey: .avgFrameRate)
            timeBase = try values.decode(String.self, forKey: .timeBase)
            startPts  = try values.decode(Double.self, forKey: .startPts)
            startTime = try TimeInterval(values.decode(String.self, forKey: .startTime))!
            durationTs = try? values.decode(TimeInterval.self, forKey: .durationTs)
            duration = try? TimeInterval(values.decode(String.self, forKey: .duration))
            disposition = try values.decode(Disposition.self, forKey: .disposition)
            profile = try? values.decode(String.self, forKey: .profile)
            codecTimeBase = try? values.decode(String.self, forKey: .codecTimeBase)
            width = try? values.decode(Int.self, forKey: .width)
            height = try? values.decode(Int.self, forKey: .height)
            codedWidth = try? values.decode(Int.self, forKey: .codedWidth)
            codedHeight = try? values.decode(Int.self, forKey: .codedHeight)
            closedCaptions = try? values.decode(Bool.self, forKey: .closedCaptions)
            hasBFrames = try? values.decode(Int.self, forKey: .hasBFrames)
            pixFmt = try? values.decode(String.self, forKey: .pixFmt)
            level = try? values.decode(Int.self, forKey: .level)
            chromaLocation = try? values.decode(String.self, forKey: .chromaLocation)
            fieldOrder = try? values.decode(String.self, forKey: .fieldOrder)
            refs = try? values.decode(Int.self, forKey: .refs)
            isAvc = try? values.decode(Bool.self, forKey: .isAvc)
            nalLengthSize = try? values.decode(Int.self, forKey: .nalLengthSize)
            sampleFmt = try? values.decode(String.self, forKey: .sampleFmt)
            sampleRate = try? values.decode(String.self, forKey: .sampleRate)
            channels = try? values.decode(Int.self, forKey: .channels)
            channelLayout = try? values.decode(String.self, forKey: .channelLayout)
            bitsPerSample = try? values.decode(Int.self, forKey: .bitsPerSample)
            bitRate = try? values.decode(Int.self, forKey: .bitRate)
            displayAspectRatio = try? values.decode(String.self, forKey: .displayAspectRatio)
            super.init()
        }
    }
    
    
    public var streams: [Stream] = []
    public var filename: String = ""
    public var nbStreams: Int = 0
    public var nbPrograms: Int = 0
    public var formatName: String = ""
  //  var formatLongName: String
    public var startTime: TimeInterval = 0
    public var duration: TimeInterval = 0
    public var size: Int = 0
    public var bitRate: Double = 0
    public var probeScore: Int = 0
    
    private enum CodingKeys: String, CodingKey {
        case streams
        case format
    }
    
    private enum FormatKeys: String, CodingKey {
        case filename
        case nbStreams = "nb_streams"
        case nbPrograms = "nb_programs"
        case formatName = "format_name"
      //  case formatLongName = "format_long_name"
        case startTime = "start_time"
        case duration
        case size
        case bitRate = "bit_rate"
        case probeScore = "probe_score"
    }
    
    public override init() {
        super.init()
    }
    
    required public init(from decoder: Decoder) throws {
        let values = try decoder.container(keyedBy: CodingKeys.self)
        let additionalInfo = try values.nestedContainer(keyedBy: FormatKeys.self, forKey: .format)
        filename = try additionalInfo.decode(String.self, forKey: .filename)
        nbStreams = try additionalInfo.decode(Int.self, forKey: .nbStreams)
        nbPrograms = try additionalInfo.decode(Int.self, forKey: .nbPrograms)
        formatName = try additionalInfo.decode(String.self, forKey: .formatName)
       // formatLongName = try additionalInfo.decode(String.self, forKey: .formatLongName)
        startTime = try TimeInterval(additionalInfo.decode(String.self, forKey: .startTime))!
        duration = try TimeInterval(additionalInfo.decode(String.self, forKey: .duration))!
        size = try Int(additionalInfo.decode(String.self, forKey: .size))!
        bitRate = try Double(additionalInfo.decode(String.self, forKey: .bitRate))!
        probeScore = try additionalInfo.decode(Int.self, forKey: .probeScore)
        
        self.streams = try values.decode([Stream].self, forKey: .streams)
        super.init()
    }
    

}


