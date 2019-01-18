
#ifndef _TASKCONTROLLER_H_
#define _TASKCONTROLLER_H_

#include "Task.h"
#include "inttypes.h"

#define MAX_TASKS		15

class TaskController: public Task{
	protected:
	Task* task[MAX_TASKS];
	int cached_size;
	public:
	TaskController(unsigned long _interval = 0);

	// run() Method is overrided
	void run();

	// Adds a thread in the first available slot (remove first)
	// Returns if the Task could be added or not
	bool add(Task* _task);

	// remove the thread (given the Task* or TaskID)
	void remove(int _id);
	void remove(Task* _task);

	// Removes all tasks
	void clear();

	// Return the quantity of Tasks
	int size(bool cached = true);

	// Return the I Task on the array
	// Returns NULL if none found
	Task* get(int index);
};


#endif /* TASKCONTROLLER_H_ */

