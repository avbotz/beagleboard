#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <cerrno>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "serial_com.h"
#include "state.h"

struct serial_com::boostImpl {
		boost::mutex m_mutex;
		boost::thread m_thread;
};


serial_com::serial_com(state *sub_state) 
	:	m_stoprequested(false),
		i_mes_buf(0),
		avi_state(sub_state),
		bimpl(new boostImpl)
{
	ser_f = open_port();
	if (ser_f != -1) {
		bimpl->m_thread = boost::thread(boost::bind(&serial_com::read_serial_t, this));
		memset(mes_buf, 0, 20);
	}
}

serial_com::~serial_com() {
	m_stoprequested = true;
	bimpl->m_thread.join();
	close(ser_f);
	printf("Serial com module terminated.\n");
}

//Open the first USB-serial converter
//Set up all sorts of crazy options to make it work
//Makes serial port non-blocking, meaning that the read function won't
//wait for input, it will return with 0 if there is no input
int serial_com::open_port() {
	int fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1) {
		perror("open_port: Unable to open /dev/ttyUSB0 - ");
		return -1;
	}
		
	tcgetattr(fd, &options);
	//Baud rate = 115200 baud
	cfsetispeed(&options, B115200);
	cfsetospeed(&options, B115200);
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
	options.c_cflag |= CS8;
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;
	options.c_cc[VMIN]  = 0;
	options.c_cc[VTIME]  = 0;
	fcntl(fd, F_SETOWN, getpid());
	fcntl(fd, F_SETFL, FNDELAY);
	tcsetattr(fd, TCSANOW, &options);
	
	return (fd);
}

//Send a message over the serial port
int serial_com::write_serial(const char* s, int size) {
	return write(ser_f, s, size);
}

//Write a numeric command in the AVNav format
// 'h' = set desired heading (0 - 359)
// 'd' = set desired depth   (0 - 200)
// 'p' = set desired power   (0 - 200)
int serial_com::write_command(char com_prefix, int data) {
	char command[4];
	command[0] = com_prefix;
	command[1] = ((data>>6) & 0x3f) + 0x20;
	command[2] = (data & 0x3f) + 0x20;
	command[3] = '\n';
	return write(ser_f, command, 4);
}

//Processes a message from the control board and sets the current state
void serial_com::process_serial(char * mes) {
	int i(0);
	int h(-1), d(-1), p(-1), k(-1);
	while (mes[i] != 0 && i < 20) {
		switch (mes[i]) {
			case 'h':
				h = read_pic_num(&mes[i+1]);
				avi_state->setProperty(state::cur_heading, h);
				i+=3;
				break;
			case 'd':
				d = read_pic_num(&mes[i+1]);
				avi_state->setProperty(state::cur_depth, d);
				i+=3;
				break;
			case 'p':
				p = read_pic_num(&mes[i+1]);
				avi_state->setProperty(state::cur_power, p);
				i+=3;
				break;
			case 'l':
				k = 0;
				avi_state->setProperty(state::kill_state, state::kill_dead);
				i++;
				break;
			case 'k':
				k = 1;
				avi_state->setProperty(state::kill_state, state::kill_alive);
				i++;
				break;
			default:
				i++;
				break;
		}
	}
	printf("Heading: %d\tDepth: %d\tPower: %d\tKill State: %d\n", h, d, p, k);
}

//Converts encoded number from AVNav into an int

//AVNav data encoding:
//Numbers are sent in 2 bytes, each with a value 0x20-0x5f, 32-95, or ' '-'_'
//The numbers are sent in base 64, with 0x20 being equal to 0 and 0x5f being equal to 63.
//The first byte is the upper digit (most significant byte/big endian)
//So the number 299 is sent as "$K" and is decoded as follows:
//	$K => 0x24, 0x4B => 36, 75 => 4, 43 => 4*64 + 43 = 299
//This keeps both bytes as readily readable ASCII characters that 
//	are not the lower case letters used for commands
inline int serial_com::read_pic_num(char* buf) {
	return ((*buf - 0x20)<<6) | (*(buf+1)-0x20);
}
		
//The serial read loop which reads and buffers the incoming serial messages
//It splits messages on the '\n' character, processing complete messages
void serial_com::read_serial_t() {
	while (!m_stoprequested) {
		int res = read(ser_f, r_buf, 256);
		if (res) {
			boost::mutex::scoped_lock l(bimpl->m_mutex);
			int i;
			for (i=0; i<res; i++) {
				mes_buf[i_mes_buf] = r_buf[i];
				if (r_buf[i] == '\n') {
					process_serial(mes_buf);
					memset(mes_buf, 0, 20);
					i_mes_buf = 0;
				} else {
					i_mes_buf = (i_mes_buf + 1) % 20;
				}
			}
		}
	}
}
