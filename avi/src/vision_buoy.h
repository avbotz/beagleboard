#ifndef __vision_buoy_h
#define __vision_buoy_h

#include "vision_proc.h"

class vision_buoy : public vision_proc {

public:
	
	vision_buoy(state *sub_state, libconfig::Config *conf);
	~vision_buoy();
	void process_image(const void *p);

	
private:
	
	int c_rMinPixelIntensity;
	int c_gMinPixelIntensity;
	int c_yMinPixelIntensity;
	int c_minTotalIntensity;
	

};

#endif