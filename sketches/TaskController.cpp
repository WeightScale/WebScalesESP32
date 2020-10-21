#include "TaskController.h"

TaskController::TaskController(unsigned long _interval)
	: Task(_interval)
	, _tasks(LinkedList<Task *>([](Task *c){ delete c; })) {
	cached_size = 0;

	clear();
	setInterval(_interval);

	#ifdef USE_TASK_NAMES
		// Overrides name
		TaskName = "TaskController ";
		TaskName = TaskName + TaskID;
	#endif
}

/*
	ThreadController run() (cool stuf)
*/
void TaskController::run(){
	// Run this thread before
	if(_onRun != NULL)
		_onRun();

	unsigned long time = millis();
	//int checks = 0;
	for (const auto& t : _tasks) {
		if (t->shouldRun(time)){
			t->run();
		}						
	}
	// ThreadController extends Thread, so we should flag as runned thread
	runned();
}


/*
	List controller (boring part)
*/
bool TaskController::add(Task* _task){
	// Check if the Thread already exists on the array
	for(const auto& t : _tasks) {
		if (t!= NULL && t->TaskID == _task->TaskID)
			return true;					
	}
	if (_tasks.length() < MAX_TASKS){
		_tasks.add(_task);
		cached_size++;
		return true;
	}	
	// Array is full
	return false;
}

void TaskController::remove(Task* _task){
	if (!_task)
		return;			
	_tasks.remove(_task);
	cached_size--;
}

void TaskController::clear(){
	_tasks.free();
	cached_size = 0;
}

int TaskController::size(bool cached){
	if(cached)
		return cached_size;
	cached_size = _tasks.length();

	return cached_size;
}

Task* TaskController::get(int index){
	int pos = -1;
	for (const auto& t : _tasks) {
		if (t != NULL) {
			pos++;

			if (pos == index)
				return t;
		}					
	}
	return NULL;
}

