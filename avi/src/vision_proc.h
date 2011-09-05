#ifndef __vision_proc_h
#define __vision_proc_h

#include "vision.h"
#include "highgui.h"
#include "cv.h"

#include <iostream>
#include <libconfig.h++>


class state;

class vision_proc {
public:
	vision_proc(state *sub_state, libconfig::Config *conf);
	~vision_proc();


	virtual void process_image(const void *p) = 0;

protected:

	int fileNum;
	int count;

	state *avi_state;


	char buf[FMT_HEIGHT * FMT_WIDTH * 3];

	inline int clamp(double x);
	inline int clamp(int x);
	int frame_save_yuv(const void *p, const char* filename);
	int frame_save_rgb(const void *p, const char* filename);
	int frame_save_jpeg(const void *p, const char* filename);
	int convert(const void *p, char *buf);
	int convert_to_image(char *buf, IplImage* frame);

	int c_snapshotEn;
	int c_snapshotTime;
	int c_noTracking;
	int c_frameTrimB;

};

#endif
