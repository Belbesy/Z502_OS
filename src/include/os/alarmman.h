#ifndef ALARMMAN_H
#define ALARMMAN_H


#include <stdlib.h>
#include <list>


using namespace std;
typedef void (*alarm_handler)(void *);

struct alarmable {

	long time;
	alarm_handler call_back;
	void* ref;

	alarmable(long t, alarm_handler cb, void * r);
};

class alarm_manager_t {
	std::list<alarmable *> alarms;
	typedef std::list<alarmable *>::iterator It;
public:
	alarm_manager_t();
	void init();
	void add_alarm(alarmable* alarm);
	bool remove(void * given_ref);
	void alarm_handler(int device, int status);
	/*  End of z502_timer_set */
};


#endif
