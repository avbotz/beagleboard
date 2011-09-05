#define BYTESIZE   8
#define PARITY     NOPARITY
#define STOPBITS   ONESTOPBIT
#define ASCII_XON  0x11
#define ASCII_XOFF 0x13

#define BUFFER_SIZE         4096
#define READ_BUFFER_TIMEOUT 1000

#define PM30F_ROW_SIZE 32
#define PM33F_ROW_SIZE 64*8
#define EE30F_ROW_SIZE 16

#define COMMAND_NACK     0x00
#define COMMAND_ACK      0x01
#define COMMAND_READ_PM  0x02
#define COMMAND_WRITE_PM 0x03
#define COMMAND_REPROG  0x04
#define COMMAND_READ_CM  0x06
#define COMMAND_WRITE_CM 0x07
#define COMMAND_RESET    0x08
#define COMMAND_READ_ID  0x09

#define TRUE 1
#define FALSE 0


enum eFamily
{
	dsPIC30F,
	dsPIC33F,
	PIC24H,
	PIC24F
};

typedef struct
{
	const char           * pName;
	unsigned short int   Id;
	unsigned short int   ProcessId;
	eFamily              Family;
} sDevice;

void msleep(unsigned int ms);



