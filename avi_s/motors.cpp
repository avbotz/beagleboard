#include "common.h"
#include <libconfig.h++>

ofstream fout("/dev/ttyS2");					//create a filestream for the serial port. Unix abstracts ports as files; you write to a port by writing to that file.
ifstream config;
libconfig::Config conf;

//this function sends motor instructions of the miniSSC2 every 0.25 seconds until we want to quit.
//Then it quits after stopping the motors.					
void motor_control::operator()() {
	cout << "Entering Sid's milkshake...\nReady for input.\n\n";  //deleting this will make the program crash. Sid is the omniscient being living within our submarine. His milkshake powers the batteries.

	while (true) {
		if (quitFlag) {			//from the signal handler for a SIGINT
			set_goals(127,127,127,127);
			adjust_motors(127,127,127,127);
			quitFlag = 2;
		}
		else if (isRunning){
			system("sleep 0.25");
			adjust_motors(m_goals[0], m_goals[1], m_goals[2], m_goals[3]);
			//adjust_motors((m_goals[0] + m_left[2]) / 2, (m_goals[1] + m_right[2]) / 2, (m_goals[2] + m_fwd[2]) / 2, (m_goals[3] + m_back[2]) / 2);
		}
		else {
			set_goals(127,127,127,127);
			adjust_motors(127, 127, 127, 127);
		}
		//sends updates to the miniSSC2 by writing each motor's data to the serial port
		//sending individual bytes fixes the "address will always evaluate as true" issue
		
		fout << m_fwd[0] << m_fwd[1] << m_fwd[2] << endl;		
		fout << m_left[0] << m_left[1] << m_left[2] << endl;
		fout << m_back[0] << m_back[1] << m_back[2] << endl;
		fout << m_right[0] << m_right[1] << m_right[2] << endl;
		if (quitFlag)		//acknowledge so that the signal handler knows it can quit
			quitFlag = 2;
	}
	
	cout << "Exiting Sid's milkshake...\n"; 
}

void set_goals(char left, char right, char forward, char back) {
	if (left != 255) m_goals[0] = left;
	if (right != 255) m_goals[1] = right;
	if (forward != 255) m_goals[2] = forward;
	if (back != 255) m_goals[3] = back;
}

//this function is to be used internally to change motor speed.
//the input is 0 to 255 where 0 to 254 are speeds and 255 means dont change the current speed
int adjust_motors (char left, char right, char forward, char back) {
	if (left > 255 || left < 0 || right > 255 || right < 0 || forward > 255 || forward < 0 || back > 255 || back < 0)
		return 0;
	if (left != 255) m_left[2] = left;
	if (right != 255) m_right[2] = right;
	if (forward != 255) m_fwd[2] = forward;
	if (back != 255) m_back[2] = back;
	return 1;
}

//sets the baud rate of the serial port on the BeagleBoard to 9600.
//We run this command twice. The first time only returns the current rate, and the second sets it.
void set_baud_motors() {
	for (int i = 0; i < 2; i++)
		system("stty -F /dev/ttyS2 speed 9600 >> /dev/null");
}

//this function reads libconfig and sends motors commands that should send the sub through the gate
void gate() {
	conf.readFile("config.conf");
	libconfig::Setting &gate = conf.lookup("motors.gate");
	int control[5];
	int i;
	while (true){
		for (i = 0; i < gate.getLength(); i++){
			for (int j = 0; j < 5; j++)
				control[j] = gate[i][j];
			set_goals((char) control[0], (char) control[1], (char) control[2], (char) control[3]);
			stringstream sleepTime;
			sleepTime << "sleep " << control[4];
			system(sleepTime.str().c_str());
			//system("sleep %d", control[4]);
			if (hasBeenKilled && isRunning) {
				cout << "Killswitch reset.\n";
				i = -1;
				hasBeenKilled = false;
			}
			if (!control[4]){
				while (true){
					if (hasBeenKilled && isRunning) {
						cout << "Killswitch reset.\n";
						i = -1;
						hasBeenKilled = false;
					}
					system("sleep 0.25");
				}
			}
			if (!isRunning) i = -1;
		}
		set_goals(127, 127, 127, 127);
		adjust_motors(127, 127, 127, 127);
		while (true){
			if (hasBeenKilled && isRunning) {
				cout << "Killswitch reset.\n";
				i = -1;
				hasBeenKilled = false;
				break;
			}
			system("sleep 0.25");
		}
	}
}
