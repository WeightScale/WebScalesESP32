
#include "Task.h"
#include "TaskController.h"

TaskController::TaskController(unsigned long _interval): Task(_interval){
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
	int checks = 0;
	for(int i = 0; i < MAX_TASKS && checks <= cached_size; i++){
		// Object exists? Is enabled? Timeout exceeded?
		if(task[i]){
			checks++;
			if(task[i]->shouldRun(time)){
				task[i]->run();
			}
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
	for(int i = 0; i < MAX_TASKS; i++){
		if(task[i] != NULL && task[i]->TaskID == _task->TaskID)
			return true;
	}

	// Find an empty slot
	for(int i = 0; i < MAX_TASKS; i++){
		if(!task[i]){
			// Found a empty slot, now add Thread
			task[i] = _task;
			cached_size++;
			return true;
		}
	}

	// Array is full
	return false;
}

void TaskController::remove(int id){
	// Find Tasks with the id, and removes
	bool found = false;
	for(int i = 0; i < MAX_TASKS; i++){
		if(task[i]->TaskID == id){
			task[i] = NULL;
			cached_size--;
			return;
		}
	}
}

void TaskController::remove(Task* _task){
	remove(_task->TaskID);
}

void TaskController::clear(){
	for(int i = 0; i < MAX_TASKS; i++){
		task[i] = NULL;
	}
	cached_size = 0;
}

int TaskController::size(bool cached){
	if(cached)
		return cached_size;

	int size = 0;
	for(int i = 0; i < MAX_TASKS; i++){
		if(task[i])
			size++;
	}
	cached_size = size;

	return cached_size;
}

Task* TaskController::get(int index){
	int pos = -1;
	for(int i = 0; i < MAX_TASKS; i++){
		if(task[i] != NULL){
			pos++;

			if(pos == index)
				return task[i];
		}
	}

	return NULL;
}

