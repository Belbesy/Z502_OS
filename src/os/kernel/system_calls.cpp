/*
 * system_calls.cpp
 *
 *  Created on: Dec 6, 2012
 *      Author: mahmoudel-maghraby
 */
#include 			 "system_calls.h"
#include			 "system_structs.h"
#include             "../../global.h"
#include             "../../z502.h"
#include             "../../syscalls.h"
#include             "../../protos.h"
#include             "../../string.h"
#include 			 <stdio.h>
#include 			 <string.h>
#include 			 <map>
#include 			 <string>
using namespace 	 std;

extern 		Z502_ARG 	Z502_ARG1;
extern 		Z502_ARG 	Z502_ARG2;
extern 		Z502_ARG 	Z502_ARG3;
extern 		Z502_ARG 	Z502_ARG4;
extern 		Z502_ARG 	Z502_ARG5;
extern 		Z502_ARG 	Z502_ARG6;
extern 		long     	Z502_REG_1;
extern 		long     	Z502_REG_2;
extern 		long     	Z502_REG_3;
extern 		long     	Z502_REG_4;
extern 		long     	Z502_REG_5;
extern 		long     	Z502_REG_6;
extern 		long     	Z502_REG_7;
extern 		long     	Z502_REG_8;
extern 		long     	Z502_REG_9;;

INT32 NEXT_ID = 0;


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
		*(INT32 *)Z502_ARG1.PTR =time;
		break;

	case SYSNUM_CREATE_PROCESS:

		printf("ana hna ya 7ywan");
		/*
		 STEP(1): Test for name validity

		if(strcpy((char*) Z502_ARG1.PTR,"")==0)
		{
			Z502_REG_9 = ERR_INVALID_PROCESS_NAME;		// set error state to INVALID_PROCESS_NAME
			break;
		}

		NAME_TABLE_IT = P_TABLE_BY_NAME.find(((string)(char *)Z502_ARG1.PTR));
		if(NAME_TABLE_IT == P_TABLE_BY_NAME.end())		// name found
		{
			Z502_REG_9 = ERR_DUPLICATE_PROCESS_NAME;		// set error state to DUPLCIATE_PROCESS_NAME
			break;
		}


		 STEP(2): Test for priority validity

		if(Z502_ARG3.VAL <= 0)
		{
			Z502_REG_9 = ERR_INVALID_PROCESS_PRIORITY;	// set error state to INVALID_PROCESS_PRIORITY
			break;
		}


		 STEP(3): Prepare a new Process Control Block

		PCB* new_pcb ; 									// create new Process Control Bloc
		new_pcb->PROCESS_NAME = (char*) Z502_ARG1.PTR ; // set process name
		new_pcb->ID = NEXT_ID; 							// set process ID
		new_pcb->STARTING_ADDRESS = Z502_ARG2.PTR; 		// set process starting address
		ZCALL( Z502_MAKE_CONTEXT( &new_pcb->CONTEXT, 	// make a new context
				(void *)Z502_ARG2.PTR, USER_MODE ));
		new_pcb->PRIORITY = Z502_ARG3.VAL; 				// set process initial priority
		Z502_REG_1 = NEXT_ID; 							// return the process ID
		NEXT_ID++; 										// update the next ID
		Z502_REG_9 = ERR_SUCCESS; 						// set error state to "SUCCESS"

		// set process parent ID
		if(current_process==NULL)
		{
			new_pcb->PARENT_ID = 0 ;						// set process parent ID to main process ID
														// (i.e. one of the tests) ,assumed to be 0,
		}
		else
		{
			new_pcb->PARENT_ID = current_process->ID ;	// set process parent ID to current process ID
		}

		current_process->children.push_back(new_pcb->ID); // add the new process to the children of the
														// current process


		 STEP(4): Insert PCB into Processes Tables

		P_TABLE_BY_NAME[new_pcb->PROCESS_NAME] = new_pcb;
		P_TABLE_BY_ID[new_pcb->ID]= new_pcb;

		break;
*/

	case SYSNUM_TERMINATE_PROCESS:
		Z502_HALT();
		break;

	default:
		// if bogus call, report error
		printf("** ERROR! call_type in svc is undefined call_type = %d\n", call_type);
		break;
	}

}


