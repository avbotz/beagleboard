#ifndef __vision_path_h
#define __vision_path_h

#include "vision_proc.h"
#include "math.h"
#include "highgui.h"
#include "cv.h"
#include <iostream>

#include <vector>
class _line{

     //= (pt2.y - pt1.y)/(pt2.x - pt1.x);
    public:
    CvPoint pt1, pt2;
    double slope;
    _line(CvPoint firstpoint, CvPoint secondpoint, double riserun) : pt1(firstpoint), pt2(secondpoint), slope(riserun) { }
};

class vision_path : public vision_proc {

public:

	vision_path(state *sub_state, libconfig::Config *conf);
	~vision_path();
	void process_image(const void *p);



private:


    std::vector<_line> endpoints;
    double meanSlope;
    std::vector<_line> temp;
    double stdDev;
    int revolutions;
	bool proceed;
	int c_minPixelIntensity;
	int c_minTotalIntensity;
};


#endif
