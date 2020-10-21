#include "Task.h"

Task::Task(){
	TaskID = (int)this;	
};

Task::Task(unsigned long _interval){
	enabled = true;
	_cached_next_run = 0;
	last_run = millis();

	TaskID = (int)this;
#ifdef USE_TASK_NAMES
	TaskName = "Task ";
	TaskName = TaskName + TaskID;
#endif

	setInterval(_interval);
};

Task::Task(void (*callback)(void), unsigned long _interval){
	enabled = true;
	onRun(callback);
	_cached_next_run = 0;
	last_run = millis();

	TaskID = (int)this;
	#ifdef USE_TASK_NAMES
	TaskName = "Task ";
	TaskName = TaskName + TaskID;
	#endif

	setInterval(_interval);
};

void Task::runned(unsigned long time){
	// Saves last_run
	last_run = time;

	// Cache next run
	_cached_next_run = last_run + interval;
}

void Task::setInterval(unsigned long _interval){
	// Save interval
	interval = _interval;

	// Cache the next run based on the last_run
	_cached_next_run = last_run + interval;
}

bool Task::shouldRun(unsigned long time){
	// If the "sign" bit is set the signed difference would be negative
	bool time_remaining = (time - _cached_next_run) & 0x80000000;

	// Exceeded the time limit, AND is enabled? Then should run...
	return !time_remaining && enabled;
}

void Task::run(){
	if(_onRun != NULL && !_paused)
		_onRun();

	// Update last_run and _cached_next_run
	runned();
}