#include "VisionTask.h"

#ifndef PATH_H__
#define PATH_H__

class Path : public VisionTask{
public:
	Path(libconfig::Config *conf);
	~Path();
private:
};

#endif
