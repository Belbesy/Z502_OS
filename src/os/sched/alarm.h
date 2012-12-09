

#include "../../global.h"

#include "../../syscalls.h"
#include "../../protos.h"

#include <cstdlib>
#include <list>

using namespace std;

#define TIMER_INTERVAL  100 // in milliseconds

typedef void (*alarm_handler)(void *);

struct alarmable {

	long time;
	alarm_handler call_back;
	void* ref;

	alarmable(long t, alarm_handler cb, void * r) {
		time = t;
		call_back = cb;
		r = ref;
	}
};

/************************************************************************
 z502_timer_set
 sets the timer with value TIMER_INTERVAL in alarm.h

 ************************************************************************/
void z502_timer_set() {
	int get_status, time;
	MEM_READ(Z502TimerStatus, &get_status);

	switch (get_status) {
	case DEVICE_IN_USE:
		// this shouldn't happen, this method is the only user of the timer
		printf(
				"SYSTEM INTERNAL ERROR (alarm.c) : accessing timer while in use!\n");
		break;
	case DEVICE_FREE:
		// device is free, this should be the case always
		// as this method is the only user for the timer.
		// reset the timer.
		// TODO : fix

		time = TIMER_INTERVAL;
		MEM_WRITE(Z502TimerStart, &time);
		break;
	default:
		// bogus status recieved
		printf(
				"SYSTEM INTERNAL ERROR (alarm.c) : bogus timer status recieved!\n");
		break;
	}

}

class alarm_manager_t {

	list<alarmable> alarms;
	typedef list<alarmable>::iterator It;
public:
	alarm_manager_t() {
		z502_timer_set();
	}
	void add_alarm(alarmable alarm) {
		alarms.push_back(alarm);
	}

	bool remove(void * given_ref){
		for (It i = alarms.begin(); i != alarms.end(); i++) {
				if(i->ref==given_ref){
					alarms.erase(i);
					return true;
				}
		}
		return false;
	}
	void alarm_handler(int device, int status) {
		switch (status) {
		case ERR_SUCCESS:

			for (It temp = alarms.begin(); temp != alarms.end();) {
				It i = temp;
				temp++;
				if (i->time > TIMER_INTERVAL)
					i->time -= TIMER_INTERVAL;
				 else{
					alarms.erase(i);
					i->call_back(i->ref);
				 }
			}

			// if set is not empty rewake the alarm
			if (!alarms.empty())
				z502_timer_set();

			break;
		case ERR_BAD_PARAM:
			// invalid parameter sent < 0, shouldn't happen
			printf(
					"SYSTEM INTERNAL ERROR (alarm.c) : invalid parameter sent retrying..\n");
			z502_timer_set(); //optional retry
			break;
		default:
			// bogus status received
			printf(
					"SYSTEM INTERNPARENTAL ERROR (alarm.c) :  bogus timer status recieved, terminating..\n");
			break;
		}
	}

	/*  End of z502_timer_set */
} alarm_manager;

