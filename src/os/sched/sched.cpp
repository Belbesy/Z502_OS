#include "global.h"
#include "syscalls.h"
#include "protos.h"

#include "system_structs.h"
#include "scheduler.h"
#include "alarmman.h"
#include "process_queue.h"

#include "syscalls.h"
#include "z502.h"
#include "protos.h"
#include "base.h"
#include <cstdio>
#include <pthread.h>
#include <list>
#include <queue>
#include <map>
#define DEBUG_OS
#define LOG_OS
extern alarm_manager_t alarm_manager;
extern scheduler_t scheduler;
extern PCB* current_process;

PCB* last_used;

char process_state[10][50] = { "CREATED", "READY", "SLEEPING", "BLOCKED", "SUSPENDED", "RUNNING" , "ZOMBIE"};

extern bool int_lock;

void eprint() {
// TODO change text color
}

void iprint() {
// TODO change text color
}
//pthread_cond_t cond_ih;
//pthread_mutex_t mutex_ih;

bool sleeping_waiting;

void wakeup_process(void* ref) {
	scheduler.wakeup((PCB *) ref);
}

scheduler_t::scheduler_t() {

}

void scheduler_t::init() {

}
void scheduler_t::wakeup(PCB* p) {

#ifdef LOG_OS
	iprint();
	printf("\t=> Calling wake up on process (%d:%s)\n", p->ID, p->PROCESS_NAME);
#endif
	switch (p->STATE) {
	case PROCESS_STATE_SLEEPING:
		CALL(ready.add(p));
		p->STATE = PROCESS_STATE_READY;

#ifdef LOG_OS
		iprint();
		int ready_size;
		CALL(ready.size(&ready_size));
		printf("\t\t=> => Process (%d:%s) waked up , ready queue now have (%d) processes\n", p->ID, p->PROCESS_NAME, ready_size);
#endif
		break;
	case PROCESS_STATE_CREATED:
	case PROCESS_STATE_READY:
	case PROCESS_STATE_BLOCKED:
	case PROCESS_STATE_ZOMBIE:
	case PROCESS_STATE_SUSPENDED:
	case PROCESS_STATE_RUNNING:
#ifdef DEBUG_OS
		eprint();
		printf("\t\t=> => Invalid process transition from %s to READY\n", process_state[p->STATE]);
#endif
		exit(1);
		break;

	default:
#ifdef DEBUG_OS
		eprint();
		printf("\t\t=> => Undefined process state with id: %d\n", p->STATE);
#endif
		exit(1);
		break;
	}
}

void scheduler_t::schedule(PCB * p) {
	INT32 lock_success;
	if (p->STATE == PROCESS_STATE_RUNNING || p!=current_process) {
		return;
	}
	Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 1, TRUE, &lock_success);
#ifdef LOG_OS
	printf("\t=> Rescheduling on process {id: %d, name:%s} with state (%d,%s).\n", p->ID, p->PROCESS_NAME,p->STATE, process_state[p->STATE]);
#endif
	PCB * next = NULL;

	int starving_size;
	CALL(starving.size(&starving_size));
	if (starving_size) {
		CALL(starving.dequeue(&next));
	} else {
		int ready_size;
		CALL(ready.size(&ready_size));
		if (ready_size) {
			CALL(ready.dequeue(&next));
		}
	}


	if (next) {
		current_process = next;
		current_process->STATE = PROCESS_STATE_RUNNING;

		Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &lock_success);
		Z502_SWITCH_CONTEXT(SWITCH_CONTEXT_SAVE_MODE, &next->CONTEXT);
#ifdef LOG_OS
		printf("\t\t=> => Currently running process {id: %d, name:%s}.\n", current_process->ID, current_process->PROCESS_NAME);
#endif
		// unlock and leave

		return;
	} else {
		Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &lock_success);
		// unlock before calling idle to allow handler to be locked

#ifdef LOG_OS
		printf("\t\t=> => Process {id: %d, name:%s} is waiting for interrupt handler to finish.\n", current_process->ID, current_process->PROCESS_NAME);
#endif

		int_lock = true;
		Z502_IDLE();
		// request lock to wait for interrupt handler to finish
		while (int_lock) {
			; //busy wait
		}

#ifdef LOG_OS
		printf("\t\t=> => Process {id: %d, name:%s} finished waiting for interrupt handler.\n", current_process->ID, current_process->PROCESS_NAME);
#endif
		// unlock and leave

		CALL(this->schedule(p));
		return;
	}
}

/*
 *
 *
 *
 *  ===========================================  Create a Process ==========================================
 *
 *
 *
 *
 */

void scheduler_t::create(PCB* p, int *err) {
	INT32 success;
	ZCALL(Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 1, TRUE, &success));
#ifdef LOG_OS
	iprint();
	printf("\t=> Creating new process {id: %d, name:%s}.\n", p->ID, p->PROCESS_NAME);
#endif

	switch (p->STATE) {
	case PROCESS_STATE_CREATED:
		p->STATE = PROCESS_STATE_READY;
		CALL(this->ready.add(p));

#ifdef LOG_OS
		iprint();
		printf("\t\t=> => Process was successfully created\n");
#endif
		break;
	case PROCESS_STATE_READY:
	case PROCESS_STATE_SLEEPING:
	case PROCESS_STATE_BLOCKED:
	case PROCESS_STATE_ZOMBIE:
	case PROCESS_STATE_SUSPENDED:
	case PROCESS_STATE_RUNNING:

#ifdef DEBUG_OS
		eprint();
		printf("\t\t=> => Invalid process transition from %s to READY\n", process_state[p->STATE]);
#endif
		*err = USER_ERROR_INVALID_STATE_TRANSITION;
		break;

	default:
#ifdef DEBUG_OS
		eprint();
		printf("\t\t=> => Undefined process state with id: %d\n", p->STATE);
#endif
		exit(1);
		break;

	}
	ZCALL(Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &success));
}
// end of scheduler.create();

/*
 *
 *
 *
 *  ===========================================  Terminate a Process ==========================================
 *
 *
 *
 *
 */

void scheduler_t::terminate(PCB* p, int* err) {
	INT32 success;
	ZCALL(Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 1, TRUE, &success));
#ifdef LOG_OS
	iprint();
	printf("\t=> Terminating process {id: %d, name:%s}.\n", p->ID, p->PROCESS_NAME);
#endif

	switch (p->STATE) {
	case PROCESS_STATE_RUNNING:
		// valid transition skip
		break;
	case PROCESS_STATE_READY:
		CALL(this->ready.remove(p));
		break;

	case PROCESS_STATE_SLEEPING:
		CALL(alarm_manager.remove(p));
		break;
	case PROCESS_STATE_SUSPENDED:
		CALL(this->suspended.remove(p));
		break;

	case PROCESS_STATE_BLOCKED:
	case PROCESS_STATE_CREATED:
	case PROCESS_STATE_ZOMBIE:

#ifdef DEBUG_OS
		eprint();
		printf("\t\t=> => Invalid process transition from %s to ZOMBIE\n", process_state[p->STATE]);
#endif
		*err = USER_ERROR_INVALID_STATE_TRANSITION;
		ZCALL(Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &success));
		return;
	default:
#ifdef DEBUG_OS
		eprint();
		printf("\t\t=> => Undefined process state with id: %d\n", p->STATE);
#endif
		exit(1);
		return;
	}

#ifdef LOG_OS
	iprint();
	printf("\t\t=> Process {id: %d, name:%s} Successfully Terminated from %s state.\n", p->ID, p->PROCESS_NAME, process_state[p->STATE]);
#endif
	p->STATE = PROCESS_STATE_ZOMBIE;
	ZCALL(Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &success));
	CALL(this->schedule(p));
}

/*
 *
 *
 *
 *
 *
 *
 *  ===============================  Make a process sleep  ===============================
 *
 *
 *
 *
 */
void scheduler_t::sleep(PCB* p, int time, int*err) {
	*err = 0;
	INT32 success;
	ZCALL(Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 1, TRUE, &success));
#ifdef LOG_OS
	iprint();
	printf("\t=> Sleep called on Process {id: %d, name:%s}.\n", p->ID, p->PROCESS_NAME);
#endif

	// validate time
	if (time <= 0) {
#ifdef DEBUG_OS
		eprint();
		printf("\t\t=> => Invalid time given to sleep on %d\n", time);
#endif
		*err = USER_ERROR_INVALID_ARGUMENT;
	}

	switch (p->STATE) {
	case PROCESS_STATE_READY:
		this->ready.remove(p);
		break;
	case PROCESS_STATE_RUNNING:
		// do nothing correct transition
		break;
	case PROCESS_STATE_CREATED:
	case PROCESS_STATE_SLEEPING:
	case PROCESS_STATE_BLOCKED:
	case PROCESS_STATE_SUSPENDED:
	case PROCESS_STATE_ZOMBIE:
#ifdef DEBUG_OS
		eprint();
		printf("\t\t=> => Invalid process state transition from %s\n", process_state[p->STATE]);
#endif
		*err = USER_ERROR_INVALID_STATE_TRANSITION;
		break;

	default:
#ifdef DEBUG_OS
		eprint();
		printf("\t\t=> Undefined process status with id: %d\n", p->STATE);
#endif
		exit(1);
		break;
	}
	if (!(*err)) {
		p->STATE = PROCESS_STATE_SLEEPING;

		alarmable* wakeup_alarm = new alarmable(time, wakeup_process, p);
		alarm_manager.add_alarm(wakeup_alarm);
#ifdef LOG_OS
		iprint();
		printf("\t=> Process {id: %d, name:%s} Slept Successfully.\n", p->ID, p->PROCESS_NAME);
#endif
		ZCALL(Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &success));
		this->schedule(p);
		return;
	}
	ZCALL(Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &success));

}
// end of scheduler.sleep();

/*
 *
 *
 *
 *  ============================================== Resume a process ===============================================
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */

bool scheduler_t::resume(PCB*p, int* err) {
	// check if valid process transition
//	if (p->STATE != PROCESS_STATE_SUSPENDED) {
//		// bogus call process should be ready to sleep
//		printf("USER_ERROR (scheduler->resume): process should be in suspended to resume!\n");
//		*err = USER_ERROR_INVALID_STATE_TRANSITION;
//		return false;
//	}
//
//	// try to remove from suspended
//	if (!this->suspended.remove(p)) {
//		// bogus call process doesn't exist in suspended queue
//		printf("USER_ERROR (scheduler->resume): process wasn't found in suspended queue! \n");
//		*err = USER_ERROR_CORRUPTED_PROCESS;
//		return false;
//	}
//
//	// validate priority for bogus calls
//	if (!this->validate_priority(p->PRIORITY)) {
//		// invalid process priority, corrupted data was given
//		printf("USER_ERROR (scheduler->resume): invalid process priority given! \n");
//		*err = USER_ERROR_CORRUPTED_PROCESS;
//		return false;
//	}
//
//	// insert  or into ready queue
//	this->add_to_ready(p);
//
//	if (!this->validate_priority(p->PRIORITY)) {
//		// invalid process priority, corrupted data was given
//		printf("USER_ERROR (scheduler->resume): invalid process priority given! \n");
//		*err = USER_ERROR_CORRUPTED_PROCESS;
//		return false;
//	}
//
//	// update process state
//	p->STATE = PROCESS_STATE_READY;
//
//	// no error occurred
//	*err = 0;

// moved to ready queue successfully
	return true;
}
bool scheduler_t::change_priority(PCB* p, int new_priority, int* err) {

//	if (!this->validate_priority(p->PRIORITY)) {
//		// invalid process priority, corrPCB* pupted data was given
//		printf("USER_ERROR (scheduler->resume): invalid process priority given! \n");
//		*err = USER_ERROR_CORRUPTED_PROCESS;
//		return false;
//	}
//
//	if (!this->validate_priority(new_priority)) {
//		// invalid process priority, corrupted data was given
//		printf("USER_ERROR (scheduler->resume): invalid process new priority given! \n");
//		*err = USER_ERROR_INVALID_ARGUMENT;
//		return false;
//	}
//
//	// the only case we need to change its place when ready
//	if (p->STATE == PROCESS_STATE_READY) {
//		this->remove_from_ready(p, true);
//		p->PRIORITY = new_priority;
//		this->add_to_ready(p);
//	} else {
//		p->PRIORITY = new_priority;
//	}
	return true;
}
bool scheduler_t::suspend(PCB* p, int* err) {
//	switch (p->STATE) {
//	case PROCESS_STATE_READY:
//		// process is ready suspend it
//		if (!this->validate_priority(p->PRIORITY)) {
//			// invalid process priority, corrupted data was given
//			printf("USER_ERROR (scheduler->resume): invalid process priority given! \n");
//			*err = USER_ERROR_CORRUPTED_PROCESS;
//			return false;
//		}
//		// try to remove from ready queue
//		if (!this->remove_from_ready(p, true)) {
//			printf("USER_ERROR (scheduler->suspend): process isn't in ready queue! \n");
//			*err = USER_ERROR_CORRUPTED_PROCESS;
//			return false;
//		}
//		break;
//	case PROCESS_STATE_SLEEPING:
//
//		// remove alarm
//		if (!alarm_manager.remove(p)) {
//			printf("USER_ERROR (scheduler->suspend): process wasn't find in alarm!\n");
//			return false;
//		}
//		break;
//
//	case PROCESS_STATE_BLOCKED:
//		// can't do that! return error
//		printf("USER_ERROR (scheduler->suspend): can't suspend process waiting for even!\n");
//		*err = USER_ERROR_INVALID_STATE_TRANSITION;
//		return false;
//	default:
//		// can't do that! return error
//		printf("USER_ERROR (scheduler->suspend): unsupported state!\n");
//		*err = USER_ERROR_INVALID_STATE_TRANSITION;
//		return false;
//
//	}
//
//	// change process state to suspended
//	p->STATE = PROCESS_STATE_SUSPENDED;
//	// put in queue
//	this->suspended.enqueue(p);
//
//	bool success;
//	this->schedule();
	// success
	return true;
}
