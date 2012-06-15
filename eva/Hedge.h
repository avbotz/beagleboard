#include "VisionTask.h"

#ifndef HEDGE_H__
#include HEDGE_H__

class Hedge : public VisionTask{
public:
	Hedge(libconfig::Config *conf);
	~Hedge();
private:
};

#endif
