/*
 * system_calls.cpp
 *
 *  Created on: Dec 6, 2012
 *      Author: mahmoudel-maghraby
 */
#include 			 "system_calls.h"

#include             "global.h"
#include             "z502.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"


void execute_system_call(int call_type)
{


	INT32 	 time;			// Time Variable for GET_TIME_OF_DAY call

	// Switch on system calls
	switch(call_type)
	{

	case SYSNUM_GET_TIME_OF_DAY:

		// calling the hardware function and return value to time
		ZCALL(MEM_READ(Z502ClockStatus, &time));
		// dereference the ptr and assign the returned time value
		//*(INT32 *)Z502_ARG1.PTR =time;
		break;


	case SYSNUM_TERMINATE_PROCESS:
		Z502_HALT();
		break;
	default:
		// if bogus call, report error
		printf("** ERROR! call_type in svc is undefined call_type = %d\n", call_type);
		break;
	}

}


