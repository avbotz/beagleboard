
#include "vision.h"
#include "state.h"
#include "vision_buoy.h"


vision_buoy::vision_buoy(state *sub_state, libconfig::Config *conf) :
		vision_proc(sub_state, conf)
{
	conf->lookupValue("buoy.r_min_pixel_intensity", c_rMinPixelIntensity);
	conf->lookupValue("buoy.g_min_pixel_intensity", c_gMinPixelIntensity);
	conf->lookupValue("buoy.y_min_pixel_intensity", c_yMinPixelIntensity);

	conf->lookupValue("buoy.min_total_intensity", c_minTotalIntensity);
}
vision_buoy::~vision_buoy() {
	
}

void vision_buoy::process_image(const void *p) {
	
	int buoyColor = avi_state->getProperty(state::buoy_color);
	
	count++;
	
	if (!(count % 10)) {
		//		printf(".");
		fflush (stdout);
	}
	convert(p, buf);
	if (c_snapshotEn) {
		if (count > c_snapshotTime) {
			//count=0;
			
			char file[20];
			sprintf(file, "im/im%d.jpg", fileNum++);
			printf("%s\n",file);
			frame_save_jpeg(buf, file);
		}
	}
	
	int i;
	int cogx(0), cogy(0), tot_int(0);
	
	int rmax(0), gmax(0), bmax(0);
	int rmin(255), gmin(255), bmin(255);
	
	for (i = 0; i < (FMT_HEIGHT - c_frameTrimB) * FMT_WIDTH; i++) {
		char* bp = buf+i*3; 
		if (bp[0] < rmin) rmin = bp[0];
		if (bp[1] < gmin) gmin = bp[1];
		if (bp[2] < bmin) bmin = bp[2];
		if (bp[0] > rmax) rmax = bp[0];
		if (bp[1] > gmax) gmax = bp[1];
		if (bp[2] > bmax) bmax = bp[2];
	}
//	printf("%d %d %d %d %d %d", rmin, rmax, gmin, gmax, bmin, bmax);
	
	if (rmax-rmin == 0 || gmax-gmin == 0 || bmax-bmin == 0) {
		avi_state->setProperty(state::cam_mode, state::cm_search);
		//std::cout << "BUOY SEARCH\n";
		return;
	}
	
	for (i = 0; i < (FMT_HEIGHT - c_frameTrimB) * FMT_WIDTH; i++) {
		char* bp = buf+i*3;

		int r = ((int)bp[0] - rmin) * 255 / (rmax - rmin);
		int g = ((int)bp[1] - gmin) * 255 / (gmax - gmin);
		int b = ((int)bp[2] - bmin) * 255 / (bmax - bmin);

		
		int val(0), minVal(0);
		switch (buoyColor) {
			case state::red_buoy: {
				val = 2*r - g - b;
				minVal = c_rMinPixelIntensity;
				break;
			} case state::green_buoy: {
				val = 2*g - r - b;
				int yval = 2*g - 2*b;
				if (yval > c_yMinPixelIntensity) {
					val -= yval;
				}
				minVal = c_gMinPixelIntensity;
				break;
			} case state::yellow_buoy: {
				val =  2*g - 2*b;
				minVal = c_yMinPixelIntensity;
				break;
			} default:
				return;
		}

		if (val > 255) val = 255;
		
		if (val < minVal)
			val = 0;
		bp[0] = val; bp[1] = val; bp[2] = val;
		cogx+= val * (i%FMT_WIDTH);
		cogy+= val * (i/FMT_WIDTH);
		tot_int+= val;
	}
	
	if (tot_int == 0) {
		avi_state->setProperty(state::cam_mode, state::cm_search);
//		std::cout << "BUOY SEARCH\n";
		return;
	}
	
	double cX = cogx / (double)tot_int;
	double cY = cogy / (double)tot_int;
	
	int ang = (int)(45*cX/FMT_WIDTH - 22);	
	int newH = avi_state->getProperty(state::cur_heading) + ang;
	if (newH > 360) newH -= 360;
	if (newH < 0) newH += 360;
	
	int newD = avi_state->getProperty(state::cur_depth);
	if (cY - FMT_HEIGHT/2 > 20) {
		newD += 6 + (cY - FMT_HEIGHT/2 - 20)/10;
	}
	if (cY - FMT_HEIGHT/2 < -20) {
		newD -= 4;
	}
//	printf("tot_int: %d\n", tot_int);
	
	if (!c_noTracking) {
		if (tot_int > c_minTotalIntensity && rmax > 50) {
			avi_state->setProperty(state::des_heading, newH);
			avi_state->setProperty(state::des_depth, newD);
			avi_state->setProperty(state::cam_mode, state::cm_track);
			printf("vision heading: %d\n", newH);
		} else {
			avi_state->setProperty(state::cam_mode, state::cm_search);
	//		std::cout << "BUOY SEARCH\n";
		}
	}
	
	if (c_snapshotEn) {
		if (count > c_snapshotTime) {
			count = 0;
			char file[20];			
			sprintf(file, "im/im%dp.jpg", fileNum-1);
			printf("%s\n",file);
			frame_save_jpeg(buf, file);
		}
	}
	
	printf("\n");
	
}
