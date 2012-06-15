
#ifndef TASK_H__
#define TASK_H__

class Task {

public:
	Task(libconfig::Config *conf);
	~Task();

	void action(void* input, 

	//null = take the probableNextTask of the task before
	Task** probableNextTask;

private:

};

#endif
