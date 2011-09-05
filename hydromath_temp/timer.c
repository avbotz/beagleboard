#include "common.h"

void clockInit();

void clockInit()
{
	/* yay more controller-specific, disabled until I can find p30f4012.h or until we get a new board
	T1CON = 0x8000;	//Timer 1 set on in 1:1 prescaler mode
	PR1 = 0xFFFF;
	_T1IE = 0;
	T2CON = 0x8038; //Timer 2 set on with a 1:256 prescaler, 32 bit mode on
	PR2 = 0xFFFF;
	PR3 = 0xFFFF;
	_T3IE = 0;*/
}
