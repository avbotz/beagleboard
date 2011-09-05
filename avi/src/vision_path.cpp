#include "vision.h"
#include "state.h"
#include "vision_path.h"
#include <stdio.h>

vision_path::vision_path(state *sub_state, libconfig::Config *conf) :
	vision_proc(sub_state, conf),
	revolutions(0),
	proceed(false)
{
    conf->lookupValue("path.min_pixel_intensity", c_minPixelIntensity);
    conf->lookupValue("path.min_total_intensity", c_minTotalIntensity);
}
vision_path::~vision_path() {

}


void vision_path::process_image(const void *p) {

	count++;
	
	if (avi_state->getProperty(state::cam_mode) == state::cm_done) {
		return;
	}

	if (!(count % 10)) {
//		printf(".");
		fflush (stdout);
	}
	convert(p, buf);


	int cogx(0), cogy(0), tot_int(0);
	CvPoint mid;
	unsigned int i;
	//std::vector<int> pixelValues;

	IplImage* frame = cvCreateImage(cvSize(320,240), IPL_DEPTH_8U, 3);
	convert_to_image(buf, frame);
	//IplImage* clone = cvCloneImage(frame);
    IplImage* frame2 = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
    //IplImage* cannyFrame = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);


	if (c_snapshotEn) {
		if (count > c_snapshotTime) {
				count=0;

			char file[20];
			sprintf(file, "im/pa%d.jpg", fileNum++);
			printf("%s\n",file);
			cvSaveImage(file, frame);
		}
	}

    mid = cvPoint(frame->width / 2, frame->height / 2);
	
	struct _rgb // declared here only for illustrative reasons
    {
        unsigned char b,g,r; // must be in reverse order, because of the IplImage buffer format

        _rgb(int r, int g, int b) : b(b), g(g), r(r) { }
    };

	_rgb *ptmp2 = (_rgb *)frame->imageData;
	int rmax(0), gmax(0), bmax(0);
	int rmin(255), gmin(255), bmin(255);

	for (i = 0; i < FMT_HEIGHT * FMT_WIDTH; i++) {
		char* bp = buf+i*3;
		if (bp[0] < rmin) rmin = bp[0];
		if (bp[1] < gmin) gmin = bp[1];
		if (bp[2] < bmin) bmin = bp[2];
		if (bp[0] > rmax) rmax = bp[0];
		if (bp[1] > gmax) gmax = bp[1];
		if (bp[2] > bmax) bmax = bp[2];
	}
//	printf("%d %d %d %d %d %d", rmin, rmax, gmin, gmax, bmin, bmax);

	if (rmax-rmin == 0 || gmax-gmin == 0 || bmax-bmin == 0) return;

	for (i = 0; i < (unsigned int)frame->height * frame->width; i++) {

        _rgb *currentPixel = ptmp2 + i;
        int r = ((int)currentPixel->r - rmin) * 255 / (rmax - rmin);
        int g = ((int)currentPixel->g - gmin) * 255 / (gmax - gmin);
        int b = ((int)currentPixel->b - bmin) * 255 / (bmax - bmin);

        int val = 2*r - g - b;
        if (val > 255){
            val = 255;
        }

        if (val < c_minPixelIntensity){
            val = 0;
        }

        *currentPixel = _rgb(val, val,val);
        cogx+= val * (i%frame->width);
		cogy+= val * (i/frame->width);
		tot_int+= val;


	}

	

	double ang = 0;
	double cX = cogx / (double)tot_int;
    double cY = cogy / (double)tot_int;
	if(avi_state->getProperty(state::cam_mode) == state::cm_search){
        if(tot_int > c_minTotalIntensity){
            avi_state->setProperty(state::cam_mode, state::cm_track);
			std::cout << "PATH TRACK\n";
        }
	}

    if(avi_state->getProperty(state::cam_mode) == state::cm_track){
        if(tot_int < c_minTotalIntensity){
            avi_state->setProperty(state::cam_mode, state::cm_search);
			std::cout << "PATH SEARCH\n";
			proceed = false;
            return;
        } else {
			if (!proceed) {
				CvPoint midred = cvPoint((int)cX, (int)cY);
				int mid_x = frame->width / 2;
				int mid_y = frame->height / 2;
				double ydist = cY-mid_y;
				double xdist = cX-mid_x;
				bool close_enough_x = (xdist < 20 && xdist > -20);
				bool close_enough_y = (ydist < 30 && ydist > -30);
				
				int newH = avi_state->getProperty(state::cur_heading);
				
				if (xdist > 20) newH += 10;
				if (xdist < -20) newH -= 10;
				if (xdist > 50) newH += 10;
				if (xdist < -50) newH -= 10;
				if (newH > 360) newH -= 360;
				if (newH < 0) newH += 360;
				avi_state->setProperty(state::des_heading, newH);
				std::cout << (xdist) << " " << ydist << std::endl;
				if (close_enough_x && close_enough_y){
					proceed = true;
				} else if (!close_enough_y && ydist > 0) {
					avi_state->setProperty(state::des_power, 90);
				} else if (!close_enough_y && ydist < 0) {
					avi_state->setProperty(state::des_power, 110);
				} else {
					avi_state->setProperty(state::des_power, 100);
				}
			} else {
				avi_state->setProperty(state::des_power, 100);

				CvMemStorage* storage = cvCreateMemStorage(0);
				CvSeq* lines = 0;
				cvCvtColor(frame, frame2, CV_BGR2GRAY);
				cvCanny(frame2, frame2, 40, 100);
				lines = cvHoughLines2( frame2, storage, CV_HOUGH_STANDARD, 1, CV_PI/180, 80, 0, 0 );
				for( int j = 0; j < MIN(lines->total,100); j++ ){
					float* line = (float*)cvGetSeqElem(lines,j);
					float rho = line[0];
					float theta = line[1];
					CvPoint pt1, pt2;
					double a = cos(theta), b = sin(theta);
					double x0 = a*rho, y0 = b*rho;
					pt1.x = cvRound(x0 + 1000*(-b));
					pt1.y = cvRound(y0 + 1000*(a));
					pt2.x = cvRound(x0 - 1000*(-b));
					pt2.y = cvRound(y0 - 1000*(a));
					double ydiff = pt2.y - pt1.y;
					//std::cout << "rise is: " << ydiff << " \n";
					double xdiff = pt2.x - pt1.x;
					//std::cout << "run is: " << xdiff << " \n";
					double slope = ydiff/xdiff;
					//std::cout << "line slope is: " << slope << " \n";
					//cvLine(clone, pt1, pt2, cvScalar(0, 255, 0), 3);
					_line templine = _line(pt1, pt2, slope);
					endpoints.push_back(templine);
					cvLine(frame, templine.pt1, templine.pt2,cvScalar(0,0,255),3);
					//cout << "number of lines " << (int)endpoints.size() << " \n";
					//printf("number of lines: %d \n ", endpoints.size());

				} //adding lines to endpoints
				if (c_snapshotEn) {
					//if (count > c_snapshotTime) {
					//	count=0;
						
						char file[20];
						sprintf(file, "im/pa%dp.jpg", fileNum++);
						printf("%s\n",file);
						cvSaveImage(file, frame);
					//}
				}
				if(lines->total > 0)
					revolutions++;

				if(revolutions == 1){
					//tell vision_path to stop calling method
					meanSlope = 0;
					revolutions = 0;
					double sum = 0;
					for(i = 0; i < endpoints.size(); i++){
						_line line1 = endpoints[i];
					    //cout << "line slope is: " << line1.slope << " \n";
						meanSlope += line1.slope;
					    //cout << "pre-mean slope is: " << meanSlope << " \n";

					}
					//cout << "pre-mean slope is: " << meanSlope << " \n";
					meanSlope /= endpoints.size();
					//cout << "number of lines " << (int)endpoints.size() << " \n";
					//cout << "pre-mean slope is: " << meanSlope << " \n";
					//endpoints.clear();
					for(i = 0; i < endpoints.size(); i++){
						_line line2 = endpoints[i];
						sum += pow(line2.slope-meanSlope, 2);
					}
					sum /= (endpoints.size());
					stdDev = sqrt(sum);
					for(i = 0; i < endpoints.size(); i++){
						_line line3 = endpoints[i];
					   // cout << "line slope is: " << line3.slope << " \n";
						double diff = fabs(line3.slope - meanSlope);
						if(diff<=fabs(stdDev)){
							temp.push_back(line3);
						}

					}
					endpoints.clear();
					for(i = 0; i < temp.size(); i++){
						_line line4 = temp[i];
						//cvLine(clone, line4.pt1, line4.pt2, cvScalar(0, 0, 255), 3);
						endpoints.push_back(line4);
					}
					temp.clear();
					meanSlope = 0;
					for(i = 0; i < endpoints.size(); i++){
						_line line4 = endpoints[i];
						meanSlope -= line4.slope;
						std::cout << line4.slope << " ";
					}
					std::cout << std::endl;
					meanSlope /= endpoints.size();
					std::cout << endpoints.size() << std::endl;
					if (!endpoints.size()) {
						return;
					}
					endpoints.clear();
					//cout << "post-mean slope is: " << meanSlope << " \n";
					ang = atan(1/meanSlope);
					//use atan2 instead

					double ang_in_degrees = ang * (180/CV_PI);
					while(ang_in_degrees>=180){
						ang_in_degrees-=360;
					}
					while(ang_in_degrees<-180){
						ang_in_degrees+=360;
					}
					if(ang_in_degrees < -90)
						ang_in_degrees+=180;
					if(ang_in_degrees > 90)
						ang_in_degrees-=180;
					if(meanSlope > 25){
					}
					
					int curError = avi_state->getProperty(state::cur_heading)-avi_state->getProperty(state::des_heading);
					if (abs(curError) < 5) {
					
						std::cout << "turn " << ang_in_degrees << " degrees, slope: " << meanSlope << "\n";


						int newH = avi_state->getProperty(state::cur_heading) + ang_in_degrees;
						/*
						if(ang_in_degrees > 15) newH +=15;
						if(ang_in_degrees > 5 && < 15) newH +=5;
						if(ang_in_degrees <-5 && >-15) newH -=5;
						if(ang_in_degrees <-15) newH -=15;
						*/
						if (newH > 360) newH -= 360;
						if (newH < 0) newH += 360;
						avi_state->setProperty(state::des_heading, newH);
						avi_state->setProperty(state::cam_mode, state::cm_done);
						std::cout << "PATH DONE\n";
					}
				} //if revolutions is 3


			}

		}
    }
        //reset power to positive

}



