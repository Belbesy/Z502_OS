#include "sched.h"

#include "../../global.h"
#include "../../syscalls.h"
#include "../../protos.h"

#include "../kernel/system_structs.h"

#include "alarm.h"
#include "process_queue.h"

#include <cstdio>
#include <list>
#include <queue>
#include <map>

extern alarm_manager_t alarm_manager;
extern scheduler_t scheduler;

void _wakeup(void* ref) {
	scheduler.wakeup((PCB *) ref);
}
void _feed(void * ref) {
	scheduler.feed((PCB*) ref);
}

 scheduler_t::scheduler_t(){

}

void scheduler_t::init(){

 }
void scheduler_t::wakeup(PCB* p) {

	if (p->STATE != PROCESS_STATE_SLEEPING) {
		printf(
				"INTERNAL ERROR (scheduler->wakeup): process state should be set to sleeping!\n");
		return;
	}

	if (!this->validate_priority(p->PRIORITY)) {
		printf(
				"INTERNAL ERROR (scheduler->wakeup): process doesn't have valid priority!\n");
		return;
	}

	this->add_to_ready(p);
	p->STATE = PROCESS_STATE_READY;
}

void scheduler_t::feed(PCB* p) {
	if (p->STATE != PROCESS_STATE_READY) {
		printf(
				"INTERNAL ERROR (scheduler->feed): process wasn't ready when set ana alarm!\n");
		return;
	}
	this->starving.enqueue(p);
}

void scheduler_t::schedule() {

	PCB * p;

	if (starving.size()) {
		p = starving.dequeue();
	} else {
		for (int i = 0; i < MAX_PRIORITY; i++) {
			if (this->ready[i].size()) {
				p = this->ready[i].dequeue();
				break;
			}
		}
	}

	if (p) {
		ZCALL(Z502_SWITCH_CONTEXT(SWITCH_CONTEXT_KILL_MODE, &p->CONTEXT));
	} else {
		//TODO : questionable!
		ZCALL(Z502_IDLE());
	}

}

bool scheduler_t::sleep(PCB* p, int time, int* err) {
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
	this->remove_from_ready(p);

	alarmable wakeup_alarm(time, _wakeup, p);
	alarm_manager.add_alarm(wakeup_alarm);

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

	// try to remove from suspended
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

	// insert into ready queue
	this->add_to_ready(p);

	if (!this->validate_priority(p->ID)) {
		// invalid process priority, corrupted data was given
		printf(
				"USER_ERROR (scheduler->resume): invalid process priority given! \n");
		*err = USER_ERROR_CORRUPTED_PROCESS;
		return false;
	}

	// update process state
	p->STATE = PROCESS_STATE_READY;

	// no error occurred
	*err = 0;

	// moved to ready queue successfully
	return true;
}

bool scheduler_t::change_priority(PCB* p, int new_priority, int* err) {

	if (!this->validate_priority(p->ID)) {
		// invalid process priority, corrPCB* pupted data was given
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
	if (p->STATE == PROCESS_STATE_READY) {
		this->remove_from_ready(p);
		p->PRIORITY = new_priority;
		this->add_to_ready(p);
	} else {
		p->PRIORITY = new_priority;
	}
	return true;
}

bool scheduler_t::suspend(PCB* p, int* err) {
	switch (p->STATE) {
	case PROCESS_STATE_READY:
		// process is ready suspend it
		if (!this->validate_priority(p->ID)) {
			// invalid process priority, corrupted data was given
			printf(
					"USER_ERROR (scheduler->resume): invalid process priority given! \n");
			*err = USER_ERROR_CORRUPTED_PROCESS;
			return false;
		}
		// try to remove from ready queue
		if (!this->remove_from_ready(p)) {
			printf(
					"USER_ERROR (scheduler->suspend): process isn't in ready queue! \n");
			*err = USER_ERROR_CORRUPTED_PROCESS;
			return false;
		}
		break;
	case PROCESS_STATE_SLEEPING:

		// remove alarm
		if (!alarm_manager.remove(p)) {
			printf(
					"USER_ERROR (scheduler->suspend): process wasn't find in alarm!\n");
			return false;
		}
		break;

	case PROCESS_STATE_BLOCKED:
		// can't do that! return error
		printf(
				"USER_ERROR (scheduler->suspend): can't suspend process waiting for even!\n");
		*err = USER_ERROR_INVALID_STATE_TRANSITION;
		return false;
	default:
		// can't do that! return error
		printf("USER_ERROR (scheduler->suspend): unsupported state!\n");
		*err = USER_ERROR_INVALID_STATE_TRANSITION;
		return false;

	}

	// change process state to suspended
	p->STATE = PROCESS_STATE_SUSPENDED;
	// put in queue
	this->suspended.enqueue(p);
	// success
	return true;
}

bool scheduler_t::terminate(PCB* p, int* err) {
	switch (p->STATE) {
	case PROCESS_STATE_READY:
		// check if valid priority
		if (!this->validate_priority(p->PRIORITY)) {
			printf(
					"USER_ERROR (scheduler->terminate): Invalid priority of process recieved! \n");
			*err = USER_ERROR_CORRUPTED_PROCESS;
			return false;
		}
		if (!this->remove_from_ready(p)) {
			printf(
					"USER_ERROR (scheduler->terminate): process wasn't found in ready queue as expected \n");
			*err = USER_ERROR_CORRUPTED_PROCESS;
			return false;
		}
		break;

	case PROCESS_STATE_BLOCKED:
		if (!this->blocked.remove(p)) {
			printf(
					"USER_ERROR (scheduler->terminate): process wasn't found in blocked queue");
			*err = USER_ERROR_CORRUPTED_PROCESS;
			return false;
		}
		break;
	case PROCESS_STATE_SLEEPING:
		// remove alarm
		if (!alarm_manager.remove(p)) {
			printf(
					"USER_ERROR (scheduler->terminate): process wasn't found in sleeping queue!\n");
			return false;
		}

		break;
	case PROCESS_STATE_SUSPENDED:
		if (!this->suspended.remove(p)) {
			printf(
					"USER_ERROR (scheduler->terminate): process wasn't found in suspended queue\n");
			*err = USER_ERROR_CORRUPTED_PROCESS;
			return false;

		}
		break;
	default:
		printf("USER_ERROR (scheduler->terminate): unsupported state");
		*err = USER_ERROR_INVALID_STATE_TRANSITION;
		break;
	}

	p->STATE = PROCESS_STATE_ZOMBIE;

	return true;
}

bool scheduler_t::add_to_ready(PCB * p) {
	alarm_manager.add_alarm(alarmable(STARVING_INTERVAL, _feed, p));
	this->ready[p->PRIORITY].enqueue(p);
	return true;
}

bool scheduler_t::remove_from_ready(PCB * p) {
	if (!alarm_manager.remove(p)) {
		printf(
				"INTERNAL ERROR (scheduler->remove_from_ready): proces in ready queue didn't have a matching alarm!\n");
		return false;
	}
	return (this->ready[p->PRIORITY].remove(p));
}

bool scheduler_t::validate_priority(int p){
	return  0 < p && p < MAX_PRIORITY;
}
