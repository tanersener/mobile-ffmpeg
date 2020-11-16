//
//  MobileFFprobe.swift
//  FFmpegKit
//
//  Created by Johnny Vega on 11/4/20.
//

import Foundation
import Combine
import Cfftools


///
/// Main class for `FFprobe` operations.
///

@objcMembers public class MobileFFprobe: NSObject {

    public static var logLevel: Int32 = AV_LOG_INFO
    
    //
    // Function to configure the AVLogging system
    //
    
    private class func setupLogging() {
        av_log_set_callback({ (ptr: UnsafeMutableRawPointer?, level: Int32, format: UnsafePointer<Int8>?, vargs: CVaListPointer?) -> Void in
 
            var  level = level
            // DO NOT PROCESS UNWANTED LOGS
            if (level >= 0) {
                level &= 0xff;
            }
            let activeLogLevel = av_log_get_level();

            // AV_LOG_STDERR logs are always redirected
            if ((activeLogLevel == AV_LOG_QUIET && level != AV_LOG_STDERR) || (level > activeLogLevel)) {
                return;
            }
            guard let unwrappedFormat = format,
                  let format = String(cString: unwrappedFormat, encoding: .utf8) else { return }
            
            if let args = vargs {
                let logData = NSString(format: format, arguments: args) as String
                let actualLog = Thread.current.threadDictionary.value(forKey: "logData") as? String ?? ""
                Thread.current.threadDictionary.setValue(actualLog + logData, forKey: "logData")
            }
            return
        })
    }
    
    ///
    /// Synchronously executes FFprobe with arguments provided.
    ///
    /// - Parameter arguments: FFprobe command options/arguments as string array
    /// - Parameter completionHandler: The completion handler to call when the ffprobe execution has finished.
    /// This completion handler takes the following parameters:
    /// - Parameter returnCode: zero on successful execution, 255 on user cancel and non-zero on error
    /// - Parameter returnedOutput: FFprobe output as  an String object, or nil if session failed
    /// - Parameter error: An error object that indicates why the request failed, or nil if the request was successful.
    ///
    
    @objc public class func execute(withArguments arguments: Array<String>,completionHandler completion: (_ returnCode: Int32,_ returnedOutput: String?,_ error: MobileFFmpegError?)->Void) {
        var returnCode: Int32 = -1
        Thread.current.threadDictionary.removeObject(forKey: "logData")
        
        av_log_set_level(self.logLevel)
        
        self.setupLogging()

        withArrayOfCStrings([LIB_NAME] + arguments) { args  in
            var vargs = args
            vargs.withUnsafeMutableBufferPointer { pointer in
                returnCode = ffprobe_execute(Int32(arguments.count + 1), pointer.baseAddress)
            }
        }
        let lastLogOutput = Thread.current.threadDictionary.value(forKey: "logData") as? String
        if returnCode != RETURN_CODE_SUCCESS {
            completion(returnCode,nil, MobileFFmpegError.FFprobeError)
        } else {
            completion(returnCode,lastLogOutput,nil)
        }
    }
    
    
    ///
    /// Synchronously executes FFprobe command provided. Space character is used to split command into arguments.
    ///
    /// - Parameter withCommand: FFprobe command
    /// - Parameter completionHandler: The completion handler to call when the ffprobe execution has finished.
    /// This completion handler takes the following parameters:
    /// - Parameter returnCode: zero on successful execution, 255 on user cancel and non-zero on error
    /// - Parameter returnedOutput: FFprobe output as  an String object, or nil if session failed
    /// - Parameter error: An error object that indicates why the request failed, or nil if the request was successful.
    ///
    
    @objc public class func execute(withCommand command: String,completionHandler completion: (_ returnCode: Int32,_ returnedOutput: String?, _ error: MobileFFmpegError?)->Void) {
        MobileFFprobe.execute(withArguments: MobileFFmpegConfig.parseArguments(command), completionHandler: completion)
    }
    
    
    ///
    /// Returns media information for the given file.
    ///
    /// - Parameter url: path or URI of media file
    /// - Returns  An MediaInformation object with the media metadata. This method returns nil and is marked with the throws keyword to indicate that it throws an error in cases of failure.
    ///
    /// - Note: You call this method in a try expression and handle any errors in the catch clauses of a do statement, as described in Error Handling in The Swift Programming Language and About Imported Cocoa Error Parameters.
    ///
    
    @objc public class func getMediaInformation(forURL path: String) throws -> MediaInformation {
        var mediaInfo: MediaInformation = MediaInformation()
        var ffprobeError: MobileFFmpegError?
        self.execute(withArguments: ["-v", "error", "-hide_banner", "-print_format", "json", "-show_format", "-show_streams", "-i", path]) { rc, output, error in
            if rc == RETURN_CODE_SUCCESS {
                guard let jsonString = output,
                     let jsonData = jsonString.data(using: .utf8) else { return }
                let decoder = JSONDecoder()
                do {
                    mediaInfo = try decoder.decode(MediaInformation.self, from: jsonData)
                } catch {
                    errorLogData = error.localizedDescription
                    ffprobeError = MobileFFmpegError.FFprobeError
                }
            } else {
               ffprobeError = error
            }
        }
        guard let theError = ffprobeError else { return mediaInfo }
        throw theError
    }
    
    ///
    /// Returns media information for the given command.
    ///
    /// - Parameter command: string with command arguments. Space character is used to split command into arguments.
    /// - Returns: A Foundation object if the command requested to return the metadata using the JSON format, otherwise a String object wiith the command output.  This method returns nil and is marked with the throws keyword to indicate that it throws an error in cases of failure.
    ///
    /// - Note: You call this method in a try expression and handle any errors in the catch clauses of a do statement, as described in Error Handling in The Swift Programming Language and About Imported Cocoa Error Parameters.
    ///
    
    @objc public class func getMediaInformationFromCommand(_ command: String) throws -> Any {
        return try self.getMediaInformationFromCommand(withArguments: MobileFFmpegConfig.parseArguments(command))
    }
    
    
    ///
    /// Returns media information for given arguments.
    ///
    /// - Parameter arguments: String Array with command arguments.
    /// - Returns: A Foundation object if the command requested to return the metadata using the JSON format, otherwise a String object wiith the command output.  This method returns nil and is marked with the throws keyword to indicate that it throws an error in cases of failure.
    ///
    /// - Note: You call this method in a try expression and handle any errors in the catch clauses of a do statement, as described in Error Handling in The Swift Programming Language and About Imported Cocoa Error Parameters.
    ///
    
   @objc public class func getMediaInformationFromCommand(withArguments arguments: Array<String>) throws -> Any {
        self.logLevel = AV_LOG_INFO
        var ffprobeError: Error?
        var ffprobeOutput: Any!
        self.execute(withArguments: arguments) { (rc, output, error) in
            if (rc == RETURN_CODE_SUCCESS) {
                guard let jsonMedia = output,
                      let jsonData = jsonMedia.data(using: .utf8) else { return }
                do {
                    ffprobeOutput = try JSONSerialization.jsonObject(with: jsonData)
                } catch {
                    errorLogData = error.localizedDescription
                    ffprobeError = error
                }
            } else {
                ffprobeError = error
            }
        }
    guard let error = ffprobeError else { return ffprobeOutput as Any }
        throw error
    }
    
}
