#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
typedef long DWORD;
#endif

/*extremely crude app to dump vp3 frames from an avi file*/
/*filenames are hardcoded*/

#include "avilib.h"

int main(int argc, const char **argv)
{

    FILE * f = fopen("outfile.vp3", "wb");
    
    char * buffer;
    int olength;
    int length;
    avi_t *avifile;
    int chunksize;    
    int frame;
    int frames;
    int keyframegap = 0;
    int maxkeyframegap = 0;

    DWORD initialticks;
    
    int framew = 0;
    int frameh = 0;
    double framerate = 0.0f;
    double fps_numerator, fps_denominator;

    avifile = AVI_open_input_file("vp31.avi", 1);
    frames = AVI_video_frames(avifile);
    framew = AVI_video_width(avifile);
    frameh = AVI_video_height(avifile);
    framerate = AVI_frame_rate(avifile);
    chunksize = AVI_max_video_chunk(avifile);
    /* avilib only reports the max video chunk size if the file has an
       idx table. We fall back to an arbitrary limit otherwise. Better
       would be just to handle the chunks dynamically */
    if (chunksize <= 0) chunksize = 131072;
    buffer = malloc(chunksize);
    printf("Frames(%d) Video(%dx%d) %3.2f fps\n",frames,framew, frameh,framerate);
    printf("Video Compressor: %s", AVI_video_compressor(avifile));
    fps_denominator = 1000000.0F;
    fps_numerator = framerate * fps_denominator;
    
    sprintf(buffer,"AVI2VP31R W%d H%d F%.0f:%.0f Ip A0:0\n", framew, frameh, fps_numerator, fps_denominator);
    fwrite(buffer, strlen(buffer), 1, f);
    
    for (frame = 0; frame < frames;) {
	int keyframe;
	olength = length;
	length = AVI_frame_size(avifile, frame++);
	if( !length ) {
	    length = olength;
	}
	AVI_read_frame(avifile, (char *) buffer, &keyframe);
	fwrite("FRAME\n", 6, 1, f);
	fwrite(&length, sizeof(int), 1, f);
	fwrite(&keyframe, sizeof(int), 1, f);
	printf("Frame size(%d) IsKeyframe(%d)\n", length, keyframe);
	fwrite(buffer, 1, length, f);
	if (!keyframe){
	  keyframegap++;
	} else {
	  if (keyframegap>maxkeyframegap) maxkeyframegap=keyframegap;	
	  keyframegap = 0;
	}
	}

	fclose(f);
	printf("Max keyframegap (%d)\n", maxkeyframegap);
	
	free(buffer);

    exit(0);
}
