#include "global.h"
#include "syscalls.h"
#include "protos.h"
#define 		DEBUG_OS
#define			LOG_OS

#include "system_structs.h"
#include "scheduler.h"
#include "alarmman.h"
#include "process_queue.h"

#include "syscalls.h"
#include "z502.h"
#include "protos.h"
#include "const.h"

#include <cstdio>
#include <pthread.h>
#include <list>
#include <queue>
#include <map>

extern alarm_manager_t alarm_manager;
extern scheduler_t scheduler;
extern PCB* current_process;

PCB* last_used;

char process_state[10][50] = { "CREATED", "READY", "SLEEPING", "BLOCKED", "SUSPENDED", "RUNNING", "ZOMBIE" };

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
	PCB * next = NULL;
	INT32 lock_success;
	INT32 success;

	int starving_size;
	int ready_size;

	ZCALL(Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 1, TRUE, &success));

	CALL(ready.size(&ready_size));
	CALL(starving.size(&starving_size));
#ifdef LOG_OS
	printf("\t=> Rescheduling on process {id: %d, name:%s} with state (%d,%s). ready queue size(%d)\n", p->ID, p->PROCESS_NAME, p->STATE, process_state[p->STATE], ready_size);
#endif

	switch (p->STATE) {

	case PROCESS_STATE_READY:
	case PROCESS_STATE_CREATED:
	case PROCESS_STATE_SUSPENDED:
	case PROCESS_STATE_BLOCKED:
	case PROCESS_STATE_SLEEPING:
	case PROCESS_STATE_ZOMBIE:

		if (starving_size) {
			CALL(starving.dequeue(&next));
		} else if (ready_size) {
			CALL(ready.dequeue(&next));
		}

		if (next) {
			current_process = next;
			current_process->STATE = PROCESS_STATE_RUNNING;

			Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &lock_success);
			Z502_SWITCH_CONTEXT(SWITCH_CONTEXT_SAVE_MODE, &next->CONTEXT);
#ifdef LOG_OS
			printf("\t\t=> => Currently running process {id: %d, name:%s}. old was {id%d, name:%s}\n", current_process->ID, current_process->PROCESS_NAME, p->ID, p->PROCESS_NAME);
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

		break;
	case PROCESS_STATE_RUNNING:
		// do nothing keep running
		ZCALL(Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &success));
		return;
	default:
#ifdef DEBUG_OS
		eprint();
		printf("\t\t=> Undefined process status with id: %d\n", p->STATE);
#endif
		exit(1);
		break;
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
	if (p == current_process)
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
		printf("\t\t=> Process {id: %d, name:%s} Slept Successfully.\n", p->ID, p->PROCESS_NAME);
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
 */

void scheduler_t::resume(PCB*p, int* err) {
	*err = 0;
// check if valid process transition
#ifdef LOG_OS
	iprint();
	printf("\t=> Calling Resume on process (%d:%s)\n", p->ID, p->PROCESS_NAME);
#endif
	switch (p->STATE) {
	case PROCESS_STATE_SUSPENDED:
		CALL(suspended.remove(p));
		CALL(ready.add(p));
		p->STATE = PROCESS_STATE_READY;

#ifdef LOG_OS
		iprint();
		int ready_size;
		CALL(ready.size(&ready_size));
		printf("\t\t=> => Process (%d:%s) is now ready , ready queue now have (%d) processes\n", p->ID, p->PROCESS_NAME, ready_size);
#endif
		break;
	case PROCESS_STATE_CREATED:
	case PROCESS_STATE_READY:
	case PROCESS_STATE_BLOCKED:
	case PROCESS_STATE_ZOMBIE:
	case PROCESS_STATE_SLEEPING:
	case PROCESS_STATE_RUNNING:
#ifdef DEBUG_OS
		eprint();
		printf("\t\t=> => Invalid process transition from %s to READY in Resume\n", process_state[p->STATE]);
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
}
// end of resume process

/*
 *
 *
 *
 *  ============================================== Change Priority ===============================================
 *
 *
 */
void scheduler_t::change_priority(PCB* p, int new_priority, int* err) {
// the only case we need to change its place when ready

	printf("========= changing Priority of (%d, %s) from %d to %d\n", p->ID, p->PROCESS_NAME, p->PRIORITY, new_priority );
	if (p->STATE == PROCESS_STATE_READY) {
		this->ready.remove(p);
		p->PRIORITY = new_priority;
		this->ready.add(p);
	} else {
		p->PRIORITY = new_priority;
	}
}
/*
 *
 *
 *
 *  ============================================== Suspend process ===============================================
 *
 *
 */
void scheduler_t::suspend(PCB* p, int* err) {
	*err = 0;
	int ready_size;
// check if valid process transition
#ifdef LOG_OS
	iprint();
	printf("\t=> Suspending process (%d:%s)\n", p->ID, p->PROCESS_NAME);
#endif
	switch (p->STATE) {
	case PROCESS_STATE_RUNNING:
		CALL(this->ready.size(&ready_size));

		if (ready_size || alarm_manager.alarms.size()) {
			CALL(suspended.enqueue(p));
			p->STATE = PROCESS_STATE_SUSPENDED;
			CALL(this->schedule(p));
			return;
		} else {
			*err = USER_ERROR_INVALID_STATE_TRANSITION;
			return;
		}

		break;
	case PROCESS_STATE_READY:
		CALL(ready.remove(p));
		CALL(suspended.enqueue(p));
		p->STATE = PROCESS_STATE_SUSPENDED;
		break;

	case PROCESS_STATE_BLOCKED:
		// don't remove from blocked queue until condition is signaled
		CALL(suspended.enqueue(p));
		p->STATE = PROCESS_STATE_SUSPENDED;
		break;
	case PROCESS_STATE_ZOMBIE:
	case PROCESS_STATE_CREATED:
	case PROCESS_STATE_SLEEPING: // to be handled
	case PROCESS_STATE_SUSPENDED:
#ifdef DEBUG_OS
		eprint();
		printf("\t\t=> => Invalid process transition from %s to SUSPENDED.\n", process_state[p->STATE]);
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
}


PCB* scheduler_t::get_waiting_for_message(INT32 SENDER_ID){
	map<int, q_item*> m = this->suspended.m;
	map<int, q_item*>::iterator it;
	for(it = m.begin(); it!= m.end(); it++)
		if(it->second->proc->waiting_for_message && it->second->proc->waiting_for == SENDER_ID )
			return it->second->proc;
	return NULL;
}
