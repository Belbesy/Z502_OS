#include "sched.h"

#include "../../global.h"
#include "../../syscalls.h"
#include "../../protos.h"

#include "../kernel/system_structs.h"

#include "alarm.h"
#include "process_queue.h"

#include <cstdio>
#include <pthread.h>
#include <list>
#include <queue>
#include <map>

extern alarm_manager_t alarm_manager;
extern scheduler_t scheduler;

extern pthread_mutex_t scheduler_mutex;
extern pthread_cond_t scheduler_cond;

bool sleeping_waiting;
bool wake_up_time(void * data) {
	alarmable* alarm = (alarmable*) data;
	if (alarm->time > 0) {
		return false;
	}
	delete alarm;
	return true;
}
void _wakeup(void* ref) {
	scheduler.wakeup((PCB *) ref);
}
void _feed(void * ref) {
	scheduler.feed((PCB*) ref);
}

scheduler_t::scheduler_t() {

}

void scheduler_t::init() {

}
void scheduler_t::wakeup(PCB* p) {

	if (p->STATE != PROCESS_STATE_SLEEPING) {
		printf("INTERNAL ERROR (scheduler->wakeup): process(%d:%s) state should be set to sleeping while was found(%d)!\n", p->ID, p->PROCESS_NAME, p->STATE);
		return;
	}

	if (!this->validate_priority(p->PRIORITY)) {
		printf("INTERNAL ERROR (scheduler->wakeup): process doesn't have valid priority!\n");
		return;
	}
	this->add_to_ready(p);
	p->STATE = PROCESS_STATE_READY;
}

void scheduler_t::feed(PCB* p) {
	if (p->STATE != PROCESS_STATE_READY) {
		printf("INTERNAL ERROR (scheduler->feed): process wasn't ready when set ana alarm!\n");
		return;
	}
	this->starving.enqueue(p);
}

bool scheduler_t::schedule(PCB* calling, bool save, condition_predicate condition, void * arg, bool suspend) {

	sleeping_waiting = false;
	//pthread_mutex_lock(&scheduler_mutex);
	bool ret = true;
	PCB * p = NULL;

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
printf("da5el %d\n", p->ID);
		Z502_SWITCH_CONTEXT(save, &p->CONTEXT);
		printf("5lst %d\n", p->ID);
		ret = true;
	} else {

		if (save) {
			if (suspend) {
				// shouldn't suspend the only process error
				perror("ERROR! User shouldn't suspend the only running process");
				ret = false;
			} else {

				Z502_IDLE();

				calling->STATE = PROCESS_STATE_RUNNING;
				ret = true;
			}
		} else {
			// no other processes to run
			Z502_HALT();
			ret = true;
		}
	}

	//pthread_mutex_unlock(&scheduler_mutex);
	return ret;

}

bool scheduler_t::sleep(PCB* p, int time, int* err) {
	// check if valid process transition
	if (p->STATE != PROCESS_STATE_READY && p->STATE != PROCESS_STATE_RUNNING && p->STATE != PROCESS_STATE_SLEEPING) {
		// bogus call process should be ready to sleep
		printf("USER_ERROR (scheduler->sleep): process(%d:%s) should be in ready or running or sleeping not (%d) status  to sleep!\n", p->ID, p->PROCESS_NAME, p->STATE);
		*err = USER_ERROR_INVALID_STATE_TRANSITION;
		return false;
	}

	// validate priority for bogus calls
	if (!this->validate_priority(p->PRIORITY)) {
		// invalid process priority, corrupted data was given
		printf("USER_ERROR (scheduler->sleep): invalid process(%d,%s) priority given(%d)! \n", p->ID, p->PROCESS_NAME, p->PRIORITY);
		*err = USER_ERROR_CORRUPTED_PROCESS;
		return false;
	}

	// remove from ready queue
	this->remove_from_ready(p, false);

	alarmable* wakeup_alarm = new alarmable(time, _wakeup, p);
	alarm_manager.add_alarm(wakeup_alarm);

	// update process state
	p->STATE = PROCESS_STATE_SLEEPING;
	// no error occured
	*err = 0;
	this->schedule(p, true, wake_up_time, wakeup_alarm, false);
	// moved to sleeping queue successfully
	return true;
}

bool scheduler_t::resume(PCB*p, int* err) {
	// check if valid process transition
	if (p->STATE != PROCESS_STATE_SUSPENDED) {
		// bogus call process should be ready to sleep
		printf("USER_ERROR (scheduler->resume): process should be in suspended to resume!\n");
		*err = USER_ERROR_INVALID_STATE_TRANSITION;
		return false;
	}

	// try to remove from suspended
	if (!this->suspended.remove(p)) {
		// bogus call process doesn't exist in suspended queue
		printf("USER_ERROR (scheduler->resume): process wasn't found in suspended queue! \n");
		*err = USER_ERROR_CORRUPTED_PROCESS;
		return false;
	}

	// validate priority for bogus calls
	if (!this->validate_priority(p->PRIORITY)) {
		// invalid process priority, corrupted data was given
		printf("USER_ERROR (scheduler->resume): invalid process priority given! \n");
		*err = USER_ERROR_CORRUPTED_PROCESS;
		return false;
	}

	// insert  or into ready queue
	this->add_to_ready(p);

	if (!this->validate_priority(p->PRIORITY)) {
		// invalid process priority, corrupted data was given
		printf("USER_ERROR (scheduler->resume): invalid process priority given! \n");
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

	if (!this->validate_priority(p->PRIORITY)) {
		// invalid process priority, corrPCB* pupted data was given
		printf("USER_ERROR (scheduler->resume): invalid process priority given! \n");
		*err = USER_ERROR_CORRUPTED_PROCESS;
		return false;
	}

	if (!this->validate_priority(new_priority)) {
		// invalid process priority, corrupted data was given
		printf("USER_ERROR (scheduler->resume): invalid process new priority given! \n");
		*err = USER_ERROR_INVALID_ARGUMENT;
		return false;
	}

	// the only case we need to change its place when ready
	if (p->STATE == PROCESS_STATE_READY) {
		this->remove_from_ready(p, true);
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
		if (!this->validate_priority(p->PRIORITY)) {
			// invalid process priority, corrupted data was given
			printf("USER_ERROR (scheduler->resume): invalid process priority given! \n");
			*err = USER_ERROR_CORRUPTED_PROCESS;
			return false;
		}
		// try to remove from ready queue
		if (!this->remove_from_ready(p, true)) {
			printf("USER_ERROR (scheduler->suspend): process isn't in ready queue! \n");
			*err = USER_ERROR_CORRUPTED_PROCESS;
			return false;
		}
		break;
	case PROCESS_STATE_SLEEPING:

		// remove alarm
		if (!alarm_manager.remove(p)) {
			printf("USER_ERROR (scheduler->suspend): process wasn't find in alarm!\n");
			return false;
		}
		break;

	case PROCESS_STATE_BLOCKED:
		// can't do that! return error
		printf("USER_ERROR (scheduler->suspend): can't suspend process waiting for even!\n");
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

	this->schedule(p, true, NULL, NULL, true);
	// success
	return true;
}

bool scheduler_t::terminate(PCB* p, int* err) {
	switch (p->STATE) {
	case PROCESS_STATE_READY:
		// check if valid priority
		if (!this->validate_priority(p->PRIORITY)) {
			printf("USER_ERROR (scheduler->terminate): Invalid priority of process recieved! \n");
			*err = USER_ERROR_CORRUPTED_PROCESS;
			return false;
		}
		if (!this->remove_from_ready(p, true)) {
			printf("USER_ERROR (scheduler->terminate): process wasn't found in ready queue as expected \n");
			*err = USER_ERROR_CORRUPTED_PROCESS;
			return false;
		}
		break;

	case PROCESS_STATE_BLOCKED:
		if (!this->blocked.remove(p)) {
			printf("USER_ERROR (scheduler->terminate): process wasn't found in blocked queue");
			*err = USER_ERROR_CORRUPTED_PROCESS;
			return false;
		}
		break;
	case PROCESS_STATE_SLEEPING:
		// remove alarm
		if (!alarm_manager.remove(p)) {
			printf("USER_ERROR (scheduler->terminate): process wasn't found in sleeping queue!\n");
			return false;
		}

		break;
	case PROCESS_STATE_SUSPENDED:
		if (!this->suspended.remove(p)) {
			printf("USER_ERROR (scheduler->terminate): process wasn't found in suspended queue\n");
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

	this->schedule(p, false, NULL, NULL, false);

	return true;
}

bool scheduler_t::add_to_ready(PCB * p) {
	//alarm_manager.add_alarm(alarmable(STARVING_INTERVAL, _feed, p));
	this->ready[p->PRIORITY].enqueue(p);
	return true;
}

bool scheduler_t::remove_from_ready(PCB * p, bool existed) {

	return (this->ready[p->PRIORITY].remove(p));
}

bool scheduler_t::validate_priority(int p) {
	return 0 <= p && p <= MAX_PRIORITY;
}

bool scheduler_t::create(PCB* p) {
	if (p->STATE != PROCESS_STATE_READY) {
		puts("INTERNAL_ERROR (scheduler->create): process passed wasn't ready");
		return false;
	}
	add_to_ready(p);
	return true;
}
