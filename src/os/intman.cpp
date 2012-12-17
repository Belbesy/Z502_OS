/*
 * Author : Belbesy
 * Created : Monday 3rd Dec 2012
 */

#include                 "intman.h"
#include                 "alarmman.h"

#include                 "global.h"
#include                 "syscalls.h"
#include                 "protos.h"

#include                 <stdio.h>
#include                 <stdlib.h>
#include                 <string.h>

extern alarm_manager_t alarm_manager;
extern bool int_lock;
// array of interrupt handlers to be changed to map if possible
//int_handler int_handlers[MAX_HANDLERS] = { alarm_ih };

/************************************************************************
 INTERRUPT_HANDLER
 When the Z502 gets a hardware interrupt, it transfers control to
 this routine in the OS.

 manages all interrupts and passes it to the responsible handler
 ************************************************************************/
void os_interrupt_handler(int device_id, int status) {

	if (device_id == TIMER_INTERRUPT)
		alarm_manager.alarm_handler(device_id, status);
	else
		puts("Unsupported device_id interrupt");
}

void os_fault_handler(int a, int b) {
	//	TODO: ..

}
/* End of os_interrupt_handler */
