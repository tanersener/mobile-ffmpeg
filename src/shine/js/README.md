Shine encoder library for Javascript
====================================

This directory contains a build of shine for Javascript using 
[kripken/emscripten](https://github.com/kripken/emscripten) and
located in `dist/libshine.js`

How to use?
-----------

The encoding API should be quite straight forward:

```
shine = new Shine({
  samplerate: 44100,
  bitrate: 128,
  channels: 2,
  mode: Shine.STEREO
});
  
// All the magic happens here
while (..) {
  // data here is an array of channels.
  // Channels must have the same number of samples
  // and both be either Int16Array or Float32Array.
  encoded = shine.encode(data);
  ...
}

// Close and get remaining data.
flushed = shine.close();
...
```

How fast is it?
---------------

You can run the test suite located in `test/`. As of now (26/07/2014), only firefox
and seems to show performances decent enough for real-time encoding while Google Chrome
can encode between 1X and 2X.

Chrome:
```
Executing encoding test
Got WAV file.
Encoding..
Done encoding.
File duration: 5.57 seconds
Encoding time: 5.18 seconds
Encoding rate: 1.08X

Executing rounds test
Encoding 50 buffers of 4096 samples
Done encoding
Total duration: 4.64
Encoding time: 2.73
Encoding rate: 1.70X
```

Firefox:
```
Executing encoding test
Got WAV file.
Encoding..
Done encoding.
File duration: 5.57 seconds
Encoding time: 1.48 seconds
Encoding rate: 3.76X

Executing rounds test
Encoding 50 buffers of 4096 samples
Done encoding
Total duration: 4.64
Encoding time: 0.98
Encoding rate: 4.72X
```

Safari:
```
Executing encoding test
Got WAV file.
Encoding..
Done encoding.
File duration: 5.57 seconds
Encoding time: 11.77 seconds
Encoding rate: 0.47X

Executing rounds test
Encoding 50 buffers of 4096 samples
Done encoding
Total duration: 4.64
Encoding time: 6.93
Encoding rate: 0.67X
```

NodeJS, v0.10.24:
```
Executing encoding test
Got WAV file.
Encoding..
Done encoding.
File duration: 5.57 seconds
Encoding time: 22.57 seconds
Encoding rate: 0.25X

Executing rounds test
Encoding 50 buffers of 4096 samples
Done encoding
Total duration: 4.64
Encoding time: 11.46
Encoding rate: 0.40X
```
