#include <iostream>
#include <iomanip>
#include <fstream>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <pthread.h>
#include <libconfig.h++>

// libraries from devmem2
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>

#ifndef __COMMON_H_
#define __COMMON_H_

#define SYNC_BYTE 255					//keep this value at 255 or it will fuck up. I don't think you want it to fuck up.


//void kill_motors();
int adjust_motors(char, char, char, char);	//left, right, forward, back
void set_goals(char, char, char, char);
void set_baud_motors();
void set_baud_controlb();
void gate();
void get_mode();
//void read_conf_motors(int* timer);


unsigned long read_mem();

//motors.cpp
struct motor_control {
	void operator()();
};

//user.cpp
struct input_handler {
	void operator()();
};

//kill_switch.cpp
struct kill_switch {
	void operator()();
};

//user.cpp
struct output {
	void operator()();
};

//user.cpp
struct signal_handler {
	void operator()();
};

//controlboard.cpp
struct location_handler {
	void operator()();
};

extern sigset_t set;		//used for blocking SIGINT.

using namespace std;

//These arrays send input to the miniSSC2 which in turn sends to the motors. 
//The format for the arrays: {SYNC_BYTE, servo number, speed}
//SYNC_BYTE tells the miniSSC2 that we're about to send new instructions.
//The servo number picks the motor. 1 = right, 2= back, 3= front, 4 = left
//We set the speed anywhere between 0 and 254. 127 is off, 254 is 100% forward, 0 is 100% back. Extrapolate.
//On the back motor, the numbers are reversed. 0 is 100% forward, 254 is 100% back. 127 is still off.

extern volatile char m_fwd[];
extern volatile char m_back[];
extern volatile char m_right[];
extern volatile char m_left[];

extern volatile char m_goals[];	//array of the goal motorspeeds. we need this because of our OCP issues.


extern volatile bool isRunning;			//if set to false, this kills the motors and quits. can be accessed from other threads (killswitch function)
extern volatile int quitFlag;			//tells the motor control function when to shut the motors off
extern volatile bool hasBeenKilled;		//tells the script reader that the kill switch has been pulled and to restart the script

extern int mode;				// competition, testing, or manual control. see the function for details.

extern volatile int nIMU, accX, accY, accZ, gyrX, gyrY, gyrZ;


#endif
