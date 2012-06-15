#include "Task.h"

#ifndef HYDROPHONE_H__
#define HYDROPHONE_H__

class Hydrophone : public Task {
public:
	Hydrophone(libconfig::Config *conf);
	~Hydrophone();
private:
};

#endif