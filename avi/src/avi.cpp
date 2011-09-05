#include <iostream>
#include <cstring>
#include <csignal>
#include <cmath>
#include <cstdlib>

#include "avi.h"

//Once set to one, the avi run loop will terminate
volatile int avi::stop_avi = 0;

//Constructor
avi::avi(libconfig::Config *conf) :
				iNav_sender(conf),
				iNav_listener(conf),
				AVnav_com(&avi_state),
				v_buoy(&avi_state, conf),
				v_path(&avi_state, conf),
				vision_mod(NULL, conf),
				buoyNum(0),
				hedgeNum(0),
				camNum(-1),
				nextPath(1)
{

	//Load the configuration variables using libconfig
	conf->lookupValue("avi.min_depth", c_minDepth);
	conf->lookupValue("gate.time", c_gateTime);
	conf->lookupValue("gate.depth", c_gateDepth);
	conf->lookupValue("gate.power", c_gatePower);
	conf->lookupValue("iNav.enable", c_iNavEn);
	conf->lookupValue("buoy.color_1", c_buoyColor[0]);
	conf->lookupValue("buoy.color_2", c_buoyColor[1]);
	conf->lookupValue("buoy.hit_time", c_buoyHitTime);
	conf->lookupValue("buoy.back_speed", c_buoyBackSpeed);
	conf->lookupValue("buoy.back_time", c_buoyBackTime);
	conf->lookupValue("path.follow_time", c_pathFollowTime);
	conf->lookupValue("hedge.hit_time", c_hedgeHitTime);
	conf->lookupValue("vision.video_stream_forward", c_vidDevice[camForward]);
	conf->lookupValue("vision.video_stream_down", c_vidDevice[camDown]);

	srand(avi_state.getTimeStamp());
	//Initialize in mission state Start
	changeState(state::m_start);
}

//Destructor
avi::~avi() {
	printf("AVI Terminated.\n");
}

//Main run loop, called from main thread
void avi::run_loop() {
	
	int lastPathH;

	if (c_iNavEn) {	//For taking pictures while under iNav control
		vision_mod.setVP(&v_buoy);
		changeCamera(camForward);
	}

	//Run loop continues until it is terminated externally (often with Ctrl-C)
	while (!stop_avi) {

		//If iNav is controlling, send status to iNav (not working now, strange loopback problem)
		//Relay all iNav commands to control board
		if (c_iNavEn) {
			if (avi_state.cur_updated) {
				avi_state.cur_updated = 0;
				char toiNav[20];
				int h(avi_state.getProperty(state::cur_heading));
				int d(avi_state.getProperty(state::cur_depth));
				int p(avi_state.getProperty(state::cur_power));
				int k(avi_state.getProperty(state::kill_state));
				toiNav[0] = 'h';
				toiNav[1] = ((h>>6) & 0x3f) + 0x20;
				toiNav[2] = (h & 0x3f) + 0x20;
				toiNav[3] = 'd';
				toiNav[4] = ((d>>6) & 0x3f) + 0x20;
				toiNav[5] = (d & 0x3f) + 0x20;
				toiNav[6] = 'p';
				toiNav[7] = ((p>>6) & 0x3f) + 0x20;
				toiNav[8] = (p & 0x3f) + 0x20;
				toiNav[9] = (k == state::kill_alive)?'k':'l';
				toiNav[10] = '\n';
				toiNav[11] = 0;
//				printf("Sending: %s\n", toiNav);
//				iNav_sender.sendMessage(toiNav, 12);
			}
			char toPic[20];
			if (iNav_listener.getNewMessage(toPic)) {
				toPic[3] = '\n';
				if (toPic[0] == 'h' || toPic[0] == 'd' || toPic[0] == 'p') {
				printf("Receiving: %s\n", toPic);
				AVnav_com.write_serial(toPic, 4);
				}
			}


			continue; //Don't do mission stuff
		}

		//AVI state machine, a different 'case' for each mission state.
		//State is changed with changeState(int newState) method
		switch (avi_state.getProperty(state::mission_state)) {
				
			//Mission state: start
			case state::m_start: {
				if (init_state) {
					init_state = 0;
				}

				//Sets desired heading to the current heading to while aiming for gate
				initHeading = avi_state.getProperty(state::cur_heading);
				avi_state.setProperty(state::des_heading, initHeading);
				
				//Make sure the first path is searched for in the initial heading
				lastPathH = initHeading;


				//Switch to gate state when kill switch is inserted
				if (avi_state.getProperty(state::kill_state) == state::kill_alive) {
					changeState(state::m_gate);
				//	changeState(state::m_path_find);
					break;
				}
				break;
			}
				
			//Mission state: starting gate
			case state::m_gate: {
				//Set initial depth and power
				if (init_state) {
					init_state = 0;
					avi_state.setProperty(state::des_depth, c_gateDepth);
					avi_state.setProperty(state::des_power, c_gatePower);
				}

				//Advance to buoy finding upon gate timer completion
				if (timeDelta() >= c_gateTime) {
					changeState(state::m_buoy_find);
					break;
				}

				break;
			}
				
			//Mission state: search for buoy
			case state::m_buoy_find: {
				static int h_rand[3];
				static int searchNum = 0;
				static int searchTime;
				//Initialize vision processing for finding a specific buoy
				//Pick 3 headings to search (forward, 45 degrees left/right)
				if (init_state) {
					init_state = 0;
					changeCamera(camForward);
					avi_state.setProperty(state::buoy_color, c_buoyColor[buoyNum]);
					vision_mod.setVP(&v_buoy);
					avi_state.setProperty(state::des_power, 120);
					h_rand[0] = avi_state.getProperty(state::cur_heading);	//Centers on current heading.  Maybe not a good idea?
					h_rand[1] = h_rand[0] - 45;
					h_rand[2] = h_rand[0] + 45;
					if (h_rand[1] < 0) h_rand[1]+=360;
					if (h_rand[2] >= 360) h_rand[2]-=360;
					searchTime = 0;
				}

				//Switch to buoy ramming state when camera grabs sub control
				if (avi_state.getProperty(state::cam_mode) == state::cm_track) {
					changeState(state::m_buoy_hit);
					break;
				}

				//If the desired heading has been reached and 2 seconds have passed, choose a new search heading
				int h_err = abs(avi_state.getProperty(state::des_heading) -
								avi_state.getProperty(state::cur_heading));

				if (h_err < 2 && timeDelta() - searchTime > 2000) {
					searchTime = timeDelta();
					searchNum = (searchNum + 1) % 3;
					avi_state.setProperty(state::des_heading, h_rand[searchNum]);
					
				}

				break;
			}
				
			//Mission state: buoy hitting (buoy is locked upon)
				
			//KNOWN PROBLEM: When the sub moves forward, especially when not at the correct depth
			//	it may pitch upwards or otherwise lose the buoy off the bottom of the frame, triggering
			//	a false hit.
			//Possible fixes: delay before a hit, minimum brightness for a hit in vision (buoy should almost fill camera)
			//KNOWN PROBLEM: The sub has difficulty disengaging with a buoy once hit. 
			//	The sub should either push forward through the buoy, back up, rise, or otherwise prevent
			//	the buoy from catching or preventing the sub from moving forward.
			case state::m_buoy_hit: {
				//Ramming speed
				if (init_state) {
					init_state = 0;
					avi_state.setProperty(state::des_power, 150);
				}

				//If the depth error is large, stop to adjust depth
				if (abs(avi_state.getProperty(state::cur_depth) -
						avi_state.getProperty(state::des_depth)) < 6) {
					avi_state.setProperty(state::des_power, 150);
				} else {
					avi_state.setProperty(state::des_power, 100);
				}

				//If the camera drops sub control, return to search pattern
				//If the camera drops control after tracking for a long period,
				// assume buoy has been hit and switch to next buoy mission state
				if (avi_state.getProperty(state::cam_mode) == state::cm_search) {

					if (timeDelta() > c_buoyHitTime) {
						changeState(state::m_buoy_next);
						break;
					} else {
						changeState(state::m_buoy_find);
						break;
					}

				}

				break;
			}
				
			//Mission state: next buoy (advances to a different buoy color, or next task)
			case state::m_buoy_next: {
				//After 1 second of continuing forward, add one to the buoy number and reverse motors
				//If there are no more buoys to hit, advance mission state, continuing slowly forward
				if (init_state) {
					if (timeDelta() > 1000) {
						init_state = 0;
						buoyNum = (buoyNum + 1);
						if (buoyNum >= 2) {
							buoyNum = 0;
							avi_state.setProperty(state::des_heading, initHeading);
							avi_state.setProperty(state::des_power, 110);
							changeState(state::m_path_find);
							break;
						}
						avi_state.setProperty(state::des_power, c_buoyBackSpeed);
					} else {
						break;
					}
				}

				//After backup timer is expired, stop and return to buoy search mission state
				if (timeDelta() > c_buoyBackTime) {
					avi_state.setProperty(state::des_power, 100);
					changeState(state::m_buoy_find);
					break;
				}
				break;
			}
				
			//Mission state: path searching
			//Currently the following zig zag _/\  /\  /...
			//pattern is implemented             \/  \/
			case state::m_path_find: {
				static int h_rand[3];		//Potential search headings
				static int searchStage;		//current stage of search pattern
				static int searchTime;		//the timestamp of the start of the current stage
				static int startedSearchLeg;//whether forward movement in stage has begun
				static int firstLeg;		//whether current stage is first in pattern (should be half length)

				if (init_state) {
					init_state = 0;
					changeCamera(camDown);
					avi_state.setProperty(state::cam_mode, state::cm_search);
					vision_mod.setVP(&v_path);
					//3 potential search headings centered around last known path heading (set in path_next/other mission state)
					//Currently, only headings 1 and 2 are used (no direct forward movement)
					h_rand[0] = lastPathH;
					h_rand[1] = lastPathH - 45;
					h_rand[2] = lastPathH + 45;
					if (h_rand[1] < 0) h_rand[1]+=360;
					if (h_rand[2] >= 360) h_rand[2]-=360;
					searchStage = 0;
					searchTime = -3000;
					startedSearchLeg = 0;
					firstLeg = 1;
					avi_state.setProperty(state::des_heading, h_rand[(searchStage % 2) + 1]);

				}

				//If vision takes control of sub, path has been acquired, switch to path centering state
				if (avi_state.getProperty(state::cam_mode) == state::cm_track) {
					changeState(state::m_path_center);
					break;
				}
				
				
				int h_err = abs(avi_state.getProperty(state::des_heading) -
								avi_state.getProperty(state::cur_heading));
				
				//If the heading error is small and the forward movement has not begun,
				//mark the time and move forward
				if (h_err < 2 && !startedSearchLeg) {
					searchTime = timeDelta();
					//If the stage is the first in the pattern, subtract from the
					//timestamp to shorten the stage by one half
					if (firstLeg) {
						firstLeg = 0;
						searchTime -= 3000;
					}
					startedSearchLeg = 1;
					avi_state.setProperty(state::des_power, 140);
				}
				
				//If the sub is moving forward for at least 6 seconds, 
				//advance the search stage number, stop, and adopt the next heading
				if (startedSearchLeg && timeDelta() - searchTime > 6000) {
					searchStage++;

					avi_state.setProperty(state::des_heading, h_rand[(searchStage % 2) + 1]);
					avi_state.setProperty(state::des_power, 100);

					startedSearchLeg = 0;
				}
				

                break;
			}
			//Mission state: centering over path
			case state::m_path_center: {
				if (init_state) {
					init_state = 0;
					avi_state.setProperty(state::des_power, 100);
				}

				int nState = avi_state.getProperty(state::cam_mode);

				//If the vision processor indicates it has lost the path, return to path finding state.
				//If the vision processor has indicated it is finished analyzing the path,
				//	make sure the desired heading is in the general direction of the initial heading 
				//	and advance to path following state
				if (nState == state::cm_search) {
					changeState(state::m_path_find);
					break;
				} else if (nState == state::cm_done) {
					
					int heading = avi_state.getProperty(state::des_heading);
					
					if (abs(initHeading - heading) < 90 || abs(initHeading - heading - 360) < 90 || abs(initHeading - heading + 360) < 90) {
					} else {
						avi_state.setProperty(state::des_heading, (heading + 180) % 360);
					}
					
					changeState(state::m_path_follow);
					break;
				}

                break;
			}

			//Mission state: following path
			case state::m_path_follow: {
				//Move forward along the path
				if (init_state) {
					init_state = 0;
					avi_state.setProperty(state::des_power, 150);
					lastPathH = avi_state.getProperty(state::des_heading);
				}

				//Decide the next mission state after the path follow time limit is met
				//Should involve some sort of search pattern
				//Currently just returns to path search
				if (timeDelta() > c_pathFollowTime) {
					changeState(state::m_path_find);
					break;

				/*	if (nextPath == nextPath_buoy){
						changeState(state::m_buoy_find);
						nextPath = nextPath_hedge;
						break;
					}
					if (nextPath == nextPath_hedge){
						changeState(state::m_hedge_find);
						nextPath = nextPath_path;
						break;
					}
					if (nextPath == nextPath_path){
						//change state to octagon class
						changeState(state::m_path_find);
						nextPath = nextPath_hedge2;
						break;
					}
					if (nextPath == nextPath_hedge2){
						changeState(state::m_hedge_find);
						break;
					}
					changeState(state::m_done);
					break;*/
				}
				break;
			}
			
				//NOTE:
				//The following hedge code is essentially a copy of the buoy code (green)
				//It isn't tested to work, because we could follow the path directly through the hedge
				//should probably do more line detection than color following, because of the likelihood
				//of hitting the side if this is ever implemented.
				
			//Mission state: search for hedge
			case state::m_hedge_find: {
				static int h_rand[3];
				//Initialize vision processing for finding a specific buoy
				//Pick 3 headings to search (forward, 45 degrees left/right)
				if (init_state) {
					init_state = 0;
					changeCamera(camForward);
					avi_state.setProperty(state::buoy_color, state::green_buoy);
					vision_mod.setVP(&v_buoy);
					avi_state.setProperty(state::des_power, 100);
					h_rand[0] = avi_state.getProperty(state::cur_heading);
					h_rand[1] = h_rand[0] - 45;
					h_rand[2] = h_rand[0] + 45;
					if (h_rand[1] < 0) h_rand[1]+=360;
					if (h_rand[2] >= 360) h_rand[2]-=360;
				}

				//Switch to hedge jumping state when camera grabs sub control
				if (avi_state.getProperty(state::cam_mode) == state::cm_track) {
					changeState(state::m_hedge_hit);
					break;
				}

				//If the desired heading has been reached, choose a new search heading
				int h_err = abs(avi_state.getProperty(state::des_heading) -
								avi_state.getProperty(state::cur_heading));

				if (h_err < 2 && timeDelta() > 1000) {

					int lottery = rand() % 3;
					avi_state.setProperty(state::des_heading, h_rand[lottery]);
				}

				break;
			}
				//Mission state: hedge hitting (hedge is locked upon)
			case state::m_hedge_hit: {
				//Ramming speed
				if (init_state) {
					init_state = 0;
					avi_state.setProperty(state::des_power, 150);
				}

				//If the depth error is large, stop to adjust depth
/*				if (abs(avi_state.getProperty(state::cur_depth) -
						avi_state.getProperty(state::des_depth)) < 6) {
					avi_state.setProperty(state::des_power, 150);
				} else {
					avi_state.setProperty(state::des_power, 100);
				}
			*/
				//If the camera drops sub control, return to search pattern
				//If the camera drops control after tracking for a long period,
				// assume buoy has been hit and switch to next buoy mission state
				if (avi_state.getProperty(state::cam_mode) == state::cm_search) {

					if (timeDelta() > c_hedgeHitTime) {
						changeState(state::m_hedge_next);
						break;
					} else {
						changeState(state::m_hedge_find);
						break;
					}

				}

				break;
			}
			case state::m_hedge_next: {
				if (init_state) {
					if (timeDelta() > 1000) {
						init_state = 0;
						hedgeNum = (hedgeNum + 1);
						if (hedgeNum >= 1) {
							hedgeNum = 0;
							changeState(state::m_path_find);
							break;
						}
					} else {
						break;
					}
				}

				//After backup timer is expired, stop and return to buoy search mission state
				if (timeDelta() > 10000) {
					avi_state.setProperty(state::des_power, 100);
					changeState(state::m_hedge_find);
					break;
				}
				break;
			}
				
			//Mission state: done (mission has completed, wait for kill switch removal
			//		Possible future improvement: add a mission switch in addition to the kill switch
			//		to prevent needing to toggle kill to restart mission code
			case state::m_done:
				if (init_state) {
					init_state = 0;
					vision_mod.setVP(NULL);
					avi_state.setProperty(state::des_depth, 0);
					avi_state.setProperty(state::des_power, 100);
				}

				if (avi_state.getProperty(state::kill_state) == state::kill_dead) {
					changeState(state::m_start);
					break;
				}
				break;
		}

		//Send changes in desired state to the AVNav board
		if (avi_state.isDesUpdated()) {
			avi_state.des_updated = 0;
			AVnav_com.write_command('h', avi_state.getProperty(state::des_heading));
			int dep = avi_state.getProperty(state::des_depth);
			if (dep < c_minDepth) dep = c_minDepth;

			AVnav_com.write_command('d', dep);
			AVnav_com.write_command('p', avi_state.getProperty(state::des_power));
		}

		//If the kill switch is ever pulled, enter the done mission state
		if (avi_state.getProperty(state::kill_state) == state::kill_dead &&
			avi_state.getProperty(state::mission_state) != state::m_start) {
			changeState(state::m_done);
		}


	}
}

//Change the mission state
//Initializes the mission state timer from the avi_state object
void avi::changeState(int newState) {
	avi_state.setProperty(state::mission_state, newState);
	time_state = avi_state.getTimeStamp();
	init_state = 1;
	printf("[%d.%03d]Changing mission state: %d\n", time_state/1000, time_state%1000, newState);

}

//Changes the active camera used by the vision module
//	if the given camera is not currently in use, restart the vision module with the new camera
void avi::changeCamera(int newCam) {
	if (newCam == camNum) return;
	camNum = newCam;
	vision_mod.stop_vision();
	vision_mod.start_vision(c_vidDevice[newCam]);
}

//Get the elapsed time of the current mission state
int avi::timeDelta() {
	return avi_state.getTimeStamp() - time_state;
}

//Handle signals for terminating the program
//Should also implement signals other than SIGINT to avoid crashing
void handleSignal(int sig) {
	//Terminate the program, given by pressing Control-C
	if (sig == SIGINT) {
		avi::stop_avi = 1;
		printf("\n--------------------------------------------------------------------------------\n");
		printf("SIGINT received\n");
	}
}


int main(int argc, char *argv[]) {

	//Set up signal handler
	signal(SIGINT, handleSignal);

	if (argc != 2) {
		printf("No configuration file given\n");
		exit(0);
	}

	//Import and read the configuration file with libconfig
	libconfig::Config conf;
	conf.readFile(argv[1]);

	//Initialize mission code
	avi barracuda(&conf);
	printf("BARRACUDA OPERATIONAL.\n");

	//Begin mission run loop
	barracuda.run_loop();
	printf("AVI run loop ended.\n");

	return 0;
}


