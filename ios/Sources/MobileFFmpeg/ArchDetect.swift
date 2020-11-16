//
//  ArchDetect.swift
//  FFmpegKit
//
//  Created by Johnny Vega on 10/19/20.
//

import Foundation
import Cfftools

///
/// This class is used to detect running architecture.
///

@objcMembers public class ArchDetect : NSObject {
    
    ///
    /// Returns running cpu architecture name.
    ///
    /// - Returns running CPU architecture name as String
    ///
    
    @objc public class func getCPUArch() -> String {
        var cpu: String = ""
        var size: Int = 0
        var type: cpu_type_t = 0
        var subtype: cpu_subtype_t = 0
        size = MemoryLayout.size(ofValue: type)
        sysctlbyname("hw.cputype", &type, &size, nil, 0)
        size = MemoryLayout.size(ofValue: subtype)
        sysctlbyname("hw.cpusubtype", &subtype, &size, nil, 0)
        if type == CPU_TYPE_X86_64 {
            cpu.append("x86_64")
        } else if type == CPU_TYPE_X86 {
            cpu.append("x86")
            switch subtype {
                case CPU_SUBTYPE_X86_64_H:
                    cpu.append("_64h")
                case CPU_SUBTYPE_X86_64_ALL:
                    cpu.append("_64all")
                case CPU_SUBTYPE_X86_ARCH1:
                    cpu.append("_arch1")
                default:
                    break
            }
        } else if type == CPU_TYPE_I386 {
            cpu.append("i386")
        } else if type == CPU_TYPE_ARM64 {
            cpu.append("arm64")
            switch subtype {
                case CPU_SUBTYPE_ARM64_V8:
                    cpu.append("v8")
                case CPU_SUBTYPE_ARM64E:
                    cpu.append("e")
                default:
                    break
            }
        } else if type == CPU_TYPE_ARM {
            cpu.append("arm")
            switch subtype {
                case CPU_SUBTYPE_ARM_XSCALE:
                    cpu.append("xscale")
                case CPU_SUBTYPE_ARM_V4T:
                    cpu.append("v4t")
                case CPU_SUBTYPE_ARM_V5TEJ:
                    cpu.append("v5tej")
                case CPU_SUBTYPE_ARM_V6:
                    cpu.append("v6")
                case CPU_SUBTYPE_ARM_V6M:
                    cpu.append("v6m")
                case CPU_SUBTYPE_ARM_V7:
                    cpu.append("v7")
                case CPU_SUBTYPE_ARM_V7EM:
                    cpu.append("v7em")
                case CPU_SUBTYPE_ARM_V7F:
                    cpu.append("v7f")
                case CPU_SUBTYPE_ARM_V7K:
                    cpu.append("v7k")
                case CPU_SUBTYPE_ARM_V7M:
                    cpu.append("v7m")
                case CPU_SUBTYPE_ARM_V7S:
                    cpu.append("v7s")
                case CPU_SUBTYPE_ARM_V8:
                    cpu.append("v8")
            default:
                break
            }
        } else if type == CPU_TYPE_ARM64_32 {
            cpu.append("arm64_32")
            switch subtype {
                case CPU_SUBTYPE_ARM64_32_V8:
                cpu.append("v8")
            default:
                break;
                
            }
        } else {
            cpu.append("\(type)")
        
        }
        return cpu
    }
    
    
    ///
    /// Returns actual architecture name.
    ///
    /// - Returns architecture name as String
    ///
    
    @objc public class func getArch()-> String {
        #if arch(i386)
        return "i386"
        #elseif arch(x86_64)
        return "x86_64"
        #elseif arch(arm)
        return "arm"
        #elseif arch(arm64)
        return "arm64"
        #else
        return "unknown"
        #endif

    }
    

    ///
    /// - Returns whether MobileFFmpeg release is a long term release or not.
    ///
    
    @objc public class var isLTSBuild: Bool {
        return ARCH_ARM == 1
    }
}

