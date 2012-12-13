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

INT32 lock;
extern alarm_manager_t alarm_manager;

// array of interrupt handlers to be changed to map if possible
int_handler int_handlers[MAX_HANDLERS] = { alarm_ih };

/************************************************************************
 INTERRUPT_HANDLER
 When the Z502 gets a hardware interrupt, it transfers control to
 this routine in the OS.

 manages all interrupts and passes it to the responsible handler
 ************************************************************************/
void os_interrupt_handler(int device_id, int status) {
	lock = true;
	INT32 result;
	//Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 1, TRUE, &result);
	INT32 Index = 0;

	if (device_id == TIMER_INTERRUPT)
		alarm_ih(0, status);
	else
		puts("Unsupported device_id interrupt");
	lock = false;
	//Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &result);

}

void os_fault_handler(int a, int b) {

}
/* End of os_interrupt_handler */

void alarm_ih(int device, int status) {
	alarm_manager.alarm_handler(device, status);
}
