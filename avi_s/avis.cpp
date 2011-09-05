#include "common.h"

sigset_t set;

//These arrays send input to the miniSSC2 which in turn sends to the motors. 
//The format for the arrays: {SYNC_BYTE, servo number, speed}
//SYNC_BYTE tells the miniSSC2 that we're about to send new instructions.
//The servo number picks the motor. 1 = right, 2= back, 3= front, 4 = left
//We set the speed anywhere between 0 and 254. 127 is off, 254 is 100% forward, 0 is 100% back. Extrapolate.
//On the back motor, the numbers are reversed. 0 is 100% forward, 254 is 100% back. 127 is still off.

volatile char m_fwd[] = {SYNC_BYTE, 3, 127};
volatile char m_back[] = {SYNC_BYTE, 2, 127};
volatile char m_right[] = {SYNC_BYTE, 1, 127};
volatile char m_left[] = {SYNC_BYTE, 4, 127};

volatile char m_goals[] = {127, 127, 127, 127};	//array of the goal motorspeeds. we need this because of our OCP issues.


volatile bool isRunning = true;			//if set to false, this kills the motors and quits. can be accessed from other threads (killswitch function)
volatile int quitFlag = 0;			//tells the motor control function when to shut the motors off
volatile bool hasBeenKilled = false;		//tells the script reader if the kill switch has been pulled and to restart

volatile int nIMU = 0, accX = 1, accY = 2, accZ = 3, gyrX = 4, gyrY = 5, gyrZ = 6;

volatile bool isIMUdatalocked = false;
//libconfig::Config conf;

int main() {
	//conf.read("config.conf");
	get_mode();

	sigemptyset (&set);
	//sigaddset (&set, SIGQUIT);
	//sigaddset (&set, SIGUSR1);
	sigaddset (&set, SIGINT);	
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	set_baud_motors();
	set_baud_controlb();

	motor_control m;						//create the wrapper handling motors
	input_handler in;						//create the wrapper handling console input
	kill_switch kil;						//create the wrapper handling kill switch
	output out;
	signal_handler signal;
	location_handler location;
	boost::thread t_motor(boost::ref(m));	//make boost threads
	boost::thread t_input(boost::ref(in));
	boost::thread t_kill(boost::ref(kil));
	boost::thread t_output(boost::ref(out));
	boost::thread t_signal(boost::ref(signal));
	boost::thread t_location(boost::ref(location));
	
	if (mode == 1 || mode == 2) {
		gate();
	}
	
	t_motor.join();							//don't exit until all the threads exit.
	t_input.join();
	t_kill.join();
	t_output.join();
	t_signal.join();
	t_location.join();
	return 0;
}
