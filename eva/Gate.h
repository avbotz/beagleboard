#include "Task.h"

#ifndef GATE_H__
#define GATE_H__

class Gate : public Task {
public:
	Gate(libconfig::Config *conf);
	~Gate();
private:
};

#endif
