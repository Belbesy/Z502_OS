


#ifndef ALARM_H
#define ALARM_H

#include "z502/global.h"
#include <z502/syscalls.h>
#include <z502/protos.h>

#include <cstdlib>
#include <list>

using namespace std;

//#define TIMER_INTERVAL  5000000000000 // in milliseconds



/************************************************************************
 z502_timer_set
 sets the timer with value TIMER_INTERVAL in alarm.h

 ************************************************************************/

typedef void (*alarm_handler)(void *);

struct alarmable {

	long time;
	alarm_handler call_back;
	void* ref;

	alarmable(long t, alarm_handler cb, void * r);
};

class alarm_manager {
	list<alarmable *> alarms;
	typedef list<alarmable *>::iterator It;
public:
	alarm_manager();
	void init();
	void add_alarm(alarmable* alarm);
	bool remove(void * given_ref);
	void alarm_handler(int device, int status);
	/*  End of z502_timer_set */
};


#endif
