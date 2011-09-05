#include <stdio.h>
#include "vision_proc.h"
#include "vision.h"
#include "state.h"



vision_proc::vision_proc(state *sub_state, libconfig::Config *conf):
	fileNum(0),
	count(0),
	avi_state(sub_state)
{
	conf->lookupValue("vision.snapshot_enable", c_snapshotEn);
	conf->lookupValue("vision.snapshot_time", c_snapshotTime);
	conf->lookupValue("vision.no_tracking", c_noTracking);
	conf->lookupValue("vision.frame_trim_bottom", c_frameTrimB);
}
vision_proc::~vision_proc() {

}



inline int vision_proc::clamp(int x) {
	return x<0 ? 0 : (x>255 ? 255 : x);
}

inline int vision_proc::clamp(double x) {
	if (x <= 0.0)
		return 0;
	if (x >= 255.0)
		return 255;
	return (int)(x+.5);
}

int vision_proc::frame_save_yuv(const void *p, const char* filename ) {

	int packed_value, i;
	i = packed_value = 0;
	FILE* fp = fopen(filename, "w" );

	// Write PNM header
	fprintf( fp, "P6\n" );
	fprintf( fp, "# YUV422 frame -> RGB \n" );
	fprintf( fp, "%d %d\n", FMT_WIDTH, FMT_HEIGHT);

	// case V4L2_PIX_FMT_YUYV:
	int Y0, Y1, Cb, Cr; /* gamma pre-corrected input [0;255] */
	int ER0,ER1,EG0,EG1,EB0,EB1; /* output [0;255] */
	double r0,r1,g0,g1,b0,b1; /* temporaries */
	double y0,y1, pb, pr;

	// Max val
	fprintf( fp, "255\n" );
	//fprintf( stderr, "frame_save(): YUYV file type!\n" );

	while(i < (FMT_WIDTH * FMT_HEIGHT/2)){

		packed_value = *((int*)p+i);

		Y0 = (char)(packed_value & 0xFF);
		Cb = (char)((packed_value >> 8) & 0xFF);
		Y1 = (char)((packed_value >> 16) & 0xFF);
		Cr = (char)((packed_value >> 24) & 0xFF);

		// Strip sign values after shift (i.e. unsigned shift)
		Y0 = Y0 & 0xFF;
		Cb = Cb & 0xFF;
		Y1 = Y1 & 0xFF;
		Cr = Cr & 0xFF;

		//fprintf( fp, "Value:%x Y0:%x Cb:%x Y1:%x Cr:%x ",packed_value,Y0,Cb,Y1,Cr);

		y0 = (255 / 219.0) * (Y0 - 16);
		y1 = (255 / 219.0) * (Y1 - 16);
		pb = (255 / 224.0) * (Cb - 128);
		pr = (255 / 224.0) * (Cr - 128);

		// Generate first pixel
		r0 = 1.0 * y0 + 0 * pb + 1.402 * pr;
		g0 = 1.0 * y0 - 0.344 * pb - 0.714 * pr;
		b0 = 1.0 * y0 + 1.772 * pb + 0 * pr;

		// Generate next pixel - must reuse pb & pr as 4:2:2
		r1 = 1.0 * y1 + 0 * pb + 1.402 * pr;
		g1 = 1.0 * y1 - 0.344 * pb - 0.714 * pr;
		b1 = 1.0 * y1 + 1.772 * pb + 0 * pr;

		ER0 = clamp (r0);
		ER1 = clamp (r1);
		EG0 = clamp (g0);
		EG1 = clamp (g1);
		EB0 = clamp (b0);
		EB1 = clamp (b1);

		fprintf( fp, "%c%c%c%c%c%c",ER0,EG0,EB0,ER1,EG1,EB1); // Output two pixels

		i++;
	}


	fprintf( stderr, "frame saved\n" );
	fclose( fp );
	return 0;
}

int vision_proc::frame_save_rgb(const void *p, const char* filename ) {

	int packed_value, i;
	char * bp = (char *)p;
	i = packed_value = 0;
	FILE* fp = fopen(filename, "w" );

	// Write PNM header
	fprintf( fp, "P6\n" );
	fprintf( fp, "# YUV422 frame -> RGB \n" );
	fprintf( fp, "%d %d\n", FMT_WIDTH, FMT_HEIGHT);

	fprintf( fp, "255\n" );

	while(i < (FMT_WIDTH * FMT_HEIGHT)){
		fprintf( fp, "%c%c%c",*(bp), *(bp+1), *(bp+2));
		bp += 3;
		i++;
	}


	fprintf( stderr, "rgb frame saved\n" );
	fclose( fp );
	return 0;
}
int vision_proc::frame_save_jpeg(const void *p, const char* filename){
    CvSize size = cvSize(320,240);
    IplImage* frame = cvCreateImage(size, IPL_DEPTH_8U, 3);
    int k = 0;
    for(int k = 0; k < frame->height * frame->width; k++){
        char* loc = buf + k*3;
        frame->imageData[k*3] = loc[2];
        frame->imageData[k*3 + 1] = loc[1];
        frame->imageData[k*3 + 2] = loc[0];
	}
	cvSaveImage(filename, frame);
	std::cout << "jpeg frame saved\n";
	return 0;




}

//Use fixed point (read: fast) math to convert YUYV colorspace into RGB.
int vision_proc::convert(const void *p, char *buf) {

	int packed_value, i;
	char * bp;
	i = packed_value = 0;

	// case V4L2_PIX_FMT_YUYV:
	int Y0, Y1, Cb, Cr; /* gamma pre-corrected input [0;255] */
	int r0,r1,g0,g1,b0,b1; /* temporaries */
	int y0,y1, pb, pr;

	while(i < (FMT_WIDTH * FMT_HEIGHT/2)){

		packed_value = *((int*)p+i);
		bp = (buf+i*6);

		Y0 = (char)(packed_value & 0xFF);
		Cb = (char)((packed_value >> 8) & 0xFF);
		Y1 = (char)((packed_value >> 16) & 0xFF);
		Cr = (char)((packed_value >> 24) & 0xFF);

		// Strip sign values after shift (i.e. unsigned shift)
		Y0 = Y0 & 0xFF;
		Cb = Cb & 0xFF;
		Y1 = Y1 & 0xFF;
		Cr = Cr & 0xFF;

		y0 = 75 * (Y0 - 16);
		y1 = 75 * (Y1 - 16);
		pb = Cb - 128;
		pr = Cr - 128;

		// Generate first pixel
		r0 = y0 + 102 * pr;
		g0 = y0 - 25 * pb - 52 * pr;
		b0 = y0 + 129 * pb;

		// Generate next pixel - must reuse pb & pr as 4:2:2
		r1 = y1 + 102 * pr;
		g1 = y1 - 25 * pb - 52 * pr;
		b1 = y1 + 129 * pb;

		*(bp)   = clamp(r0 >> 6);
		*(bp+1) = clamp(g0 >> 6);
		*(bp+2) = clamp(b0 >> 6);
		*(bp+3) = clamp(r1 >> 6);
		*(bp+4) = clamp(g1 >> 6);
		*(bp+5) = clamp(b1 >> 6);

		i++;
	}

	return 0;
}


int vision_proc::convert_to_image(char *buf, IplImage* frame) {
    //before implementing this method, you have to initialize a blank IplImage like the following:
   // CvSize size = cvSize(320,240);
    //IplImage* frame = cvCreateImage(size, IPL_DEPTH_8U, 3);
	int k = 0;
    //int blarg = 0;

	for(int k = 0; k < frame->height * frame->width; k++){
        char* loc = buf + k*3;
        frame->imageData[k*3] = loc[2];
        frame->imageData[k*3 + 1] = loc[1];
        frame->imageData[k*3 + 2] = loc[0];
	}

	return 0;


}
