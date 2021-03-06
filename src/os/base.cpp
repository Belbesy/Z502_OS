/************************************************************************

 This code forms the base of the operating system you will
 build.  It has only the barest rudiments of what you will
 eventually construct; yet it contains the interfaces that
 allow test.c and z502.c to be successfully built together.

 Revision History:
 1.0 August 1990
 1.1 December 1990: Portability attempted.
 1.3 July     1992: More Portability enhancements.
 Add call to sample_code.
 1.4 December 1992: Limit (temporarily) printout in
 interrupt handler.  More portability.
 2.0 January  2000: A number of small changes.
 2.1 May      2001: Bug fixes and clear STAT_VECTOR
 2.2 July     2002: Make code appropriate for undergrads.
 Default program start is in test0.
 3.0 August   2004: Modified to support memory mapped IO
 3.1 August   2004: hardware interrupt runs on separate thread
 3.11 August  2004: Support for OS level locking
 ************************************************************************/
#include			"system_calls.h"
#include			"alarmman.h"
#include			"scheduler.h"

#include			"intman.h"
#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"

extern char MEMORY[];
//extern BOOL          POP_THE_STACK;
extern UINT16 *Z502_PAGE_TBL_ADDR;
extern INT16 Z502_PAGE_TBL_LENGTH;
extern INT16 Z502_PROGRAM_COUNTER;
extern INT16 Z502_INTERRUPT_MASK;
extern INT32 SYS_CALL_CALL_TYPE;
extern INT16 Z502_MODE;
extern Z502_ARG Z502_ARG1;
extern Z502_ARG Z502_ARG2;
extern Z502_ARG Z502_ARG3;
extern Z502_ARG Z502_ARG4;
extern Z502_ARG Z502_ARG5;
extern Z502_ARG Z502_ARG6;

extern void *TO_VECTOR[];
extern INT32 CALLING_ARGC;
extern char **CALLING_ARGV;

char *call_names[] = { "mem_read ", "mem_write", "read_mod ", "get_time ", "sleep    ", "get_pid  ", "create   ", "term_proc", "suspend  ", "resume   ", "ch_prior ", "send     ", "receive  ", "disk_read", "disk_wrt ", "def_sh_ar" };

scheduler_t scheduler;
alarm_manager_t alarm_manager;
bool int_lock = 0;
/************************************************************************
 INTERRUPT_HANDLER
 When the Z502 gets a hardware interrupt, it transfers control to
 this routine in the OS.
 ************************************************************************/
void interrupt_handler(void) {
	INT32 success;
	ZCALL(Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 1, TRUE, &success));
	INT32 device_id;
	INT32 status;
	INT32 Index = 0;
	// Get cause of interrupt
	MEM_READ(Z502InterruptDevice, &device_id);
	// Set this device as target of our query
	MEM_WRITE(Z502InterruptDevice, &device_id);
	// Now read the status of this device
	MEM_READ(Z502InterruptStatus, &status);

	CALL(os_interrupt_handler(device_id, status));

	MEM_WRITE(Z502InterruptClear, &Index);

	ZCALL(Z502_READ_MODIFY(MEMORY_INTERLOCK_BASE, 0, TRUE, &success));
	int_lock = 0;
} /* End of interrupt_handler */
/************************************************************************
 FAULT_HANDLER
 The beginning of the OS502.  Used to receive hardware faults.
 ************************************************************************/

void fault_handler(void) {

	INT32 device_id;
	INT32 status;
	INT32 Index = 0;
//	// Get cause of interrupt
//	MEM_READ(Z502InterruptDevice, &device_id);
//	// Set this device as target of our query
//	MEM_WRITE(Z502InterruptDevice, &device_id);
//	// Now read the status of this device
//	MEM_READ(Z502InterruptStatus, &status);

	switch(device_id){
	case CPU_ERROR:
		puts("CPU_ERROR!\n");
		break;
	case PRIVILEGED_INSTRUCTION:
		puts("Executing privileged instruction!\n");
		break;
	case INVALID_MEMORY:
		puts("Invalid memory access!\n");
		break;
	}
	puts("Invalid Instruction!");
	ZCALL(Z502_HALT());
//	MEM_WRITE(Z502InterruptClear, &Index);

} /* End of fault_handler */

/************************************************************************
 SVC
 The beginning of the OS502.  Used to receive software interrupts.
 All system calls come to this point in the code and are to be
 handled by the student written code here.
 ************************************************************************/

void svc(void) {
	int call_type = SYS_CALL_CALL_TYPE;
	// Call the execute _system_call method
	CALL(execute_system_call(call_type));
} // End of svc

/************************************************************************
 OS_SWITCH_CONTEXT_COMPLETE
 The hardware, after completing a process switch, calls this routine
 to see if the OS wants to do anything before starting the user
 process.
 ************************************************************************/

void os_switch_context_complete(void) {
	static INT16 do_print = TRUE;

	if (do_print == TRUE) {
		printf("os_switch_context_complete  called before user code.\n");
		do_print = FALSE;
	}
} /* End of os_switch_context_complete */

/************************************************************************
 OS_INIT
 This is the first routine called after the simulation begins.  This
 is equivalent to boot code.  All the initial OS components can be
 defined and initialized here.
 ************************************************************************/

void os_init(void) {
	void *next_context;
	INT32 i;

	/* Demonstrates how calling arguments are passed thru to here       */

	printf("Program called with %d arguments:", CALLING_ARGC);
	for (i = 0; i < CALLING_ARGC; i++)
		printf(" %s", CALLING_ARGV[i]);
	printf("\n");
	printf("Calling with argument 'sample' executes the sample program.\n");

	/*          Setup so handlers will come to code in base.c           */

	TO_VECTOR[TO_VECTOR_INT_HANDLER_ADDR] = (void *) interrupt_handler;
	TO_VECTOR[TO_VECTOR_FAULT_HANDLER_ADDR] = (void *) fault_handler;
	TO_VECTOR[TO_VECTOR_TRAP_HANDLER_ADDR] = (void *) svc;

	/*  Determine if the switch was set, and if so go to demo routine.  */

	if ((CALLING_ARGC > 1) && (strcmp(CALLING_ARGV[1], "sample") == 0)) {
		ZCALL( Z502_MAKE_CONTEXT( &next_context, (void *)sample_code, KERNEL_MODE ));
		ZCALL( Z502_SWITCH_CONTEXT( SWITCH_CONTEXT_KILL_MODE, &next_context ));
	} /* This routine should never return!!           */

	/*  This should be done by a "os_make_process" routine, so that
	 test0 runs on a process recognized by the operating system.    */

	scheduler.init();
	alarm_manager.init();

	ZCALL( Z502_MAKE_CONTEXT( &next_context, (void *)test1j, USER_MODE ));

	CALL(create_root_process((void *)test1j, next_context));

	ZCALL( Z502_SWITCH_CONTEXT( SWITCH_CONTEXT_KILL_MODE, &next_context ));

} /* End of os_init       */
