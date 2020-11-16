//
//  Statistics.swift
//  FFmpegKit
//
//  Created by Johnny Vega on 10/19/20.
//

import Foundation


///
/// Statistics for running FFmpeg sessions.
///
@available(iOS 13.0, macOS 10.15, *)
@objcMembers public class Statistics : NSObject, ObservableObject {

    internal var executionId: Int = -1
    @Published public var videoFrameNumber: Int
    @Published public var videoFps: Float
    @Published public var videoQuality: Float
    @Published public var size: Int
    @Published public var time: TimeInterval
    @Published public var bitrate: Double
    @Published public var speed: Double
    @Published public var progress: Progress
    internal var startTime: Date?
    
    ///
    /// Common initializer
    ///
    public init(withExecutionId id:Int) {
            executionId = id
            videoFrameNumber = 0
            videoFps = 0
            videoQuality = 0
            size = 0
            time = 0
            bitrate = 0
            speed = 0
        progress = Progress()
        super.init()
    }
    
    public init(withExecutionId id: Int, andDuration duration: Int64) {
        self.executionId = id
        videoFrameNumber = 0
        videoFps = 0
        videoQuality = 0
        size = 0
        time = 0
        bitrate = 0
        speed = 0
        progress = Progress(totalUnitCount: duration)
        super.init()
    }

    
    ///
    /// Initialize a Statistics object with the respective characteristic values
    ///
    
    public init(forExecId id: Int, videoFrameNumber newVideoFrameNumber: Int, fps newVideoFps: Float, quality newVideoQuality: Float, size newSize: Int, time newTime: TimeInterval, bitrate newBitrate: Double, speed newSpeed: Double) {
        executionId = id
        videoFrameNumber = newVideoFrameNumber
        videoFps = newVideoFps
        videoQuality = newVideoQuality
        size = newSize
        time = newTime
        bitrate = newBitrate
        speed = newSpeed
        progress = Progress()
        super.init()
    }
    
    
    ///
    /// Update the receiver Statistic object with the passed new statistic object.
    /// - Parameter statistics: Statistic object with new values to update the receiver properties.
    ///
    
    public func update(_ statistics: Statistics) {
        guard statistics.executionId == self.executionId else { return }
            if statistics.videoFrameNumber > 0 {
                videoFrameNumber = statistics.videoFrameNumber
            }
            if statistics.videoFps > 0 {
                videoFps = statistics.videoFps
            }
            if statistics.videoQuality > 0 {
                videoQuality = statistics.videoQuality
            }
            if statistics.size > 0 {
                size = statistics.size
            }
            if statistics.time > 0 {
                time = statistics.time
            }
            if statistics.bitrate > 0 {
                bitrate = statistics.bitrate
            }
            if statistics.speed > 0 {
                speed = statistics.speed
            }
            self.progress.completedUnitCount = Int64(statistics.time)
            guard let startTime = self.startTime else { return }
            let remaining = self.progress.totalUnitCount - self.progress.completedUnitCount
            let estimatedRemining = NSNumber(value: remaining * Int64(Date().timeIntervalSince(startTime)) / self.progress.completedUnitCount)
            self.progress.setUserInfoObject(estimatedRemining, forKey: .estimatedTimeRemainingKey)
    }
  
}

