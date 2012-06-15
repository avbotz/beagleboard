#include "VisionTask.h"

#ifndef DROPPER_H__
#define DROPPER_H__

class Dropper : public VisionTask {
public:
	Dropper(libconfig::Config *conf);
	~Dropper();
private:
};

#endif
