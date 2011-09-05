#ifndef __avi_h
#define __avi_h

#include <libconfig.h++>

#include "state.h"
#include "net_listener.h"
#include "net_sender.h"
#include "vision_proc.h"
#include "vision_buoy.h"
#include "vision.h"
#include "serial_com.h"
#include "vision_path.h"

class avi {
public:
	avi(libconfig::Config *nConf);
	~avi();

	static volatile int stop_avi;

	void run_loop();
	int timeDelta();
	void changeState(int newState);
	void changeCamera(int newCam);

private:

	state avi_state;

	net_sender iNav_sender;
	net_listener iNav_listener;
	serial_com AVnav_com;
	vision_buoy v_buoy;
	vision_path v_path;
	vision vision_mod;

	//Configuration variables
	int c_minDepth;
	int c_gateTime;
	int c_gateDepth;
	int c_gatePower;
	int c_iNavEn;
	int c_buoyColor[2];
	int c_buoyBackTime;
	int c_buoyBackSpeed;
	int c_buoyHitTime;
	int c_hedgeHitTime;
	int c_pathFollowTime;
	const char *c_vidDevice[2];

	enum cam_devices {
		camForward,
		camDown
	};

	enum nextPath_tasks{
        nextPath_buoy,
        nextPath_hedge,
        nextPath_path,
        nextPath_hedge2
	};
	int time_state;
	int init_state;

	int buoyNum;
	int hedgeNum;
	int camNum;
	int nextPath;

	int initHeading;

};


#endif
