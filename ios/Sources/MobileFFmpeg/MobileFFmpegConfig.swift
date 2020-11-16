//
//  MobileFFmpegConfig.swift
//  FFmpegKit
//
//  Created by Johnny Vega on 10/19/20.
//

import Foundation
import Cfftools

public let AV_LOG_QUIET: Int32 = -8
public let AV_LOG_PANIC: Int32 = 0
public let AV_LOG_FATAL: Int32 = 8
public let AV_LOG_ERROR: Int32 = 16
public let AV_LOG_WARNING: Int32 = 24
public let AV_LOG_INFO: Int32 = 32
public let AV_LOG_VERBOSE: Int32 = 40
public let AV_LOG_DEBUG: Int32 = 48

/** Common return code values */
public let RETURN_CODE_SUCCESS: Int32 = 0
public let RETURN_CODE_CANCEL: Int32 = 255
public var LIB_NAME = "mobile-ffmpeg"
public let MOBILE_FFMPEG_PIPE_PREFIX = "mf_pipe_"

private var supportedExternalLibraries: Array<String> = []
private var lastCreatedPipeIndex: Int?

internal func scan<S : Sequence, U>(_ seq: S, _ initial: U, _ combine: (U, S.Iterator.Element) -> U) -> [U] {
    var result: [U] = []
    result.reserveCapacity(seq.underestimatedCount)
    var runningResult = initial
    for element in seq {
        runningResult = combine(runningResult, element)
        result.append(runningResult)
    }
    return result
}

///
/// Convert an Array of Strings to Array of C Strings.
///
/// - Parameter args: Array of Strings
/// - Parameter body: Calls the given closure with an Array of C Strings (represented as a pointer to the content of the string).
///

internal func withArrayOfCStrings<R>(_ args: [String], _ body: ([UnsafeMutablePointer<CChar>?]) -> R) -> R {
  let argsCounts = Array(args.map { $0.utf8.count + 1 })
  let argsOffsets = [ 0 ] + scan(argsCounts, 0, +)
  let argsBufferSize = argsOffsets.last!

  var argsBuffer: [UInt8] = []
  argsBuffer.reserveCapacity(argsBufferSize)
    
  for arg in args {
    argsBuffer.append(contentsOf: arg.utf8)
    argsBuffer.append(0)
  }

  return argsBuffer.withUnsafeMutableBufferPointer {
    (argsBuffer) in
    let ptr = UnsafeMutableRawPointer(argsBuffer.baseAddress!).bindMemory(
      to: CChar.self, capacity: argsBuffer.count)
    var cStrings: [UnsafeMutablePointer<CChar>?] = argsOffsets.map { ptr + $0 }
    cStrings[cStrings.count - 1] = nil
    return body(cStrings)
  }
}
///
/// errorLogData: Maintain last log output as Error description
///
internal var errorLogData: String?

///
/// MobileFFmpeg Exception errors
///

@objcMembers public class MobileFFmpegError: NSError {
    
    @objc public static var FFprobeError: MobileFFmpegError {
        return MobileFFmpegError(domain: "MyVideo", code: 901, userInfo: [NSLocalizedDescriptionKey : NSLocalizedString(errorLogData ?? "ffprobe finished with error", comment: "FFprobe Execution error")])
    }
    
    @objc public static var FFmpegError: MobileFFmpegError {
        return MobileFFmpegError(domain: "MyVideo", code: 902, userInfo: [NSLocalizedDescriptionKey : NSLocalizedString(errorLogData ?? "ffmpeg finished with error", comment: "FFmpeg Execution error")])
    }
    
    override init(domain: String, code: Int, userInfo: [String : Any]?) {
        super.init(domain: domain, code: code, userInfo: userInfo)
    }
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
    }
}

/// Switf native Error Type, can't be use because it is not available to Objc

/*@objc public enum MobileFFmpegError: Int, Error {
    case FFprobeError
    case FFmpegError
}

///
/// MobileFFmpeg error descriptions
///

extension MobileFFmpegError: LocalizedError {
    public var errorDescription: String? {
        switch self {
        case .FFprobeError:
            return NSLocalizedString(errorLogData ?? "ffprobe finished with error", comment: "FFprobe Execution error")
        case .FFmpegError:
            return NSLocalizedString(errorLogData ?? "ffmpeg finished with error", comment: "FFmpeg Execution error")
        }
    }
}
 */

@objcMembers public class MobileFFmpegConfig: NSObject {
    
    ///
    /// Sets and overrides fontconfig configuration directory.
    ///
    /// - Parameter path: directory which contains fontconfig configuration (fonts.conf)
    ///
    
    @objc public class func setFontconfigConfigurationPath(_ path: String) {
        path.withCString { cPath -> Void in
            setenv("FONTCONFIG_PATH", cPath, 1)
        }
    }
    

    ///
    /// Registers fonts inside the given path, so they are available in FFmpeg filters.
    ///
    /// Note that you need to build MobileFFmpeg with fontconfig
    /// enabled or use a prebuilt package with fontconfig inside to use this feature.
    ///
    /// - Parameter fontDirectoryPath: directory which contains fonts (.ttf and .otf files)
    /// - Parameter fontNameMapping: custom font name mappings, useful to access your fonts with more friendly names
    ///
    
    @objc public class func setFontDirectory(path fontPath: String, withFontNameMapping mapping: [String : String]?) {
        
        var isDirectory: ObjCBool = ObjCBool(true)
        let activeLogLevel = av_log_get_level()
        let logIsActive = activeLogLevel != AV_LOG_QUIET && AV_LOG_WARNING <= activeLogLevel
        var validFontNameMappingCount = 0
        let tempConfigurationDirectory = (NSTemporaryDirectory() as NSString).appendingPathComponent(".mobileffmpeg")
        let fontConfigurationFile = (tempConfigurationDirectory as NSString).appendingPathComponent("fonts.conf")
        
        if !(FileManager.default.fileExists(atPath: tempConfigurationDirectory, isDirectory: &isDirectory)) {
            
            do {
            try FileManager.default.createDirectory(atPath: tempConfigurationDirectory, withIntermediateDirectories: true, attributes: nil)
                
            } catch {
                if logIsActive {
                    NSLog("Failed to set font directory. Error received while creating temp conf directory: %@.", error.localizedDescription)
                }
                return
            }
               

            if logIsActive {
                NSLog("Created temporary font conf directory: TRUE.")
            }
        }
        
        if FileManager.default.fileExists(atPath: fontConfigurationFile, isDirectory: &isDirectory) {
            do {
                try FileManager.default.removeItem(atPath: fontConfigurationFile)
                NSLog("Deleted old temporary font configuration: TRUE")
            } catch {
                if logIsActive {
                    NSLog("Deleted old temporary font configuration: FALSE")
                }
            }
        }
        
        var fontNameMappingBlock = ""
        
        for fontName in (mapping ?? [:]).keys  {
            if let mappedFontName = mapping?[fontName],
               !fontName.isEmpty,
               !mappedFontName.isEmpty {

                fontNameMappingBlock = String(format: "%@\n%@\n%@%@%@\n%@\n%@\n%@%@%@\n%@\n%@\n",
                    "        <match target=\"pattern\">",
                    "                <test qual=\"any\" name=\"family\">",
                    "                        <string>", fontName, "</string>",
                    "                </test>",
                    "                <edit name=\"family\" mode=\"assign\" binding=\"same\">",
                    "                        <string>", mappedFontName, "</string>",
                    "                </edit>",
                    "        </match>")

                validFontNameMappingCount += 1
            }
        }
        
        let fontConfiguration = String(format: "%@\n%@\n%@\n%@\n%@%@%@\n%@\n%@\n",
                                "<?xml version=\"1.0\"?>",
                                "<!DOCTYPE fontconfig SYSTEM \"fonts.dtd\">",
                                "<fontconfig>",
                                "    <dir>.</dir>",
                                "    <dir>", fontPath, "</dir>",
                                fontNameMappingBlock,
                                "</fontconfig>")
        do {
            try fontConfiguration.write(toFile: fontConfigurationFile, atomically: true, encoding: .utf8)
        } catch {
            if logIsActive {
                NSLog("Failed to set font directory. Error received while saving font configuration: %@.", error.localizedDescription)
            }
            return
        }
        
        if logIsActive {
            NSLog("Saved new temporary font configuration with %d font name mappings.", validFontNameMappingCount)
        }

        MobileFFmpegConfig.setFontconfigConfigurationPath(tempConfigurationDirectory)

        if logIsActive {
            NSLog("Font directory %@ registered successfully.", fontPath)
        }
    }
    
    ///
    /// Parses the given command into arguments.
    ///
    /// - Parameter command: string command
    /// - Returns: Array of arguments
    ///
    
    @objc public class func parseArguments(_ command: String) -> Array<String> {
        var argumentArray = Array<String>()
        var currentArgument: String = ""
        var singleQuoteStarted: Bool = false
        var doubleQuoteStarted: Bool = false
        let nullChar = Character(Unicode.Scalar(0))
        for i in command.indices {
            var previousChar: Character
            if i > command.startIndex {
                previousChar = command[command.index(before: i)]
            } else {
                previousChar = Character(Unicode.Scalar(0))
            }
            let currentChar = command[i]
            if currentChar == " " {
                if singleQuoteStarted || doubleQuoteStarted {
                    currentArgument.append(currentChar)
                } else if currentArgument.count > 0 {
                    argumentArray.append(currentArgument)
                    currentArgument = ""
                }
            } else if currentChar == "\'" && (previousChar == nullChar || previousChar != "\\") {
                if singleQuoteStarted {
                    singleQuoteStarted = false
                } else if doubleQuoteStarted {
                    currentArgument.append(currentChar)
                } else {
                    singleQuoteStarted = true
                }
            } else if currentChar == "\"" && (previousChar == nullChar || previousChar != "\\") {
                if doubleQuoteStarted {
                    doubleQuoteStarted = false
                } else if singleQuoteStarted {
                    currentArgument.append(currentChar)
                } else {
                    doubleQuoteStarted = true
                }
            } else {
                currentArgument.append(currentChar)
            }
        }
        if currentArgument.count > 0 {
            argumentArray.append(currentArgument)
        }
        return argumentArray
    }
}
