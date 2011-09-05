#ifndef __vision_h
#define __vision_h

//See vision.cpp for explanation/apology.

#include <iostream>
#include <libconfig.h++>

class vision_proc;


#define FMT_WIDTH 320
#define FMT_HEIGHT 240


class vision {
public:
	vision(vision_proc *process, libconfig::Config *conf);
	~vision();
	
	void start_vision(const char * inDevName);
	void stop_vision();
	
	void setVP(vision_proc *process);
	void run_vision();
	void take_picture();
	
private:
	
	struct buffer {
		void * start;
		size_t length;
	};
	typedef enum {
		IO_METHOD_READ,
		IO_METHOD_MMAP,
		IO_METHOD_USERPTR,
	} io_method;
	
	char devName[20];
	
	io_method		io;
	int 			fd;
	buffer *		buffers;
	unsigned int	n_buffers;
	
	long last_tick, curr_tick;
	int frames;
	float fps;
	
	int dropFrames;
			
	char takePic;
	volatile bool m_stoprequested;

	vision_proc *vp;

	struct boostImpl;
	boostImpl* bimpl;
	
	void open_device();
	void init_device();
	void init_mmap();
	void start_capturing();
	int read_frame();
	void stop_capturing();
	void uninit_device();
	void close_device();
	
	void errno_exit(const char*);
	int xioctl(int fd, int request, void *arg);
	
	int fps_calc();
	unsigned int tick_count();
};

#endif