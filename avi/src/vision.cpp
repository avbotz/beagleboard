/*Reasoning behind this class:
 *	For some reason, OpenCV can not handle capturing images from our cameras.
 *	It starts reading the first frame in the middle (still does that, don't know why),
 *	converts the frames into jpeg (I have no idea why, really doesn't work if
 *	you start the first frame in the middle), converts the color space from YUV to RGB
 *	COMPLETELY wrong (absolutely no clue why), and is generally horribly slow at that.
 *	This class uses Video 4 Linux 2 directly to grab the YUV frames, and
 *	gives it to a vision_proc class, which can convert, save, do whatever.
 *	Most of this is sample code, which I just wrapped in a thread and added methods to
 *	start, stop, and change cameras.  As a result, its mostly impossible to read.
 *	It works most of the time and it does it pretty fast, so no complaining allowed.
 *	I also put in some unused, untested, and likely unworking FPS measuring stuff.
 *	Enjoy.
 *
 *	PS the original non-chopped down sample code is in the V4L2 API
 *	 http://v4l2spec.bytesex.org/spec/capture-example.html
 */

#include <sys/time.h>
#include <highgui.h>
#include <cv.h>
#include <cxcore.h>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <cerrno>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>


#include "vision.h"
#include "vision_proc.h"

#define UPDATE_SPEED 1000

#define CLEAR(x) memset (&(x), 0, sizeof (x))


struct vision::boostImpl {
		boost::mutex m_mutex;
		boost::thread m_thread;
};

vision::vision(vision_proc *process, libconfig::Config *conf) :
		io(IO_METHOD_MMAP),
		fd(-1),
		buffers(NULL),
		n_buffers(0),
		last_tick(tick_count()),
		curr_tick(tick_count()),
		frames(0),
		fps(20.0f),
		takePic(0),
		m_stoprequested(1),
		vp(process),
		bimpl(new boostImpl)
{
};

vision::~vision() {
	stop_vision();
	printf("Vision module terminated.\n");
};

void vision::start_vision(const char *inDevName) {
	if (!m_stoprequested) {
		return;
	}
	m_stoprequested = 0;
	strcpy(devName, inDevName);
	printf("Started vision on: %s", devName);
	
	dropFrames = 10;
	open_device();
	init_device();
	start_capturing();
	
	bimpl->m_thread = boost::thread(boost::bind(&vision::run_vision, this));
}

void vision::stop_vision() {
	if (m_stoprequested) {
		return;
	}		
	m_stoprequested = 1;
	bimpl->m_thread.join();
	
	stop_capturing();
	uninit_device();
	close_device();
}

void vision::setVP(vision_proc *process) {
	boost::mutex::scoped_lock l(bimpl->m_mutex);
	vp = process;
}

void vision::run_vision() {
	while (!m_stoprequested) {
		for (;;) {
			fd_set fds;
			struct timeval tv;
			int r;
			
			FD_ZERO (&fds);
			FD_SET (fd, &fds);
			
			/* Timeout. */
			tv.tv_sec = 2;
			tv.tv_usec = 0;
			
			r = select (fd + 1, &fds, NULL, NULL, &tv);
			
			if (-1 == r) {
				if (EINTR == errno)
					continue;
				errno_exit ("select");
			}
			
			if (0 == r) {
				fprintf (stderr, "select timeout\n");
				exit (EXIT_FAILURE);
			}
			
			if (read_frame())
				break;
			
			/* EAGAIN - continue select loop. */
		}
	}
}



void vision::take_picture() {
	boost::mutex::scoped_lock l(bimpl->m_mutex);
	takePic = 1;
}

int vision::fps_calc() {
	curr_tick = tick_count();
	long tick_diff = curr_tick - last_tick;
	frames++;
	if (tick_diff >= UPDATE_SPEED) {
		last_tick = curr_tick;
		float calc = 1 / (UPDATE_SPEED/1000.0f);
		float fps_calc = (float)frames*calc;
		fps += fps_calc;
		fps /= 2;
		frames = 0;
		return fps;
	} else
		return -1;
}

unsigned int vision::tick_count() {
		struct timeval tv;
		if (gettimeofday(&tv, NULL) != 0)
			return 0;

		return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}



void vision::open_device() {
	struct stat st; 
	
	if (-1 == stat (devName, &st)) {
		fprintf (stderr, "Cannot identify '%s': %d, %s\n",
					devName, errno, strerror (errno));
		exit (EXIT_FAILURE);
	}
	if (!S_ISCHR (st.st_mode)) {
		fprintf (stderr, "%s is no device\n", devName);
		exit (EXIT_FAILURE);
	}
	
	fd = open (devName, O_RDWR /* required */ | O_NONBLOCK, 0);
	
	struct v4l2_input input;
	
	memset (&input, 0, sizeof (input));
	if (-1 == ioctl (fd, VIDIOC_G_INPUT, &input.index)) {
		perror ("VIDIOC_G_INPUT");
		exit (EXIT_FAILURE);
	}
	if (-1 == ioctl (fd, VIDIOC_ENUMINPUT, &input)) {
		perror ("VIDIOC_ENUM_INPUT");
		exit (EXIT_FAILURE);
	}
	if (-1 == fd) {
		fprintf (stderr, "Cannot open '%s': %d, %s\n",
					devName, errno, strerror (errno));
		exit (EXIT_FAILURE);
	}
}


void vision::init_device() {
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	unsigned int min;
	if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s is no V4L2 device\n", devName);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_QUERYCAP");
		}
	}
	
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf (stderr, "%s is no video capture device\n", devName);
		exit (EXIT_FAILURE);
	}
	
	switch (io) {
		case IO_METHOD_READ:
			if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
				fprintf (stderr, "%s does not support read i/o\n", devName);
				exit (EXIT_FAILURE);
			}
			break;
		
		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
			if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
				fprintf (stderr, "%s does not support streaming i/o\n", devName);
				exit (EXIT_FAILURE);
			}
			break;
	}
	
	
	/* Select video input, video standard and tune here. */
	
	CLEAR (cropcap);
	
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */
	
		if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
				case EINVAL:
				/* Cropping not supported. */
				break;
				default:
				/* Errors ignored. */
				break;
			}
		}
	} else {        
		/* Errors ignored. */
	}
	
	struct v4l2_format fmt;
	
	
	CLEAR (fmt);
	
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = FMT_WIDTH; 
	fmt.fmt.pix.height      = FMT_HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
	
	if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt))
	errno_exit ("VIDIOC_S_FMT");
	
	/* Note VIDIOC_S_FMT may change width and height. */
	
	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;
	
	switch (io) {
        case IO_METHOD_MMAP:
                init_mmap ();
                break;
		default:
			printf("SEE ORIGINAL CODE FOR OTHER MEMORY METHODS");
			exit(1);
	}
}

void vision::init_mmap() {
	struct v4l2_requestbuffers req;
	
	CLEAR (req);
	
	req.count               = 4;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;
	
	if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s does not support memory mapping\n", devName);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_REQBUFS");
		}
	}
	
	if (req.count < 2) {
		fprintf (stderr, "Insufficient buffer memory on %s\n", devName);
		exit (EXIT_FAILURE);
	}
	
	buffers = (buffer *)calloc (req.count, sizeof (*buffers));
	
	if (!buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}
	
	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;
	
		CLEAR (buf);
	
		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = n_buffers;
	
		if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
			errno_exit ("VIDIOC_QUERYBUF");
	
		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = 
			mmap (	NULL /* start anywhere */,
					buf.length,
					PROT_READ | PROT_WRITE /* required */,
					MAP_SHARED /* recommended */,
					fd, buf.m.offset);
	
		if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit ("mmap");
	}
}

void vision::start_capturing() {
	unsigned int i;
	enum v4l2_buf_type type;
	
	switch (io) {
		case IO_METHOD_MMAP:
			for (i = 0; i < n_buffers; ++i) {
				struct v4l2_buffer buf;
				
				CLEAR (buf);
				
				buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory      = V4L2_MEMORY_MMAP;
				buf.index       = i;
				
				if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
					errno_exit ("VIDIOC_QBUF");
			}
			
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			
			if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
				errno_exit ("VIDIOC_STREAMON");
			
			break;

		default:
			printf("SEE ORIGINAL CODE FOR OTHER MEMORY METHODS");
			exit(1);
	}
}

int vision::read_frame() {
	struct v4l2_buffer buf;
	
	switch (io) {
		case IO_METHOD_MMAP:
			CLEAR (buf);
			
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			
			if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
				switch (errno) {
					case EAGAIN:
						return 0;
					case EIO:
						/* Could ignore EIO, see spec. */
						/* fall through */
					default:
						errno_exit ("VIDIOC_DQBUF");
				}
			}
			assert (buf.index < n_buffers);
			if (dropFrames > 0) {
				dropFrames--;
			} else {
				boost::mutex::scoped_lock l(bimpl->m_mutex);
				if (vp != NULL) {
					vp->process_image(buffers[buf.index].start);
				}
			}
			if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
				errno_exit ("VIDIOC_QBUF");
			
			break;
		default:
			printf("SEE ORIGINAL CODE FOR OTHER MEMORY METHODS");
			exit(1);
	}	
	return 1;
}


void vision::stop_capturing() {
	enum v4l2_buf_type type;
	
	switch (io) {
		case IO_METHOD_READ:
			/* Nothing to do. */
			break;
	
		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
				errno_exit ("VIDIOC_STREAMOFF");
			break;
	}
}

void vision::uninit_device() {
	unsigned int i;
	
	switch (io) {
		case IO_METHOD_READ:
			free (buffers[0].start);
			break;		
		case IO_METHOD_MMAP:
			for (i = 0; i < n_buffers; ++i)
				if (-1 == munmap (buffers[i].start, buffers[i].length))
					errno_exit ("munmap");
			break;
			
		case IO_METHOD_USERPTR:
			for (i = 0; i < n_buffers; ++i)
				free (buffers[i].start);
			break;
	}
	
	free (buffers);
}

void vision::close_device() {
	if (-1 == close (fd))
		errno_exit ("close");
	fd = -1;
}


void vision::errno_exit (const char *s) {
	fprintf (stderr, "%s error %d, %s\n", s, errno, strerror (errno));
	exit (EXIT_FAILURE);
}

int vision::xioctl(int fd, int request, void *arg) {
	int r;
	do r = ioctl (fd, request, arg);
	while (-1 == r && EINTR == errno);
	return r;
}