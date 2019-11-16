Kvazaar
=======
An open-source HEVC encoder licensed under LGPLv2.1

Join channel #kvazaar_hevc in Freenode IRC network to contact us.

Kvazaar is still under development. Speed and RD-quality will continue to improve.

http://ultravideo.cs.tut.fi/#encoder for more information.

- Linux/Mac [![Build Status](https://travis-ci.org/ultravideo/kvazaar.svg?branch=master)](https://travis-ci.org/ultravideo/kvazaar)
- Windows [![Build status](https://ci.appveyor.com/api/projects/status/88sg1h25lp0k71pu?svg=true)](https://ci.appveyor.com/project/Ultravideo/kvazaar)

## Table of Contents

- [Using Kvazaar](#using-kvazaar)
  - [Example:](#example)
  - [Parameters](#parameters)
  - [LP-GOP syntax](#lp-gop-syntax)
- [Presets](#presets)
- [Kvazaar library](#kvazaar-library)
- [Compiling Kvazaar](#compiling-kvazaar)
  - [Required libraries](#required-libraries)
  - [Autotools](#autotools)
  - [OS X](#os-x)
  - [Visual Studio](#visual-studio)
  - [Docker](#docker)
  - [Visualization (Windows only)](#visualization-windows-only)
- [Paper](#paper)
- [Contributing to Kvazaar](#contributing-to-kvazaar)
  - [Code documentation](#code-documentation)
  - [For version control we try to follow these conventions:](#for-version-control-we-try-to-follow-these-conventions)
  - [Testing](#testing)
  - [Unit tests](#unit-tests)
  - [Code style](#code-style)

## Using Kvazaar

### Example:

    kvazaar --input BQMall_832x480_60.yuv --output out.hevc

The mandatory parameters are input and output. If the resolution of the input file is not in the filename, or when pipe is used, the input resolution must also be given: ```--input-res=1920x1080```.

The default input format is 8-bit yuv420p for 8-bit and yuv420p10le for 10-bit. Input format and bitdepth can be selected with ```--input-format``` and ```--input-bitdepth```.

Speed and compression quality can be selected with ```--preset```, or by setting the options manually.

### Parameters

[comment]: # (BEGIN KVAZAAR HELP MESSAGE)
```
Usage:
kvazaar -i <input> --input-res <width>x<height> -o <output>

Required:
  -i, --input <filename>     : Input file
      --input-res <res>      : Input resolution [auto]
                                   - auto: Detect from file name.
                                   - <int>x<int>: width times height
  -o, --output <filename>    : Output file

Presets:
      --preset <preset>      : Set options to a preset [medium]
                                   - ultrafast, superfast, veryfast, faster,
                                     fast, medium, slow, slower, veryslow
                                     placebo

Input:
  -n, --frames <integer>     : Number of frames to code [all]
      --seek <integer>       : First frame to code [0]
      --input-fps <num>[/<denom>] : Frame rate of the input video [25]
      --source-scan-type <string> : Source scan type [progressive]
                                   - progressive: Progressive scan
                                   - tff: Top field first
                                   - bff: Bottom field first
      --input-format <string> : P420 or P400 [P420]
      --input-bitdepth <int> : 8-16 [8]
      --loop-input           : Re-read input file forever.

Options:
      --help                 : Print this help message and exit.
      --version              : Print version information and exit.
      --(no-)aud             : Use access unit delimiters. [disabled]
      --debug <filename>     : Output internal reconstruction.
      --(no-)cpuid           : Enable runtime CPU optimizations. [enabled]
      --hash <string>        : Decoded picture hash [checksum]
                                   - none: 0 bytes
                                   - checksum: 18 bytes
                                   - md5: 56 bytes
      --(no-)psnr            : Calculate PSNR for frames. [enabled]
      --(no-)info            : Add encoder info SEI. [enabled]
      --crypto <string>      : Selective encryption. Crypto support must be
                               enabled at compile-time. Can be 'on' or 'off' or
                               a list of features separated with a '+'. [off]
                                   - on: Enable all encryption features.
                                   - off: Disable selective encryption.
                                   - mvs: Motion vector magnitudes.
                                   - mv_signs: Motion vector signs.
                                   - trans_coeffs: Coefficient magnitudes.
                                   - trans_coeff_signs: Coefficient signs.
                                   - intra_pred_modes: Intra prediction modes.
      --key <string>         : Encryption key [16,213,27,56,255,127,242,112,
                                               97,126,197,204,25,59,38,30]

Video structure:
  -q, --qp <integer>         : Quantization parameter [22]
  -p, --period <integer>     : Period of intra pictures [64]
                                   - 0: Only first picture is intra.
                                   - 1: All pictures are intra.
                                   - N: Every Nth picture is intra.
      --vps-period <integer> : How often the video parameter set is re-sent [0]
                                   - 0: Only send VPS with the first frame.
                                   - N: Send VPS with every Nth intra frame.
  -r, --ref <integer>        : Number of reference frames, in range 1..15 [4]
      --gop <string>         : GOP structure [8]
                                   - 0: Disabled
                                   - 8: B-frame pyramid of length 8
                                   - lp-<string>: Low-delay P-frame GOP
                                     (e.g. lp-g8d4t2, see README)
      --(no-)open-gop        : Use open GOP configuration. [enabled]
      --cqmfile <filename>   : Read custom quantization matrices from a file.
      --scaling-list <string>: Set scaling list mode. [off]
                                   - off: Disable scaling lists.
                                   - custom: use custom list (with --cqmfile).
                                   - default: Use default lists.
      --bitrate <integer>    : Target bitrate [0]
                                   - 0: Disable rate control.
                                   - N: Target N bits per second.
      --(no-)lossless        : Use lossless coding. [disabled]
      --mv-constraint <string> : Constrain movement vectors. [none]
                                   - none: No constraint
                                   - frametile: Constrain within the tile.
                                   - frametilemargin: Constrain even more.
      --roi <filename>       : Use a delta QP map for region of interest.
                               Reads an array of delta QP values from a text
                               file. The file format is: width and height of
                               the QP delta map followed by width*height delta
                               QP values in raster order. The map can be of any
                               size and will be scaled to the video size.
      --set-qp-in-cu         : Set QP at CU level keeping pic_init_qp_minus26.
                               in PPS and slice_qp_delta in slize header zero.
      --(no-)erp-aqp         : Use adaptive QP for 360 degree video with
                               equirectangular projection. [disabled]
      --level <number>       : Use the given HEVC level in the output and give
                               an error if level limits are exceeded. [6.2]
                                   - 1, 2, 2.1, 3, 3.1, 4, 4.1, 5, 5.1, 5.2, 6,
                                     6.1, 6.2
      --force-level <number> : Same as --level but warnings instead of errors.
      --high-tier            : Used with --level. Use high tier bitrate limits
                               instead of the main tier limits during encoding.
                               High tier requires level 4 or higher.

Compression tools:
      --(no-)deblock <beta:tc> : Deblocking filter. [0:0]
                                   - beta: Between -6 and 6
                                   - tc: Between -6 and 6
      --sao <string>         : Sample Adaptive Offset [full]
                                   - off: SAO disabled
                                   - band: Band offset only
                                   - edge: Edge offset only
                                   - full: Full SAO
      --(no-)rdoq            : Rate-distortion optimized quantization [enabled]
      --(no-)rdoq-skip       : Skip RDOQ for 4x4 blocks. [disabled]
      --(no-)signhide        : Sign hiding [disabled]
      --(no-)smp             : Symmetric motion partition [disabled]
      --(no-)amp             : Asymmetric motion partition [disabled]
      --rd <integer>         : Intra mode search complexity [0]
                                   - 0: Skip intra if inter is good enough.
                                   - 1: Rough intra mode search with SATD.
                                   - 2: Refine intra mode search with SSE.
                                   - 3: Try all intra modes and enable intra
                                        chroma mode search.
      --(no-)mv-rdo          : Rate-distortion optimized motion vector costs
                               [disabled]
      --(no-)full-intra-search : Try all intra modes during rough search.
                               [disabled]
      --(no-)transform-skip  : Try transform skip [disabled]
      --me <string>          : Integer motion estimation algorithm [hexbs]
                                   - hexbs: Hexagon Based Search
                                   - tz:    Test Zone Search
                                   - full:  Full Search
                                   - full8, full16, full32, full64
                                   - dia:   Diamond Search
      --me-steps <integer>   : Motion estimation search step limit. Only
                               affects 'hexbs' and 'dia'. [-1]
      --subme <integer>      : Fractional pixel motion estimation level [4]
                                   - 0: Integer motion estimation only
                                   - 1: + 1/2-pixel horizontal and vertical
                                   - 2: + 1/2-pixel diagonal
                                   - 3: + 1/4-pixel horizontal and vertical
                                   - 4: + 1/4-pixel diagonal
      --pu-depth-inter <int>-<int> : Inter prediction units sizes [0-3]
                                   - 0, 1, 2, 3: from 64x64 to 8x8
      --pu-depth-intra <int>-<int> : Intra prediction units sizes [1-4]
                                   - 0, 1, 2, 3, 4: from 64x64 to 4x4
      --tr-depth-intra <int> : Transform split depth for intra blocks [0]
      --(no-)bipred          : Bi-prediction [disabled]
      --cu-split-termination <string> : CU split search termination [zero]
                                   - off: Don't terminate early.
                                   - zero: Terminate when residual is zero.
      --me-early-termination <string> : Motion estimation termination [on]
                                   - off: Don't terminate early.
                                   - on: Terminate early.
                                   - sensitive: Terminate even earlier.
      --fast-residual-cost <int> : Skip CABAC cost for residual coefficients
                                   when QP is below the limit. [0]
      --(no-)intra-rdo-et    : Check intra modes in rdo stage only until
                               a zero coefficient CU is found. [disabled]
      --(no-)early-skip      : Try to find skip cu from merge candidates.
                               Perform no further search if skip is found.
                               For rd=0..1: Try the first candidate.
                               For rd=2.. : Try the best candidate based
                                            on luma satd cost. [enabled]
      --max-merge <integer>  : Maximum number of merge candidates, 1..5 [5]
      --(no-)implicit-rdpcm  : Implicit residual DPCM. Currently only supported
                               with lossless coding. [disabled]
      --(no-)tmvp            : Temporal motion vector prediction [enabled]

Parallel processing:
      --threads <integer>    : Number of threads to use [auto]
                                   - 0: Process everything with main thread.
                                   - N: Use N threads for encoding.
                                   - auto: Select automatically.
      --owf <integer>        : Frame-level parallelism [auto]
                                   - N: Process N+1 frames at a time.
                                   - auto: Select automatically.
      --(no-)wpp             : Wavefront parallel processing. [enabled]
                               Enabling tiles automatically disables WPP.
                               To enable WPP with tiles, re-enable it after
                               enabling tiles. Enabling wpp with tiles is,
                               however, an experimental feature since it is
                               not supported in any HEVC profile.
      --tiles <int>x<int>    : Split picture into width x height uniform tiles.
      --tiles-width-split <string>|u<int> :
                                   - <string>: A comma-separated list of tile
                                               column pixel coordinates.
                                   - u<int>: Number of tile columns of uniform
                                             width.
      --tiles-height-split <string>|u<int> :
                                   - <string>: A comma-separated list of tile row
                                               column pixel coordinates.
                                   - u<int>: Number of tile rows of uniform
                                             height.
      --slices <string>      : Control how slices are used.
                                   - tiles: Put tiles in independent slices.
                                   - wpp: Put rows in dependent slices.
                                   - tiles+wpp: Do both.

Video Usability Information:
      --sar <width:height>   : Specify sample aspect ratio
      --overscan <string>    : Specify crop overscan setting [undef]
                                   - undef, show, crop
      --videoformat <string> : Specify video format [undef]
                                   - undef, component, pal, ntsc, secam, mac
      --range <string>       : Specify color range [tv]
                                   - tv, pc
      --colorprim <string>   : Specify color primaries [undef]
                                   - undef, bt709, bt470m, bt470bg,
                                     smpte170m, smpte240m, film, bt2020
      --transfer <string>    : Specify transfer characteristics [undef]
                                   - undef, bt709, bt470m, bt470bg,
                                     smpte170m, smpte240m, linear, log100,
                                     log316, iec61966-2-4, bt1361e,
                                     iec61966-2-1, bt2020-10, bt2020-12
      --colormatrix <string> : Specify color matrix setting [undef]
                                   - undef, bt709, fcc, bt470bg, smpte170m,
                                     smpte240m, GBR, YCgCo, bt2020nc, bt2020c
      --chromaloc <integer>  : Specify chroma sample location (0 to 5) [0]

Deprecated parameters: (might be removed at some point)
  -w, --width <integer>       : Use --input-res.
  -h, --height <integer>      : Use --input-res.
```
[comment]: # (END KVAZAAR HELP MESSAGE)


### LP-GOP syntax
The LP-GOP syntax is "lp-g(num)d(num)t(num)", where
- g = GOP length.
- d = Number of GOP layers.
- t = How many references to skip for temporal scaling, where 4 means only
  every fourth picture needs to be decoded.

```
QP
+4  o o o o  
+3   o   o    o o o o 
+2     o       o o o    ooooooo
+1 o       o o       o o       o ooooooooo
   g8d4t1    g8d3t1    g8d2t1    g8d1t1
```

## Presets
The names of the presets are the same as with x264: ultrafast,
superfast, veryfast, faster, fast, medium, slow, slower, veryslow and
placebo. The effects of the presets are listed in the following table,
where the names have been abbreviated to fit the layout in GitHub.

|                      | 0-uf  | 1-sf  | 2-vf  | 3-fr  | 4-f   | 5-m   | 6-s   | 7-sr  | 8-vs  | 9-p   |
| -------------------- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| rd                   | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 2     | 2     | 2     |
| pu-depth-intra       | 2-3   | 2-3   | 2-3   | 2-3   | 1-3   | 1-4   | 1-4   | 1-4   | 1-4   | 1-4   |
| pu-depth-inter       | 2-3   | 2-3   | 1-3   | 1-3   | 1-3   | 0-3   | 0-3   | 0-3   | 0-3   | 0-3   |
| me                   | hexbs | hexbs | hexbs | hexbs | hexbs | hexbs | hexbs | hexbs | hexbs | tz    |
| gop                  | g4d4t1| g4d4t1| g4d4t1| g4d4t1| g4d4t1| 8     | 8     | 8     | 8     | 8     |
| ref                  | 1     | 1     | 1     | 1     | 2     | 4     | 4     | 4     | 4     | 4     |
| bipred               | 0     | 0     | 0     | 0     | 0     | 0     | 1     | 1     | 1     | 1     |
| deblock              | 1     | 1     | 1     | 1     | 1     | 1     | 1     | 1     | 1     | 1     |
| signhide             | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 1     | 1     | 1     |
| subme                | 2     | 2     | 2     | 4     | 4     | 4     | 4     | 4     | 4     | 4     |
| sao                  | off   | full  | full  | full  | full  | full  | full  | full  | full  | full  |
| rdoq                 | 0     | 0     | 0     | 0     | 0     | 1     | 1     | 1     | 1     | 1     |
| rdoq-skip            | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 0     |
| transform-skip       | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 1     |
| mv-rdo               | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 1     |
| full-intra-search    | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 0     |
| smp                  | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 1     | 1     |
| amp                  | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 1     |
| cu-split-termination | zero  | zero  | zero  | zero  | zero  | zero  | zero  | zero  | zero  | off   |
| me-early-termination | sens. | sens. | sens. | sens. | sens. | on    | on    | off   | off   | off   |
| intra-rdo-et         | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 0     | 0     |
| early-skip           | 1     | 1     | 1     | 1     | 1     | 1     | 1     | 1     | 1     | 1     |
| fast-residual-cost   | 28    | 28    | 28    | 0     | 0     | 0     | 0     | 0     | 0     | 0     |
| max-merge            | 5     | 5     | 5     | 5     | 5     | 5     | 5     | 5     | 5     | 5     |


## Kvazaar library
See [kvazaar.h](src/kvazaar.h) for the library API and its
documentation.

When using the static Kvazaar library on Windows, macro `KVZ_STATIC_LIB`
must be defined. On other platforms it's not strictly required.

The needed linker and compiler flags can be obtained with pkg-config.


## Compiling Kvazaar
If you have trouble regarding compiling the source code, please make an
[issue](https://github.com/ultravideo/kvazaar/issues) about in Github.
Others might encounter the same problem and there is probably much to
improve in the build process. We want to make this as simple as
possible.


### Autotools
Depending on the platform, some additional tools are required for compiling Kvazaar with autotools.
For Ubuntu, the required packages are `automake autoconf libtool m4 build-essential yasm`. Yasm is
optional, but some of the optimization will not be compiled in if it's missing.

Run the following commands to compile and install Kvazaar.

    ./autogen.sh
    ./configure
    make
    sudo make install

See `./configure --help` for more options.


### OS X
- Install Homebrew
- run ```brew install automake libtool yasm```
- Refer to Autotools instructions


### Visual Studio
- At least VisualStudio 2015 is required.
- Project files can be found under build/.
- Requires external [vsyasm.exe](http://yasm.tortall.net/Download.html)
  in %PATH%


### Docker
This project includes a [Dockerfile](./Dockerfile), which enables building for Docker. Kvazaar is also available in the Docker Hub [`ultravideo/kvazaar`](https://hub.docker.com/r/ultravideo/kvazaar/)
Build using Docker: `docker build -t kvazaar .`
Example usage: `docker run -i -a STDIN -a STDOUT kvazaar -i - --input-res=320x240 -o - < testfile_320x240.yuv > out.265`
For other examples, see [Dockerfile](./Dockerfile)


### Visualization (Windows only)
Compiling `kvazaar_cli` project in the `visualizer` branch results in a Kvazaar executable with visualization enabled.

Additional Requirements: [`SDL2`](https://www.libsdl.org/download-2.0.php), [`SDL2-ttf`](https://www.libsdl.org/projects/SDL_ttf/).

Directory `visualizer_extras` has to be added into the same directory level as the kvazaar project directory. Inside should be directories `include` and `lib` found from the development library zip packages.

`SDL2.dll`, `SDL2_ttf.dll`, `libfreetype-6.dll`, and `zlib1.dll` should be placed in the working directory (i.e. the folder the `kvazaar.exe` is in after compiling the `kvazaar_cli` project/solution) when running the visualizer. The required `.dll` can be found in the aforementioned `lib`-folder (`lib\x64`).

Note: The solution should be compiled on the x64 platform in visual studio.

Optional font file `arial.ttf` is to be placed in the working directory, if block info tool is used.

## Paper

Please cite [this paper](https://dl.acm.org/citation.cfm?doid=2964284.2973796) for Kvazaar:

```M. Viitanen, A. Koivula, A. Lemmetti, A. Ylä-Outinen, J. Vanne, and T. D. Hämäläinen, “Kvazaar: open-source HEVC/H.265 encoder,” in Proc. ACM Int. Conf. Multimedia, Amsterdam, The Netherlands, Oct. 2016.```

Or in BibTex:

```
@inproceedings{Kvazaar2016,
 author = {Viitanen, Marko and Koivula, Ari and Lemmetti, Ari and Yl\"{a}-Outinen, Arttu and Vanne, Jarno and H\"{a}m\"{a}l\"{a}inen, Timo D.},
 title = {Kvazaar: Open-Source HEVC/H.265 Encoder},
 booktitle = {Proceedings of the 24th ACM International Conference on Multimedia},
 year = {2016},
 isbn = {978-1-4503-3603-1},
 location = {Amsterdam, The Netherlands},
 url = {http://doi.acm.org/10.1145/2964284.2973796},
}
```

## Contributing to Kvazaar
We are happy to look at pull requests in Github. There is still lots of work to be done.


### Code documentation
You can generate Doxygen documentation pages by running the command
"doxygen docs.doxy". Here is a rough sketch of the module structure:
![Kvazaar module hierarchy](https://github.com/ultravideo/kvazaar/blob/master/doc/kvazaar_module_hierarchy.png)


### For version control we try to follow these conventions:
- Master branch always produces a working bitstream (can be decoded with
  HM).
- Commits for new features and major changes/fixes put to a sensibly
  named feature branch first and later merged to the master branch.
- Always merge the feature branch to the master branch, not the other
  way around, with fast-forwarding disabled if necessary.
- Every commit should at least compile.


### Testing
- Main automatic way of testing is with Travis CI. Commits, branches
  and pull requests are tested automatically.
  - Uninitialized variables and such are checked with Valgrind.
  - Bitstream validity is checked with HM.
  - Compilation is checked on GCC and Clang on Linux, and Clang on OSX.
- Windows msys2 and msvc builds are checked automatically on Appveyor.
- If your changes change the bitstream, decode with HM to check that
  it doesn't throw checksum errors or asserts.
- If your changes shouldn't alter the bitstream, check that they don't.
- Automatic compression quality testing is in the works.


### Unit tests
- There are some unit tests located in the tests directory. We would
  like to have more.
- The Visual Studio project links the unit tests against the actual .lib
  file used by the encoder. There is no Makefile as of yet.
- The unit tests use "greatest" unit testing framework. It is included
  as a submodule, but getting it requires the following commands to be
  run in the root directory of kvazaar:

        git submodule init
        git submodule update

- On Linux, run ```make test```.


### Code style
We try to follow the following conventions:
- C99 without features not supported by Visual Studio 2015 (VLAs).
 - // comments allowed and encouraged.
- Follow overall conventions already established in the code.
- Indent by 2 spaces. (no tabs)
- { on the same line for control logic and on the next line for functions
- Reference and deference next to the variable name.
- Variable names in lowered characters with words divided by underscore.
- Maximum line length 79 characters when possible.
- Functions only used inside the module shouldn't be defined in the
  module header. They can be defined in the beginning of the .c file if
  necessary.
- Symbols defined in headers prefixed with kvz_ or KVZ_.
- Includes in alphabetic order.
