# Chromaprint

Chromaprint is an audio fingerprint library developed for the [AcoustID][acoustid] project. It's designed to identify near-identical audio
and the fingerprints it generates are as compact as possible to achieve that. It's not a general purpose audio fingerprinting solution.
It trades precision and robustness for search performance. The target use cases are full audio file identifcation,
duplicate audio file detection and long audio stream monitoring.

[acoustid]: https://acoustid.org/

## Building

The most common way to build Chromaprint is like this:

    $ cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TOOLS=ON .
    $ make
    $ sudo make install

This will build Chromaprint as a shared library and also include the `fpcalc`
utility (which is used by MusicBrainz Picard, for example). For this to work,
you will need to have the FFmpeg libraries installed.

See below for other options.

### FFT Library

Chromaprint can use multiple FFT libraries -- [FFmpeg][ffmpeg], [FFTW3][fftw], [KissFFT][kissfft] or
[vDSP][vdsp] (macOS).

FFmpeg is preferred on all systems except for macOS, where you should use
the standard vDSP framework. These are the fastest options.

FFTW3 can be also used, but this library is released under the GPL
license, which makes also the resulting Chromaprint binary GPL licensed.

KissFFT is the slowest option, but it's distributed with a permissive license
and it's very easy to build on platforms that do not have packaged
versions of FFmpeg or FFTW3. We ship a copy of KissFFT, so if
the build system is unable to find another FFT library it will use
that as a fallback.

You can explicitly set which library to use with the `FFT_LIB` option.
For example:

    $ cmake -DFFT_LIB=kissfft .

[ffmpeg]: https://www.ffmpeg.org/
[fftw]: http://www.fftw.org/
[kissfft]: https://sourceforge.net/projects/kissfft/
[vdsp]: https://developer.apple.com/reference/accelerate/1652565-vdsp

### FFmpeg

FFmpeg is as a FFT library and also for audio decoding and resampling in `fpcalc`.
If you have FFmpeg installed in a non-standard location, you can use the `FFMPEG_ROOT` option to specify where:

    $ cmake -DFFMPEG_ROOT=/path/to/local/ffmpeg/install .

While we try to make sure things work also with libav, FFmpeg is preferred.

## API Documentation

You can use Doxygen to generate a HTML version of the API documentation:

    $ make docs
    $ $BROWSER docs/html/index.html

## Unit Tests

The test suite can be built and run using the following commands:

    $ cmake -DBUILD_TESTS=ON .
    $ make check

In order to build the test suite, you will need the sources of the [Google Test][gtest] library.

[gtest]: https://github.com/google/googletest

## Related Projects

Bindings, wrappers and reimplementations in other languages:

 * [Python](https://github.com/beetbox/pyacoustid)
 * [Rust](https://github.com/jameshurst/rust-chromaprint)
 * [Ruby](https://github.com/TMXCredit/chromaprint)
 * [Perl](https://github.com/jonathanstowe/Audio-Fingerprint-Chromaprint)
 * [JavaScript](https://github.com/parshap/node-fpcalc)
 * [JavaScript](https://github.com/bjjb/chromaprint.js) (reimplementation)
 * [Go](https://github.com/go-fingerprint/gochroma)
 * [C#](https://github.com/wo80/AcoustID.NET) (reimplementation)
 * [C#](https://github.com/protyposis/Aurio/tree/master/Aurio/Aurio/Matching/Chromaprint) (reimplementation)
 * [Pascal](https://github.com/CMCHTPC/ChromaPrint) (reimplementation)
 * [Scala/JVM](https://github.com/mgdigital/Chromaprint.scala) (reimplementation)
 * [C++/CLI](https://github.com/CyberSinh/Luminescence.Audio)
 * [Vala](https://github.com/GNOME/vala-extra-vapis/blob/master/libchromaprint.vapi)

Integrations:

 * [FFmpeg](https://www.ffmpeg.org/ffmpeg-formats.html#chromaprint-1)
 * [GStreamer](http://cgit.freedesktop.org/gstreamer/gst-plugins-bad/tree/ext/chromaprint)

If you know about a project that is not listed here, but should be, please let me know.

## Standing on the Shoulders of Giants

I've learned a lot while working on this project, which would not be possible
without having information from past research. I've read many papers, but the
concrete ideas implemented in this library are based on the following papers:

 * Yan Ke, Derek Hoiem, Rahul Sukthankar. Computer Vision for Music
   Identification, Proceedings of Computer Vision and Pattern Recognition, 2005.
   http://www.cs.cmu.edu/~yke/musicretrieval/

 * Frank Kurth, Meinard Müller. Efficient Index-Based Audio Matching, 2008.
   http://dx.doi.org/10.1109/TASL.2007.911552

 * Dalwon Jang, Chang D. Yoo, Sunil Lee, Sungwoong Kim, Ton Kalker.
   Pairwise Boosted Audio Fingerprint, 2009.
   http://dx.doi.org/10.1109/TIFS.2009.2034452

