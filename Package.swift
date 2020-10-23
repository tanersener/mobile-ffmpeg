// swift-tools-version:5.3
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "Mobile-FFmpeg-Full-GPL-4.4-IOS-XCFramework",
    products: [
        .library(
            name: "Mobile-FFmpeg-Full-GPL-4.4-IOS-XCFramework",
            targets: ["Mobile-FFmpeg-Full-GPL-4.4-IOS-XCFramework"]),
    ],
    dependencies: [
        // Dependencies declare other packages that this package depends on.
        // .package(url: /* package url */, from: "1.0.0"),
    ],
    targets: [
        // Targets are the basic building blocks of a package. A target can define a module or a test suite.
        // Targets can depend on other targets in this package, and on products in packages this package depends on.
        .target(
            name: "Mobile-FFmpeg-Full-GPL-4.4-IOS-XCFramework",
            dependencies: []),
        .testTarget(
            name: "Mobile-FFmpeg-Full-GPL-4.4-IOS-XCFrameworkTests",
            dependencies: ["Mobile-FFmpeg-Full-GPL-4.4-IOS-XCFramework"]),
    ]
)
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/expat.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/fontconfig.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/freetype.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/fribidi.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/giflib.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/gmp.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/gnutls.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/jpeg.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/kvazaar.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/lame.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libaom.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libass.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libavcodec.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libavdevice.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libavfilter.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libavformat.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libavutil.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libhogweed.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/liblibc.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libnettle.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libogg.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libopencore-ammb.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libpng.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libsndfile.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libswresample.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libswscale.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libtheora.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libtheoradec.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libtheoraenc.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libvidstab.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libvorbis.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libvorbisenc.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libvpx.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libwebp.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/libwebpdemux.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/xml2.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/mobileffmpeg.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/opus.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/shine.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/snappy.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/soxr.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/speex.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/tiff.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/twolame.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/vo-amrwbenc.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/wavpack.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/x264.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/x265.xcframework")
// .binaryTarget(name: "Crypto", path: "full-gpl-4.4-ios-xcframework/xvidcore.xcframework")
