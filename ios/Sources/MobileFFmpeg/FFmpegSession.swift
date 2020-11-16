//
//  FFmpegSession.swift
//  FFmpegKit
//
//  Created by Johnny Vega on 10/19/20.
//

import Foundation
import Cfftools

struct IntegerSequence: IteratorProtocol {
    
    static var `default` = IntegerSequence()
    
    var current: Int = {
        var random = Int.random(in: 1...10000)
        return random
    }()

    mutating func next() -> Int? {
        defer {
            current += 1
        }

        return current
    }
    
    mutating func next() -> Int {
        return self.next()!
    }
}

let DEFAULT_EXECUTION_ID = 0
let MOBILE_FFMPEG_VERSION = "4.4"

///
/// Main class for `FFmpeg` operations.
///

@available(iOS 13.0, macOS 10.15, *)
@objcMembers public class FFmpegSession: NSObject, ObservableObject, Identifiable {
    
    @objc public private(set) var arguments = Array<String>()

    @objc public let executionId: Int
    
    ///
    /// The operation session return code.  You can observe this property using Key-value observing,
    /// or use a Combine Subscriber to receive published events.
    ///
    
    @Published @objc dynamic public var returnCode: Int = -1
    
    ///
    /// Session operation log level.  You should assign the respective session log level before starting the session execution.
    ///

    @objc public var logLevel: Int32 = AV_LOG_INFO
    
    ///
    /// Last operation Log output.  You can observe this property using Key-value observing,
    /// or use a Combine Subscriber to receive published events.
    ///
    
    @Published @objc dynamic public var lastLogOutput: String = ""
    ///
    /// Last operation statisticst.  You can observe this property using Key-value observing,
    /// or use a Combine Subscriber to receive published events.
    ///
    
    @Published @objc dynamic public var statistics: Statistics!
    
    private var observeLogContext: Int = 0
    private var observeStatisticsContext: Int = 1
        
    // KVO of logOutput and Statistics data generated during the FFmpeg command execution
    
    public override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey : Any]?, context: UnsafeMutableRawPointer?) {
        if context == &observeLogContext,
            let output = change?[.newKey] as? String,
            !output.isEmpty {
            if output.last != "\n" {
                self.lastLogOutput.append(output)
            } else if output != "\n" {
                self.lastLogOutput = output
            }
        } else if context == &observeStatisticsContext,
            let statistics = change?[.newKey] as? Statistics,
            statistics.executionId == self.executionId {
            self.statistics = statistics
        }
    }
    
    
    /// Initialize a FFmpegSession with an Array of command arguments.
    ///
    /// - Parameter arguments: FFmpeg command options/arguments as Array of Strings
    ///
    
    @objc public init(withArguments arguments: [String]) {
        self.executionId = IntegerSequence.default.next()
        super.init()
        self.arguments = [LIB_NAME] + arguments
        guard let index = arguments.firstIndex(of: "-i"),
              index < arguments.endIndex - 1 else { return }
        let url = arguments[index + 1]
        MobileFFprobe.execute(withCommand: "-loglevel 0 -hide_banner -print_format default=nk=1:nw=1 -select_streams v -show_entries stream=duration -i \"\(url)\"") { rc, duration, error  in
            if rc == RETURN_CODE_SUCCESS {
                guard let duration = duration?.split(separator: "\n").first,
                let timeDuration = Double(duration) else {
                    self.statistics = Statistics(withExecutionId: self.executionId)
                    return
                }
                self.statistics = Statistics(withExecutionId: self.executionId, andDuration: Int64(timeDuration * 1000))
            }
        }
    }
    
    
    /// Initializa a FFmpegSession with a command arguments string. `Space` character is used to split command into arguments.
    ///
    /// - Parameter arguments: FFmpeg command.
    ///
    
    @objc public convenience init(withCommand command: String) {
        let args =  MobileFFmpegConfig.parseArguments(command)
        self.init(withArguments: args)
    }
    
    //
    // Function to configure the AVLogging system
    //
    private func setupLogging() {
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
                let logData = NSString(format: format, arguments: args)
                guard let execId = Thread.current.name else { return }
                Thread.current.threadDictionary.setValue(logData, forKey: "logData-\(execId)")
            }
            return
        })
        
        set_report_callback({ (frameNumber: Int32, fps: Float, quality: Float, size: Int64, time: Int32, bitrate: Double, speed: Double) -> Void in
            guard let execIdS = Thread.current.name,
                  let execId = Int(execIdS) else { return }
            let statistics = Statistics(forExecId: Int(execId), videoFrameNumber: Int(frameNumber), fps: fps, quality: quality, size: Int(size), time: TimeInterval(time), bitrate: bitrate, speed: speed)
            if let actualStatistics = Thread.current.threadDictionary.value(forKey: "statistics-\(execIdS)") as? Statistics {
                actualStatistics.update(statistics)
                Thread.current.threadDictionary.setValue(actualStatistics, forKey: "statistics-\(execIdS)")
            } else {
                Thread.current.threadDictionary.setValue(statistics, forKey: "statistics-\(execIdS)")
            }
        })
    }
    
    
    ///
    /// Synchronously executes FFmpeg.
    ///
    /// - Parameter completionHandler: The completion handler to call when the ffmpeg execution has finished.  This completion handler takes the following parameters:
    /// - Parameter returnCode: zero on successful execution, 255 on user cancel and non-zero on error
    /// - Parameter logOutput: Last log output generated during the session.  Nil in case of error.
    /// - Parameter error: if execution finished in error, this object will provide the reason of failure
    ///
    
    @objc public func execute(withCompletionHandler completionHandler: (_ returnCode: Int, _ logOutput: String?, _ error: MobileFFmpegError?)->Void) {
        var returnCode: Int32 = -1
            self.statistics.startTime = Date()
            Thread.current.threadDictionary.addObserver(self, forKeyPath: "logData", options: [.new], context: &observeLogContext)
            Thread.current.threadDictionary.addObserver(self, forKeyPath: "statistics", options: [.new], context: &observeStatisticsContext)
            self.setupLogging()
            // SETS DEFAULT LOG LEVEL BEFORE STARTING A NEW EXECUTION
            av_log_set_level(self.logLevel)
  
            withArrayOfCStrings(self.arguments) { args  in
                var vargs = args
                vargs.withUnsafeMutableBufferPointer { pointer in
                    returnCode = ffmpeg_execute(Int32(self.arguments.count), pointer.baseAddress)
                }
            }
        self.returnCode = Int(returnCode)
        Thread.current.threadDictionary.removeObserver(self, forKeyPath: "logData", context: &observeLogContext)
        Thread.current.threadDictionary.removeObserver(self, forKeyPath: "statistics", context: &observeStatisticsContext)
        if self.returnCode == RETURN_CODE_SUCCESS {
            completionHandler(self.returnCode, self.lastLogOutput, nil)
        } else {
            errorLogData = self.lastLogOutput
            completionHandler(self.returnCode, nil, MobileFFmpegError.FFmpegError)
        }
    }
    
    
    ///
    /// Asynchronously executes FFmpeg.  Because the execution is performed asynchronously, this method returns immediately — you can Observe or a Subscribe the `statistics` property to monitor the session progress.  To determine when the session has finished, you can observe or subscribe to the `returnCode` session property. Use the `lastLogOutput` property to monitor the session output.
    ///
    
    @objc public func executeAsync() {
        var thread: Thread!
        DispatchQueue.global(qos: .userInitiated).async {
            thread = Thread(block: {
                self.statistics.startTime = Date()
                self.setupLogging()
                // SETS DEFAULT LOG LEVEL BEFORE STARTING A NEW EXECUTION
                av_log_set_level(self.logLevel)
                
                withArrayOfCStrings(self.arguments) { args  in
                    var vargs = args
                    vargs.withUnsafeMutableBufferPointer { pointer in
                        self.returnCode = Int(ffmpeg_execute(Int32(self.arguments.count), pointer.baseAddress))
                        thread.threadDictionary.removeObserver(self, forKeyPath: "logData-\(self.executionId)", context: &self.observeLogContext)
                        thread.threadDictionary.removeObserver(self, forKeyPath: "statistics-\(self.executionId)", context: &self.observeStatisticsContext)
                    }
                }
            })
            thread.name = String(self.executionId)
            thread.threadDictionary.setValue(self.statistics, forKey: "statistics-\(self.executionId)")
            thread.threadDictionary.addObserver(self, forKeyPath: "logData-\(self.executionId)", options: [.new], context: &self.observeLogContext)
            thread.threadDictionary.addObserver(self, forKeyPath: "statistics-\(self.executionId)", options:[.new], context: &self.observeStatisticsContext)
            thread.start()
        }
        
    }
  
    
    ///
    /// Cancels an ongoing operation.
    ///
    /// This function does not wait for termination to complete and returns immediately.
    ///
    
    @objc public func cancel() {
        cancel_operation(DEFAULT_EXECUTION_ID)
    }
    
    ///
    /// Combines arguments into a string.
    ///
    /// - Parameter arguments: arguments
    /// - Returns: string containing all arguments
    ///
    
    @objc public class func argumentsToString(_ arguments: [String]) -> String {
        guard var string: String = arguments.first else { return "" }
        for argument in arguments.dropFirst() {
            string.append(argument)
        }
        return string
    }
    
    
    /// Log level Name
    
    internal func logLevelToString(level: Int32)-> String {
        switch (level) {
            case AV_LOG_STDERR:
                return "STDERR"
            case AV_LOG_TRACE:
                return "TRACE"
            case AV_LOG_DEBUG:
                return "DEBUG"
            case AV_LOG_VERBOSE:
                return "VERBOSE"
            case AV_LOG_INFO:
                return "INFO"
            case AV_LOG_WARNING:
                return "WARNING"
            case AV_LOG_ERROR:
                return "ERROR"
            case AV_LOG_FATAL:
                return "FATAL"
            case AV_LOG_PANIC:
                return "PANIC"
            case AV_LOG_QUIET:
                return ""
            default:
                return ""
        }
    }
    
    
    // MARK: - Equatable Protocol
    
    public static func == (lhs: FFmpegSession, rhs: FFmpegSession) -> Bool {
        return lhs.executionId == rhs.executionId
    }

    #if os(macOS)
    public override func isEqual(to object: Any?) -> Bool {
        guard let object = object as? FFmpegSession else { return false }
        return object.executionId == self.executionId
    }
    #else
    public override func isEqual(_ object: Any?) -> Bool {
        guard let object = object as? FFmpegSession else { return false }
        return object.executionId == self.executionId
    }
    #endif
}

