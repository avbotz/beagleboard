#include "Task.h"

#ifndef VISION_TASK_H__
#define VISION_TASK_H__

class VisionTask : public Task {
public:
	VisionTask(libconfig::Config *conf);
	~VisionTask();

	bool intensiveSearch(CVImage* image);
	bool basicSearch(CVImage* image);
private:
};

#endif
