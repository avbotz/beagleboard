#include "common.h"
#include <stdio.h> //needed for my printf >.>
//note that on Windows no window is created, need to enter debug mode to see values, probably due to missing stdafx.h

void solve(int*, int, double*, double*, double*);
void addMat(double*, double*, double);
void multMat(double*, double);
/* more controller stuff
extern void ioInit();
extern void clockInit();
extern void canInit();

extern void canSendReset();*/

double v; //velocity
double hx[4]; //initializing position arrays for hydrophones
double hy[4];
double hz[4];

volatile int led; //not needed in simulator
volatile unsigned int eepromaddr;

//_FOSC(CSW_FSCM_OFF & XT_PLL8); specific to controller?

int main (void)
{ 
	/*led = 0; controller specific, doesn't work apparently
	eepromaddr = ReadWord(0x0000);
	if (eepromaddr == -1) {
		eepromaddr=1;
		WriteWord(0x0000, eepromaddr);
	}
	ioInit();
	clockInit();
	canInit();
//	canSendReset();
	while(1) { //some form of synchronization? not very elegant if it is
		while (led == 0);
		long i = 1000000;
		while (i--);
	//	canSendReset();
		//PORTE = 0;
		led=0;
	}*/

	 
	int ht[4] = {-13, 0, -9, -7}; //to simulate solver
	double vx, vy, vz;
	solve(ht, -5372, &vx, &vy, &vz);
	
	return 0;
}
void solve(int* ht, int depth, double* vx, double* vy, double* vz)
{
	short i, j, f;
	double gx0, gx1;
	double gy0, gy1;
	double gz0, gz1;
	double gt0, gt1;
	double matrix[3][5];
	double jax, jay, jaz;
	double a, b, c;
	double disc;

//	v = SIGNALSPEED / (UNITARRAYSIDE * SAMPLESPEED);
	v = 1.5; //velocity constant, probably for debug
	//setting values of hydrophone positions, setting ht[] likely for debug purposes
	hx[0] = 0;	hy[0] = 0; 				hz[0] = 12.2474487139;	ht[0] = -13;
	hx[1] =-10;	hy[1] = -5.7735026919;	hz[1] = -4.0824829046;	ht[1] = 0;
	hx[2] = 10;	hy[2] = -5.7735026919;	hz[2] = -4.0824829046;	ht[2] = -9;
	hx[3] = 0;	hy[3] = 11.5470053838;	hz[3] = -4.0824829046;	ht[3] = -7;
	depth = -5372; //uncalibrated depth

	f=0; //finds hydrophone closest to emitter
	if (ht[0] < ht[f])
		f = 0;
	if (ht[1] < ht[f])
		f = 1;
	if (ht[2] < ht[f])
		f = 2;
	if (ht[3] < ht[f])
		f = 3;

	j=0; //creates TDOA equation matrix centered at hydrophone f
	for (i = 0; i < 4; ++i)
	{
		if (i != f) {
			matrix[j][0] = 2*(hx[i] - hx[f]);
			matrix[j][1] = 2*(hy[i] - hy[f]);
			matrix[j][2] = 2*(hz[i] - hz[f]);
			matrix[j][3] = 2*pow(v,2)*(ht[i] - ht[f]);
			matrix[j][4] = pow(hx[i], 2) + pow(hy[i], 2) + pow(hz[i], 2) - pow(v*ht[i], 2) -
						   pow(hx[f], 2) + pow(hy[f], 2) + pow(hz[f], 2) - pow(v*ht[f], 2);
			j++;
		}
	}

	//Find the reduced row echelon form of the matrix
    for (i = 0; i < 3; i++){
        multMat(matrix[i], 1/matrix[i][i]);
        for (j = 0; j < 3; j++){
            if (j != i) addMat(matrix[j], matrix[i], (-1 * matrix[j][i]));
        }
    }

    //ja[dim] = (solution of that dim) - position of hydrophone closest to emitter
    //position of emitter then?
	jax = matrix[0][4] - hx[f];
	jay = matrix[1][4] - hy[f];
	jaz = matrix[2][4] - hz[f];

    //creating quadratic from the solution of the matrix, not exactly sure why; matrix[x][3] = xT; matrix[x][4] = x
	a = pow(matrix[0][3], 2) + pow(matrix[1][3], 2) + pow(matrix[2][3], 2) - pow(v, 2); //a = xT^2 + yT^2 + zT^2 - v^2
	b = -2 * (jax*matrix[0][3] + jay*matrix[1][3] + jaz*matrix[2][3] - pow(v, 2)*ht[0]); //b = -2(x*xT + y*yT + z*zT - v^2*t(0)
	c = pow(jax, 2) + pow(jay, 2) + pow(jaz, 2) - pow(v*ht[0], 2); //c = x^2 + y^2 + z^2 - (v * t(0))^2

	disc = (pow(b,2) - 4*a*c); //discriminant of quadratic created
	if (disc >= 0) //if solution to quadratic is real (if not then what happens?)
	{
		double tmpa = b / (-2*a);
		double tmpb = sqrt(disc) / (2*a);
		gt0 = tmpa+tmpb; //solutions of the quadratic
		gt1 = tmpa-tmpb;
		gx0 = matrix[0][4] - matrix[0][3]*gt0; //finding the solution of the matrix, setting t to be solutions to quadratics
		gx1 = matrix[0][4] - matrix[0][3]*gt1; //this should narrow down possible locations of pinger to 2
		gy0 = matrix[1][4] - matrix[1][3]*gt0;
		gy1 = matrix[1][4] - matrix[1][3]*gt1;
		gz0 = matrix[2][4] - matrix[2][3]*gt0;
		gz1 = matrix[2][4] - matrix[2][3]*gt1;
		*vx = gx0 - gx1; //distance between the found locations of the pingers...
		*vy = gy0 - gy1;
		*vz = gz0 - gz1;
		if (fabs(*vz) > .05) //if too deep then adjust for wave speed
		{
			double mag = depth / *vz;
			*vx *= mag;
			*vy *= mag;
			*vz *= mag;
		}
	}
}
void addMat(double* rowA, double* rowB, double scalar) //adds two rows of the matrix with a scalar if necessary
{
    int i = 0;
	for (i = 0; i < 5; i++) rowA[i] += rowB[i] * scalar;
}

void multMat(double* row, double scalar) //multiplies a row of the matrix by a scalar value
{
    int i = 0;
	for (i = 0; i < 5; i++) row[i] *= scalar;
}

