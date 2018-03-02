Shine    = require "../../dist/libshine.js"
{Reader} = require "wav"
fs       = require "fs"

console.log ""
console.log "Executing encoding test"

bitrate = 128
str     = fs.createReadStream "../lib/encode.wav"
fd      = fs.openSync "./encode.mp3", "w"
reader  = new Reader

str.pipe reader

write = (encoded) ->
  return unless encoded.length > 0
  buf = new Buffer encoded
  fs.writeSync fd, buf, 0, buf.length

reader.on "format", (format) ->
  console.log "Got WAV file."

  shine = new Shine
    bitrate:    bitrate
    samplerate: format.sampleRate
    channels:   format.channels

  console.log "Encoding.."
  started    = new Date
  duration   = 0.0
  samplerate = parseFloat format.sampleRate

  reader.on "data", (buf) ->
    samples = buf.length/(2*format.channels)

    duration += parseFloat(samples) / samplerate

    write shine.encode(convertInterleavedBuffer(buf, format.channels, samples))

  reader.on "end", ->
    write shine.close()

    ended = new Date
    encodingTime = (ended.getTime() - started.getTime())/1000
    console.log "Done encoding."
    console.log "File duration: #{duration.toFixed(2)} seconds"
    console.log "Encoding time: #{encodingTime.toFixed(2)} seconds"
    console.log "Encoding rate: #{(duration/encodingTime).toFixed(2)}X"
    process.exit 0
    
convertInterleavedBuffer = (buf, channels, samples) ->
  ret = new Array channels
  for chan in [0..channels-1]
    ret[chan] = new Int16Array samples
    for i in [0..samples-1]
      ret[chan][i] = buf.readInt16LE 2*(i*channels + chan)

  ret
