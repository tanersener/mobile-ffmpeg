Quick hack at a transcoder tool from VP3 to Theora

I actually built the avi2vp3 tool with codeWarrior, but it should compile under
VC as well. I have included a source avi file and the converted .vp3 output.
Output is a file with some header info matching YUVMPEG, and for each frame: 

FRAME header block matching YUV2MPEG
long (Intel aligned) keyframeflag describing in frame is a keyframe
long (Intel aligned) fsize storing frame size in bytes
bytes[fsize] with binary frame data

The transcode tool is a modification of the current encoder. PUt it into the
win32/experimental subdirectory, and the paths should be correct. It produces
an apparently valid theora stream, but outputs garbage data. The code is packing
the binary frame data in a way that SHOULD work at least imo, but I am probably
missing some initialization issue (or vp3 is not transcodable to theora).

