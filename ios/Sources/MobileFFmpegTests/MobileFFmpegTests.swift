//
//  FFmpegKitTests.swift
//  FFmpegKitTests
//
//  Created by Johnny Vega on 10/20/20.
//

import XCTest
@testable import MobileFFmpeg
import VideoToolbox
import Combine

class MobileFFmpegTests: XCTestCase {
    
    var cancallables: Array<AnyCancellable> = []
    
    override func setUpWithError() throws {
        // Put setup code here. This method is called before the invocation of each test method in the class.
    }
    
    func test() {
    }
    
    override func tearDownWithError() throws {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
    }
    
    func testSync() {
        // Example of a syncronous execution.
        
        var hwaccel = ["-hwaccel",
                       "videotoolbox"]
        if !VTIsHardwareDecodeSupported(kCMVideoCodecType_HEVC) {
            hwaccel = []
        }
        
        let args = hwaccel + [ "-i",
                               "<Movie>",
                               "-c:a",
                               "copy",
                               "-c:v",
                               "libx265",
                               "-movflags",
                               "faststart",
                               "-f",
                               "mp4",
                               "-y",
                               "<Movie Output>"
        ]
        let ffmpeg = FFmpegSession(withArguments: args)
        ffmpeg.execute { rc, logOutput, error in
            if rc == RETURN_CODE_SUCCESS {
                print(logOutput ?? "")
            } else {
                print(error?.localizedDescription ?? "")
            }
            
        }
    }
    
    
    func testExampleAsync() throws {
        // This is an example of two concurrent encoding sessions.  Each of them displaying the respective encoding progress.
        
        let sema = DispatchSemaphore(value: 0)
        var hwaccel = ["-hwaccel",
                       "videotoolbox"]
        if !VTIsHardwareDecodeSupported(kCMVideoCodecType_HEVC) {
            hwaccel = []
        }
        
        let args = [ "-i",
                     "<Movie 1>",
                     "-c:a",
                     "copy",
                     "-c:v",
                     "libx264",
                     "-movflags",
                     "faststart",
                     "-f",
                     "mp4",
                     "-y",
                     "<Movie 1 converted>"
        ]
        
        let args2 = hwaccel + [ "-i",
                                "<Movie 2>",
                                "-c:a",
                                "aac",
                                "-c:v",
                                "libx265",
                                "-movflags",
                                "faststart",
                                "-f",
                                "mp4",
                                "-y",
                                "<Movie 2 converted>"
        ]
        
        let ffmpeg = FFmpegSession(withArguments: args)
        let ffmpeg1 = FFmpegSession(withArguments: args2)
        
        ffmpeg.executeAsync()
        
        ffmpeg1.executeAsync()
        
        DispatchQueue.global().async {
            let formatter = DateComponentsFormatter()
            formatter.allowedUnits = [.hour, .minute,.second]
            let cancellable1 = ffmpeg.$statistics.sink { stat in
                guard let statistics = stat,
                      statistics.executionId == ffmpeg.executionId  else { return }
                let formatNumber = NumberFormatter()
                formatNumber.numberStyle = .percent
                formatNumber.maximumFractionDigits = 2
                let percent = formatNumber.string(for: statistics.progress.fractionCompleted)
                print("Progress for Movie1: \(percent ?? " ") \(statistics.progress.localizedAdditionalDescription ?? " ")")
            }
            cancellable1.store(in: &self.cancallables)
            
            let cancellable2 = ffmpeg.$returnCode.sink { code in
                if code == 0 {
                    if ffmpeg1.returnCode >= 0 {
                        sema.signal()
                    }
                } else if code > 0 {
                    print("Return for Movie1: \(ffmpeg.lastLogOutput)")
                    if ffmpeg1.returnCode >= 0 {
                        sema.signal()
                    }
                }
            }
            cancellable2.store(in: &self.cancallables)
        }
        
        DispatchQueue.global(qos: .userInitiated).async {
            let cancellable3 = ffmpeg1.$statistics.sink { stat in
                guard let statistics = stat,
                      statistics.executionId == ffmpeg1.executionId else { return }
                let formatNumber = NumberFormatter()
                formatNumber.numberStyle = .percent
                formatNumber.maximumFractionDigits = 2
                let percent = formatNumber.string(for: statistics.progress.fractionCompleted)
                print("Progress for Movie2: \(percent ?? " ") \(statistics.progress.localizedAdditionalDescription ?? " ")")
            }
            cancellable3.store(in: &self.cancallables)
            
            let cancellable4 = ffmpeg1.$returnCode.sink { code in
                if code == 0 {
                    if ffmpeg.returnCode >= 0 {
                        sema.signal()
                    }
                } else if code > 0 {
                    print("Return for Movie2: \(ffmpeg1.lastLogOutput)")
                    if ffmpeg.returnCode >= 0 {
                        sema.signal()
                    }
                }
            }
            cancellable4.store(in: &self.cancallables)
        }
        sema.wait()
    }
    
    func testFFProbe() throws {
        // Example of how to obtain Media Information
        do {
            let mediaInfo = try MobileFFprobe.getMediaInformation(forURL: "<Video to obtain Media Information>")
            dump(mediaInfo)
        } catch {
            print(error.localizedDescription)
        }
        
    }
    
    func testFFProbeCommand() throws {
        // Example of how obtain a Dictionary object with the result of a ffprobe command
        do {
            let mediaInfo = try MobileFFprobe.getMediaInformationFromCommand(" -loglevel 0  -hide_banner  -print_format json -show_format -show_streams -i \"<Media File>\"")
            dump(mediaInfo)
        } catch {
            print(error.localizedDescription)
        }
    }
    
}
