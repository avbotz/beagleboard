/* all commented out, all controller specific
#include "common.h"

void canInit();
void canSendReset();
void __attribute__((interrupt, no_auto_psv)) _C1Interrupt(void);

extern int led;
extern int eepromaddr;

int a[3];
int b[3];
int c[3];

void canInit() {
	C1CTRLbits.REQOP = 0b100; //request config mode
	while (C1CTRLbits.OPMODE != 0b100); //wait for config mode
	
	C1CTRLbits.CANCAP = 0; //Disable message receive capture

	//	TIMING REGISTERS
	//16 time quanta per bit
	//14.744 MHz clock
	//115187.5 Hz baud rate (128 clocks per bit)
	//8 clocks per time quanta
	//Tq = 542.6 ns
	C1CTRLbits.CANCKS = 1; //CAN clock = Main clock
	C1CFG1bits.SJW = 0; //(re-)synchronization Jump Width is 1 Tq
	C1CFG1bits.BRP = 3; //Baud rate prescaler
	C1CFG2bits.PRSEG = 2; //Propagation segment is 2 Tq long
	C1CFG2bits.WAKFIL = 0; //Line filter not used for wake-up
	C1CFG2bits.SEG1PH = 7; //Phase segment 1 is 7 Tq long
	C1CFG2bits.SEG2PH = 6; //Phase segment 2 is 6 Tq long
	C1CFG2bits.SEG2PHTS = 1; //Phase segment 2 is freely programmable
	
	//	RECEIVE REGISTERS
	C1RX0CONbits.DBEN = 0; //Double buffers disabled

	//	FILTER REGISTERS
	C1RXF0SIDbits.SID = 0b10000000000; //RX0's filter 0
	C1RXF1SIDbits.SID = 0b10000000000; //RX0's filter 1
	C1RXM0SIDbits.SID = 0b10000000000; //RX0's mask
	C1RXM0SIDbits.MIDE = 0; //RX0 accepts both SID and EID
	
	//	INTERRUPT REGISTERS
	C1INTEbits.RX0IE = 1;	//Receive interrupt enabled
	C1INTEbits.RX1IE = 0;
	_C1IE = 1; //All CAN interrupts enabled
	_C1IP = 7; //Zero priority for CAN interrupts
	
	C1CTRLbits.REQOP = 0b000; //request normal mode
	while (C1CTRLbits.OPMODE != 0b000); //wait for normal mode
}	

void canSendReset() {
	_RE0=1;
	C1TX0CONbits.TXPRI = 3;	//High priority
	
	C1TX0SIDbits.SRR = 0; //Not remote request frame
	C1TX0SIDbits.TXIDE = 0; //Standard identifier (not extended)
	C1TX0EID = 0;
	C1TX0DLCbits.TXRTR = 0; //Not remote request frame
	//Set reserved bits (required by CAN protocol)
	C1TX0DLCbits.TXRB0 = 0;
	C1TX0DLCbits.TXRB1 = 0;
	
	C1TX0DLCbits.DLC = 1; //Message is 1 byte long
	
	//Set message identifier and data
	C1TX0SIDbits.SID10_6 = 0b11111;
	C1TX0SIDbits.SID5_0 = 0b111111;
	C1TX0B1 = 0;
	
	C1TX0CONbits.TXABT = 1; //Will be cleared when TXREQ is set;
	C1TX0CONbits.TXREQ = 1; //Request send of transmit buffer 0
	
	while (C1INTFbits.TX0IF != 1); //Wait for message to be sent successfully
	C1INTFbits.TX0IF = 0; //Clear the interrupt flag;
	_RE0=0;
}	

void __attribute__((interrupt, no_auto_psv)) _C1Interrupt(void)
{
	IFS1bits.C1IF = 0;	//Clear the CAN interrupt flag
//	if (C1INTFbits.RX0IF) {
		C1INTFbits.RX0IF = 0;	//Clear the receive interrupt flag
		led |= 1<<(C1RX0SIDbits.SID & 0b11);
		PORTE=led;
		int id=C1RX0SIDbits.SID & 0b11;
		a[id] = (C1RX0B1&0b11)<<8 | C1INTFbits.RX0OVR;
		b[id] = C1RX0B2;
		c[id] = C1RX0B3;
		C1INTFbits.RX0OVR=0;
		C1RX0CONbits.RXFUL = 0;
		if (led == 7) {
			for (id=0; id<3; id++) {
				WriteWord(eepromaddr++, id);
				WriteWord(eepromaddr++, a[id]);
				WriteWord(eepromaddr++, b[id]);
				WriteWord(eepromaddr++, c[id]);
				WriteWord(0x0000, eepromaddr);
			}
		}
		C1RX0CONbits.RXFUL = 0;	//Tell the module that the receive buffer is open again
//	}
/*	if (C1INTFbits.RX1IF) {
		C1INTFbits.RX1IF = 0;	//Clear the receive interrupt flag
		led |= 1<<(C1RX1SIDbits.SID & 0b11);
		PORTE=led;
		WriteWord(eepromaddr++, C1RX1SIDbits.SID);
		WriteWord(eepromaddr++, (C1RX1B1&0b11)<<8 | (C1RERRCNT&0xff));
		WriteWord(eepromaddr++, C1RX1B2);
		WriteWord(eepromaddr++, C1RX1B3);
		WriteWord(0x0000, eepromaddr);
		C1RX1CONbits.RXFUL = 0;	//Tell the module that the receive buffer is open again
	}*//*
}*/
