
#include "alarmman.h"

#include "global.h"
#include "syscalls.h"
#include "protos.h"
#include "z502.h"

#include <list>

#include <stdlib.h>
#include <pthread.h>


#define TIMER_INTERVAL  1// in milliseconds
alarmable * turn_process;


void z502_timer_set(int interval) {

	//INT32 success;
	//Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 1, TRUE, &success);

	int get_status, time;
	bool done = false;

	MEM_READ(Z502TimerStatus, &get_status);
	while (!done) {
		switch (get_status) {
		case DEVICE_IN_USE:
			// this shouldn't happen, this method is the only user of the timer
		printf("SYSTEM INTERNAL ERROR (alarm.c) : accessing timer while in use!\n");
			break;
		case DEVICE_FREE:
			// device is free, this should be the case always
			// as this method is the only user for the timer.
			// reset the timer.

			time = interval;
			MEM_WRITE(Z502TimerStart, &time);
			done = true;
			break;
		default:
			// bogus status recieved
			printf("SYSTEM INTERNAL ERROR (alarm.c) : bogus timer status recieved!\n");
			break;
		}
	}
//	Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &success);

}

alarmable::alarmable(long t, alarm_handler cb, PCB * r) {
	time = t;
	call_back = cb;
	ref = r;
}

alarm_manager_t::alarm_manager_t() {

}

void alarm_manager_t::init() {

}
void alarm_manager_t::add_alarm(alarmable* alarm) {
	if (!alarms.size()) {
		turn_process = alarm;
		z502_timer_set(alarm->time);
	}
	alarms.push_back(alarm);
}
bool alarm_manager_t::remove(void * given_ref) {
		for (It i = alarms.begin(); i != alarms.end(); i++) {
		if ((*i)->ref == given_ref) {
			alarms.erase(i);
			INT32 success;
			return true;
		}
	}
	return false;
}

void alarm_manager_t::alarm_handler(int device, int status) {
//	extern bool alarm_lock;
//	alarm_lock = true;
	switch (status) {
	case ERR_SUCCESS:
		turn_process->call_back(turn_process->ref);
		// this->remove(turn_process->ref);
		alarms.pop_front();
		free(turn_process);

		if (alarms.size()) {
			turn_process = alarms.front();
			z502_timer_set(turn_process->time);
		}

		break;
	case ERR_BAD_PARAM:
		// invalid parameter sent < 0, shouldn't happen
		printf("SYSTEM INTERNAL ERROR (alarm.c) : invalid parameter sent retrying..\n");

		break;
	default:
		// bogus status received
		printf("SYSTEM INTERNPARENTAL ERROR (alarm.c) :  bogus timer status received, terminating..\n");
		break;
	}

//	alarm_lock = false;
}

/*  End of z502_timer_set */

