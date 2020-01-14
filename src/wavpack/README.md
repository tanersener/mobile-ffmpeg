<img src="http://www.rarewares.org/wavpack/logos/wavpacklogo.png" width="250"></img>

Hybrid Lossless Wavefile Compressor

Copyright (c) 1998 - 2019 David Bryant.

All Rights Reserved.

Distributed under the [BSD Software License](https://github.com/dbry/WavPack/blob/master/license.txt).

---

This [repository](https://github.com/dbry/WavPack) contains all of the source code required to build the WavPack library (_libwavpack_), and any associated command-line programs.

Additional references:

* [Official website](http://wavpack.com/)
* [Binaries](http://wavpack.com/downloads.html#binaries)
* [Other sources](http://wavpack.com/downloads.html#sources)
* [Documentation](http://wavpack.com/downloads.html#documentation)
* [Test suite](http://www.rarewares.org/wavpack/test_suite.zip)
* [Logos](http://wavpack.com/downloads.html#logos)

---

## Build Status

| Branch         | Status                                                                                                            |
|----------------|-------------------------------------------------------------------------------------------------------------------|
| `master`       | [![Build Status](https://travis-ci.org/dbry/WavPack.svg?branch=master)](https://travis-ci.org/dbry/WavPack)       |

Branches [actively built](https://travis-ci.org/dbry/WavPack/branches) by TravisCI.

---

## Building

### Windows

There are solution and project files for Visual Studio 2008, and additional source code to build the [CoolEdit/Audition](https://github.com/dbry/WavPack/tree/master/audition) plugin and the [Winamp](https://github.com/dbry/WavPack/tree/master/winamp) plugin.

The CoolEdit/Audition plugin provides a good example for using the library to both read and write WavPack files, and the Winamp plugin makes extensive use of APEv2 tag reading and writing.

Both 32-bit and 64-bit platforms are provided.

Visual Studio 2008 does not support projects with x64 assembly very well. I have provided a copy of the edited `masm.rules` file that works for me, but I can't provide support if your build does not work. Please make a copy of your `masm.rules` file first.

On my system it lives here: `C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\VCProjectDefaults`

### Linux

To build everything on Linux, type:

1. `./configure`
   * `--disable-asm`
   * `--enable-man`
   * `--enable-rpath`
   * `--enable-tests`
   * `--disable-apps`
   * `--disable-dsd`
   * `--enable-legacy`
2. `make`
   * Optionally, `make install`, to install into `/usr/local/bin`

If you are using the code directly from Git (rather than a distribution) then you will need to do a ./autogen.sh instead of the configure step. If assembly optimizations are available for your processor they will be automatically enabled, but if there is a problem with them then use the `--disable-asm` option to revert to pure C.

For Clang-based build systems (Darwin, FreeBSD, etc.), Clang version 3.5 or higher is required.

If you get a WARNING about unexpected _libwavpack_ version when you run the command-line programs, you might try using `--enable-rpath` to hardcode the library location in the executables, or simply force static linking with `--disable-shared`.

There is now a CLI program to do a full suite of stress tests for _libwavpack_, and this is particularly useful for packagers to make sure that the assembly language optimizations are working correctly on various platforms. It is built with the configure option `--enable-tests` and requires Pthreads (it worked out-of-the-box on all the platforms I tried it on). There are lots of options, but the default test suite (consisting of 192 tests) is executed with `wvtest --default`. There is also a seeking test. On Windows a third-party Pthreads library is required, so I am not including this in the build for now.

---

## Assembly

Assembly language optimizations are provided for x86 and x86-64 (AMD64) processors (encoding and decoding) and ARMv7 (decoding only).

The x86 assembly code includes a runtime check for MMX capability, so it will work on legacy i386 processors.

## Documentation

There are four documentation files contained in the distribution:

| File                         | Description                                                                                                                                                   |
|------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------|
| [doc/wavpack_doc.html](https://github.com/dbry/WavPack/blob/master/doc/wavpack_doc.html)         | Contains user-targeted documentation for the command-line programs.                                                                                            |
| [doc/WavPack5PortingGuide.pdf](https://github.com/dbry/WavPack/blob/master/doc/WavPack5PortingGuide.pdf) | This document is targeted at developers who are migrating to WavPack 5, and it provides a short description of the major improvements and how to utilize them. |
| [doc/WavPack5LibraryDoc.pdf](https://github.com/dbry/WavPack/blob/master/doc/WavPack5LibraryDoc.pdf)   | Contains a detailed description of the API provided by WavPack library appropriate for reading and writing WavPack files and manipulating APEv2 tags.              |
| [doc/WavPack5FileFormat.pdf](https://github.com/dbry/WavPack/blob/master/doc/WavPack5FileFormat.pdf)   | Contains a description of the WavPack file format, including details needed for parsing WavPack, blocks, and interpreting the block header and flags.            |

There is also a description of the WavPack algorithms in the forth edition of David Salomon's book "Data Compression: The Complete Reference". This section can be found here: www.wavpack.com/WavPack.pdf

## Portability

This code is designed to be easy to port to other platforms.

It is endian-agnostic and usually uses callbacks for I/O, although there's a convenience function for reading files that accepts filename strings and automatically handles correction files.

On Windows, there is now an option to select UTF-8 instead of ANSI.

To maintain compatibility on various platforms, the following conventions are used:
* `char` must be 8-bits (`signed` or `unsigned`).
* `short` must be 16-bits.
* `int` and `long` must be at least 32-bits.

## Design

The code's modules are organized in such a way that if major chunks of the functionality are not referenced (for example, creating WavPack files) then link-time dependency resolution should provide optimum binary sizes.

However, some functionality could not be easily excluded in this way and so there are additional macros that may be used to further reduce the size of the binary. Note that these must be defined for all modules:

| Macros          | Description                                                                                                |
|-----------------|------------------------------------------------------------------------------------------------------------|
| `NO_SEEKING`    | To not allow seeking to a specific sample index (for applications that always read entire files).          |
| `NO_TAGS`       | To not read specified fields from ID3v1 and APEv2 tags, and not create or edit APEv2 tags.                 |
| `ENABLE_LEGACY` | Include support for Wavpack files from before version 4.0. This was eliminated by default with WavPack 5. |
| `ENABLE_DSD`    | Include support for DSD audio. New for WavPack 5 and the default, but obviously not universally required. |

Note that this has been tested on many platforms.

## Tiny Decoder

There are alternate versions of this library available specifically designed for resource limited CPUs, and hardware encoding and decoding.

There is the _Tiny Decoder_ library which works with less than 32k of code and less than 4k of data, and has assembly language optimizations for the ARM and Freescale ColdFire CPUs.

The _Tiny Decoder_ is also designed for embedded use and handles the pure lossless, lossy, and hybrid lossless modes.

Neither of these versions use any memory allocation functions, nor do they require floating-point arithmetic support.

---

Questions or comments should be directed to david@wavpack.com.

You may also find David on GitHub as [dbry](https://github.com/dbry).
