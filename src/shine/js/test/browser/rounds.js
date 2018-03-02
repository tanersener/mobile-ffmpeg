var channels = 2; // test.wav is stereo
var samplerate = 44100; // ditto

var runRoundsTest = function(Shine, log) {
  log("Executing rounds test");

  var nPasses = 50;
  var frameSize = 4096;
  var data = new Array(channels);
  var chan;
  for (chan=0; chan<channels; chan++)
     data[chan] = new Int16Array(frameSize);

  log("Encoding " + nPasses + " buffers of " + frameSize + " samples");
  var started = new Date;

  var shine = new Shine({
    samplerate: samplerate,
    bitrate: 128,
    channels: channels,
    model: Shine.STEREO
  });

  var i;
  for (i=0; i < nPasses; i++)
    shine.encode(data);
  shine.close();

  var ended = new Date;
  var duration = (parseFloat(nPasses*frameSize) / parseFloat(samplerate)).toFixed(2);
  var encodingTime = (ended.getTime() - started.getTime())/1000;
  log("Done encoding");
  log("Total duration: " + duration);
  log("Encoding time: " + encodingTime.toFixed(2));
  log("Encoding rate: " + (duration/encodingTime).toFixed(2) + "X");
};

if (typeof process === "object" && typeof require === "function")
  module.exports = runRoundsTest;
