#include "common.h"

ifstream IMUin("/dev/ttyUSB0");


void location_handler::operator()() {
	while (true) {
		char IMUdata[16];
		while (IMUdata[0] != 'A') {
			IMUdata[0]=IMUin.get();
		}
		
		for (int i = 1; i < 16; i++) {
			IMUdata[i]=IMUin.get();
		}
		if (IMUdata[15]!='Z') continue;

		nIMU = IMUdata[1]<<8 | IMUdata[2];
		accX = IMUdata[3]<<8 | IMUdata[4];
		accY = IMUdata[5]<<8 | IMUdata[6];
		accZ = IMUdata[7]<<8 | IMUdata[8];
		gyrX = IMUdata[9]<<8 | IMUdata[10];
		gyrY = IMUdata[11]<<8 | IMUdata[12];
		gyrZ = IMUdata[13]<<8 | IMUdata[14];
		system("sleep 0.1");
	}
}
		   
// set baud of the control board to 115200
void set_baud_controlb() {
	for (int i = 0; i < 2; i++) {
		system("stty -F /dev/ttyUSB0 speed 115200 >> /dev/null");
	}
}
