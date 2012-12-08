#include "alarm.h"

#include "../../global.h"
#include "../../z502.h"
#include "../../syscalls.h"
#include "../../protos.h"

#include <stdlib.h>

/************************************************************************
 timer_ih
 timer interrupt handler
 TODO : search for alarmables
 ************************************************************************/

alarmable* head, *tail;

void init_alarm(void) {
	head = (alarmable *) malloc(sizeof(alarmable));
	tail = (alarmable *) malloc(sizeof(alarmable));
	head->next = tail;
	tail->prev = head;
	z502_timer_set();
}

void add_alarm(alarmable* alarm) {
	alarmable* next = head->next;
	next->prev = alarm, head->next = alarm;
	alarm->prev = head, alarm->next = next;
}

void alarm_ih(int device_id, int status) {
	alarmable* cur, *temp, *prev;
	switch (status) {
	case ERR_SUCCESS:

		// start list at the head
		cur = head->next;
		while (cur != tail) {

			// there are still time don't remove
			if (cur->time > TIMER_INTERVAL) {
				cur->time -= TIMER_INTERVAL;
				cur = cur->next;
			} else {
				// delete from list and call
				temp = cur, prev = cur->prev;
				cur = cur->next;
				prev->next = cur, cur->prev = cur;
				alarm_handler call_back = temp->alarm_h;
				// free
				free(temp);
				// call handler
				call_back();
			}
		}

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
/* End of os_interrupt_handler */

/************************************************************************
 z502_timer_set
 sets the timer with value TIMER_INTERVAL in alarm.h

 ************************************************************************/
void z502_timer_set() {
	int get_status, ss;
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

		ss = TIMER_INTERVAL;
		MEM_WRITE(Z502TimerStatus, &ss);
		break;
	default:
		// bogus status recieved
		printf(
				"SYSTEM INTERNAL ERROR (alarm.c) : bogus timer status recieved!\n");
		break;
	}

}
/*  End of z502_timer_set */

