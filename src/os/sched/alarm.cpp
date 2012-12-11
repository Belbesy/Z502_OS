
#include "alarmman.h"

#include "global.h"
#include "syscalls.h"
#include "protos.h"
#include "z502.h"


#include <stdlib.h>
#include <pthread.h>


#define TIMER_INTERVAL  1// in milliseconds
alarmable * turn_process;


void z502_timer_set(int interval) {


	int get_status, time;
	bool done = false;

	MEM_READ(Z502TimerStatus, &get_status);
	while (!done) {
		switch (get_status) {
		case DEVICE_IN_USE:
			// this shouldn't happen, this method is the only user of the timer
		//	printf("SYSTEM INTERNAL ERROR (alarm.c) : accessing timer while in use!\n");
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

}

alarmable::alarmable(long t, alarm_handler cb, void * r) {
	time = t;
	call_back = cb;
	r = ref;
}

alarm_manager_t::alarm_manager_t() {

}

void alarm_manager_t::init() {

}
void alarm_manager_t::add_alarm(alarmable* alarm) {
	if (alarms.size()) {

		alarms.push_back(alarm);
	} else {
		turn_process = alarm;
		z502_timer_set(alarm->time);
	}
}
bool alarm_manager_t::remove(void * given_ref) {
	for (It i = alarms.begin(); i != alarms.end(); i++) {
		if ((*i)->ref == given_ref) {
			alarms.erase(i);
			return true;
		}
	}
	return false;
}

void alarm_manager_t::alarm_handler(int device, int status) {

	switch (status) {
	case ERR_SUCCESS:
		turn_process->call_back(turn_process->ref);
		free(turn_process);

		if (alarms.size()) {
			turn_process = alarms.front();
			alarms.pop_front();

			z502_timer_set(turn_process->time);
		}

		break;
	case ERR_BAD_PARAM:
		// invalid parameter sent < 0, shouldn't happen
		printf("SYSTEM INTERNAL ERROR (alarm.c) : invalid parameter sent retrying..\n");

		break;
	default:
		// bogus status received
		printf("SYSTEM INTERNPARENTAL ERROR (alarm.c) :  bogus timer status recieved, terminating..\n");
		break;
	}

}

/*  End of z502_timer_set */

