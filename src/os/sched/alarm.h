
#ifndef ALARM_H
#define ALARM_H

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

	alarmable(long t, alarm_handler cb, void * r);
};

/************************************************************************
 z502_timer_set
 sets the timer with value TIMER_INTERVAL in alarm.h

 ************************************************************************/
void z502_timer_set();

class alarm_manager_t {
	list<alarmable> alarms;
	typedef list<alarmable>::iterator It;
public:
	alarm_manager_t();
	void init();
	void add_alarm(alarmable alarm);
	bool remove(void * given_ref);
	void alarm_handler(int device, int status);
	/*  End of z502_timer_set */
};

#endif
