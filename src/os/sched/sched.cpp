#include "sched.h"

#include "../../global.h"
#include "../kernel/system_structs.h"

#include "alarm.h"

#include <list>
#include <queue>
#include <map>

scheduler_t scheduler;

bool scheduler_t::sleep(PCB* p, int* err) {
	// check if valid process transition
	if (p->STATE != PROCESS_STATE_READY) {
		// bogus call process should be ready to sleep
		printf(
				"USER_ERROR (scheduler->sleep): process should be in ready status to sleep!\n");
		*err = USER_ERROR_INVALID_STATE_TRANSITION;
		return false;
	}


	// validate priority for bogus calls
	if (!this->validate_priority(p->ID)) {
		// invalid process priority, corrupted data was given
		printf(
				"USER_ERROR (scheduler->sleep): invalid process priority given! \n");
		*err = USER_ERROR_CORRUPTED_PROCESS;
		return false;
	}

	// remove from ready queue
	this->ready[p->PRIORITY].remove(p);

	// insert into sleeping queue
	this->sleeping.queue(p);

	// update process state
	p->STATE = PROCESS_STATE_SLEEPING;
	// no error occured
	*err = 0;
	// moved to sleeping queue successfully
	return true;
}

bool scheduler_t::resume(PCB*p, int* err) {
	// check if valid process transition
	if (p->STATE != PROCESS_STATE_SUSPENDED) {
		// bogus call process should be ready to sleep
		printf(
				"USER_ERROR (scheduler->resume): process should be in suspended to resume!\n");
		*err = USER_ERROR_INVALID_STATE_TRANSITION;
		return false;
	}

	// try to remove from ready queue
	if (!this->suspended.remove(p)) {
		// bogus call process doesn't exist in suspended queue
		printf(
				"USER_ERROR (scheduler->resume): process wasn't found in suspended queue! \n");
		*err = USER_ERROR_CORRUPTED_PROCESS;
		return false;
	}

	// validate priority for bogus calls
	if (!this->validate_priority(p->ID)) {
		// invalid process priority, corrupted data was given
		printf(
				"USER_ERROR (scheduler->resume): invalid process priority given! \n");
		*err = USER_ERROR_CORRUPTED_PROCESS;
		return false;
	}

	// insert into sleeping queue
	this->ready[p->PRIORITY].queue(p);

	// update process state
	p->STATE = PROCESS_STATE_SLEEPING;
	// no error occurred
	*err = 0;
	// moved to ready queue successfully
	return true;
}

bool scheduler_t::change_priority(PCB* p, int new_priority, int* err) {

	if (!this->validate_priority(p->ID)) {
		// invalid process priority, corrupted data was given
		printf(
				"USER_ERROR (scheduler->resume): invalid process priority given! \n");
		*err = USER_ERROR_CORRUPTED_PROCESS;
		return false;
	}

	if (!this->validate_priority(new_priority)) {
		// invalid process priority, corrupted data was given
		printf(
				"USER_ERROR (scheduler->resume): invalid process new priority given! \n");
		*err = USER_ERROR_INVALID_ARGUMENT;
		return false;
	}

	// the only case we need to change its place when ready
	if(p->STATE == PROCESS_STATE_READY){
		this->ready[p->PRIORITY].remove(p);
		this->ready[new_priority].queue(p);
	}

	p->PRIORITY = new_priority;
	return true;
}


bool scheduler_t::suspend(PCB* p, int* err){
	switch(p->STATE){
	case PROCESS_STATE_READY:
		if (!this->validate_priority(p->ID)) {
			// invalid process priority, corrupted data was given
			printf(
					"USER_ERROR (scheduler->resume): invalid process priority given! \n");
			*err = USER_ERROR_CORRUPTED_PROCESS;
			return false;
		}
		this->ready[p->PRIORITY].remove(p);

		break;
	}



	this->suspended.queue(p);
}

void schedule() {
	scheduler.schedule();
}

void feed() {
	//TODO.
}
