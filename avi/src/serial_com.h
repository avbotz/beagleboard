#ifndef __serial_com_h
#define __serial_com_h

#include <termios.h>
#include <sys/signal.h>

class state;

class serial_com {
public:
	serial_com(state*);
	~serial_com();
	
	int write_serial(const char*, int);
	int write_command(char com_prefix, int data);
		
private:
	volatile bool m_stoprequested;
	int ser_f;
	struct termios options;
	char r_buf[256];
	char mes_buf[20];
	int i_mes_buf;
	
	state *avi_state;
	
	struct boostImpl;
	boostImpl* bimpl;
	
	void process_serial(char *);
	int open_port();
	void read_serial_t();
	inline int read_pic_num(char* buf);
};

#endif
