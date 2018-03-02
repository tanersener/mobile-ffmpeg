function runEncodeTest(log, callback) {
  log("Executing encoding test");

  var samplerate = 44100;
  var channels = 2;
  var bitrate = 128;
  var mode = Shine.STEREO;

  var shine = new Shine({
    samplerate: samplerate,
    channels: channels,
    bitrate: bitrate,
    mode: mode
  });

  var started = new Date;
  var duration = 0.0;
  var encoded = [];

  var xhr = new XMLHttpRequest();
  xhr.open("GET", "encode.wav", true);
  xhr.responseType = "arraybuffer";
  xhr.onload = function() {
    var samples = xhr.response.byteLength / (2*channels);
    var data = new Int16Array(xhr.response);
    var buf = convertInterleavedBuffer(data, channels, samples);
    duration += parseFloat(samples) / samplerate;
    encoded.push(shine.encode(buf));
  };

  log("Got WAV file.");
  log("Encoding..");
  xhr.addEventListener("load", function () {
    encoded.push(shine.close());

    var ended = new Date
    var encodingTime = (ended.getTime() - started.getTime())/1000
    log("Done encoding.");
    log("File duration: " + duration.toFixed(2) + " seconds");
    log("Encoding time: " + encodingTime.toFixed(2) + " seconds");
    log("Encoding rate: " + (duration/encodingTime).toFixed(2) + "X");
    callback(new Blob(encoded));
  });

  xhr.send();
}

function convertInterleavedBuffer(buf, channels, samples) {
  var chan, i;
  ret = new Array(channels);
  for (chan=0; chan<channels; chan++) {
    ret[chan] = new Int16Array(samples);
    for (i=0; i<samples; i++) {
      ret[chan][i] = buf[i*channels + chan];
    }
  }
  return ret;
}
