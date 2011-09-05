#include "common.h"

int mode;

//thread to handle console input.
void input_handler::operator()() {
	if (mode == 3) {		// Basically, if the mode isn't 3, it makes the thread and kills the thread. 
		while (1) {		
			char input;
			cin >> input;
			
			switch (input) {
				case 'g':		// All motors
					set_goals(0, 0, 0, 254);
					break;
				case 'f':		// Forward
					set_goals(0, 0, 127, 127);
					break;
				case 'b':		// Back
					set_goals(254, 254, 127, 127);
					break;
				case 'u':		// Up
					set_goals(127, 127, 254, 0); //127, 127, 254, 0
					break;
				case 'd':		// Down
					set_goals(127, 127, 0, 254);
					break;
				case 'r':		// Turn right
					set_goals(0, 254, 127, 127);
					break;
				case 'l':		// Turn left
					set_goals(254, 0, 127, 127);
					break;
				case 'z':		// Debug; runs the back motor
					set_goals(127, 127, 127, 254);	//127, 127, 127, 254
					break;
				case 'y':		// Debug: runs all but the left motor
					set_goals(255, 0, 0, 0);		//127, 0, 0, 0
					break;
				case 's':		// Stops the motors immediately. This bypasses set_goals.
					adjust_motors(127, 127, 127, 127);
					set_goals(127, 127, 127, 127);
					break;
				default:
					cout << "Syntax error.\n";
					break;
			}
		}
	}
}

void output::operator()() {
	while (true) {
		
		cout << setw(7) << "left" << setw(7) << "right" << setw(7) << "front" << setw(7) << "back" << setw(10) << "killed" << endl;
		cout << setw(7) << (int)m_left[2] << setw(7) << (int)m_right[2] << setw(7) << (int)m_fwd[2] << setw(7) << (int)m_back[2] << setw(10);
		if(!isRunning) cout << "yes";
		else cout << "no";
		cout << endl;
		/*
		cout << setw(5) << "nIMU" << setw(5) << "accX" << setw(5) << "accY" << setw(5) << "accZ" << setw(5) << "gyrX" << setw(5) << "gyrY" << setw(5) << "gyrZ" << endl;
		cout << setw(5) << (int)nIMU << setw(5) << (int)accX << setw(5) << (int)accY << setw(5) << (int)accZ << setw(5) << (int)gyrX << setw(5) << (int)gyrY << setw(5) << (int)gyrZ << endl<<endl;
		cout << endl << endl;
		*/
		
		system("sleep 0.25");
	}
}

void signal_handler::operator()() {
	int sig;
	while(1) {
		sigwait(&set, &sig);
		cout << endl;
		quitFlag = 1;			//send a message to the motor control
		while (quitFlag != 2) {
			system("sleep 0.25");
		}
		exit(0);
	}
}

void get_mode () {
	cout << "MODE?\n1 = testing\n2 = competition\n3 = manual control\n\n> ";
	while (!(mode == 1 || mode == 2 || mode == 3)) cin >> mode;
}
