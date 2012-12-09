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
#include 			 <cstdlib>
using namespace 	 std;

extern 		Z502_ARG 	Z502_ARG1;
extern 		Z502_ARG 	Z502_ARG2;
extern 		Z502_ARG 	Z502_ARG3;
extern 		Z502_ARG 	Z502_ARG4;
extern 		Z502_ARG 	Z502_ARG5;
extern 		Z502_ARG 	Z502_ARG6;


INT32 NEXT_ID = 0;
INT32 NUM_OF_PROCESSES = 1 ;

map <string, PCB*> P_TABLE_BY_NAME;
map <INT32, PCB*> P_TABLE_BY_ID;

map<string,PCB*>::iterator NAME_TABLE_IT ;
map<INT32,PCB*>::iterator ID_TABLE_IT ;

PCB* current_process = NULL ;


void execute_system_call(int call_type)
{

	INT32 	 time;					// Time Variable for GET_TIME_OF_DAY call
	PCB* 	 new_pcb;				// Process Control Block Variable for CREATE_PROCESS call
    INT32 	 ID;					// ID Variable for TERMINATE_PROCESS call
    INT32    CHILD_ID;
	// Switch on system calls
	switch(call_type)
	{

	case SYSNUM_GET_TIME_OF_DAY:

		/*
		 STAGE(1) : calling the hardware function and return value to time
		*/

		ZCALL(MEM_READ(Z502ClockStatus, &time));

		/*
		 STAGE(2) : Assign the returned time value
		*/

		*(INT32 *)Z502_ARG1.PTR =time;
		break;

	case SYSNUM_GET_PROCESS_ID:

		/*
		 STAGE(1): Test name parameter
		*/

		if(strcmp((char*) Z502_ARG1.PTR,"")==0)				// get current process ID
		{
			if(current_process == NULL)						// root process (i.e. one of the tests processes)
			{
				*(INT32 *)Z502_ARG2.PTR = 0;
			}
			else											// not root process
			{
				*(INT32 *)Z502_ARG2.PTR = current_process->ID;
			}
			*(INT32 *)Z502_ARG3.PTR = ERR_SUCCESS;			// set error state to SUCCESS
			break;
		}

		/*
		 STAGE(2): Search for process by name
		 */

		NAME_TABLE_IT = P_TABLE_BY_NAME.find(((string)(char *)Z502_ARG1.PTR));

		if(NAME_TABLE_IT != P_TABLE_BY_NAME.end())					// name found
		{
			*(INT32 *)Z502_ARG2.PTR = NAME_TABLE_IT->second->ID;
			*(INT32 *)Z502_ARG3.PTR = ERR_SUCCESS;					// set error state to SUCCESS
		}
		else														// name not found
		{
			*(INT32 *)Z502_ARG3.PTR = ERR_INVALID_PROCESS_NAME;		// set error state to INVALID_PROCESS_NAME
		}

		break;

	case SYSNUM_CHANGE_PRIORITY:

		ID = (INT32)  Z502_ARG1.VAL;

		/*
		 STAGE(1): Test for id validity
		*/

		 if(ID<-1)
		 {
			*(INT32 *)Z502_ARG3.PTR = ERR_INVALID_PROCESS_ID;		// set error state to INVALID_PROCESS_ID
			break;
		 }

		 /*
		 STAGE(2): change current process priority
		 */

		 if(ID==-1)
		 {
			 if(current_process==NULL)
			 {
				 // change root process pro=iority
			 }
			 else
			 {
				current_process->PRIORITY =(INT32)Z502_ARG2.VAL;
			 }
			break;
		 }

		 /*
		  STAGE(2): change process priority by ID
		 */

		 ID_TABLE_IT = P_TABLE_BY_ID.find(ID);
		 if(ID_TABLE_IT == P_TABLE_BY_ID.end())						// ID not found
		 {
			*(INT32 *)Z502_ARG3.PTR = ERR_INVALID_PROCESS_ID;		// set error state to INVALID_PROCESS_ID
		 }
		 else														// ID found
		 {
			PCB* temp = ID_TABLE_IT->second;
			temp->PRIORITY = (INT32)Z502_ARG2.VAL;
		 }

	break;


	case SYSNUM_CREATE_PROCESS:

		/*
		 STAGE(1): Test for name validity
		*/

		if(strcmp((char*) Z502_ARG1.PTR,"")==0)
		{
			*(INT32 *)Z502_ARG5.PTR = ERR_INVALID_PROCESS_NAME;		// set error state to INVALID_PROCESS_NAME
			break;
		}

		NAME_TABLE_IT = P_TABLE_BY_NAME.find(((string)(char *)Z502_ARG1.PTR));
		if(NAME_TABLE_IT != P_TABLE_BY_NAME.end())					// name found
		{
			*(INT32 *)Z502_ARG5.PTR = ERR_DUPLICATE_PROCESS_NAME;	// set error state to DUPLCIATE_PROCESS_NAME
			break;
		}

		/*
		 STAGE(2): Test for priority validity
		*/

		if(Z502_ARG3.VAL <= 0)
		{
			*(INT32 *)Z502_ARG5.PTR = ERR_INVALID_PROCESS_PRIORITY;	// set error state to INVALID_PROCESS_PRIORITY
			break;
		}

		/*
		 STAGE(3): Test nUMBER of processes
		*/

		if(NUM_OF_PROCESSES > MAX_NUM_OF_PROCESSES)
		{
			*(INT32 *)Z502_ARG5.PTR = ERR_MAX_NUM_EXCEEDED;			// set error state to INVALID_PROCESS_PRIORITY
			break;
		}
		else
		{
			NUM_OF_PROCESSES++;
		}

		/*
		 STAGE(4):Prepare a new Process Control Block
		*/


		new_pcb = new PCB;
		new_pcb->PROCESS_NAME = (char*) Z502_ARG1.PTR ; // set process name
		new_pcb->STARTING_ADDRESS = Z502_ARG2.PTR; 		// set process starting address
		new_pcb->PRIORITY = Z502_ARG3.VAL; 				// set process initial priority
		NEXT_ID++; 										// update the next ID
		new_pcb->ID = NEXT_ID; 							// set process ID
		ZCALL( Z502_MAKE_CONTEXT( &new_pcb->CONTEXT, 	// make a new context
				(void *)Z502_ARG2.PTR, USER_MODE ));
		*(INT32 *)Z502_ARG4.PTR = new_pcb->ID;			// return the process ID
		*(INT32 *)Z502_ARG5.PTR = ERR_SUCCESS; 			// set error state to "SUCCESS"

		/*
		STAGE(5) set process parent ID
		*/

		if(current_process==NULL)
		{
			new_pcb->PARENT_ID = 0 ;					// set process parent ID to main process ID
														// (i.e. one of the tests) ,assumed to be 0,
		}
		else
		{
			new_pcb->PARENT_ID = current_process->ID ;	// set process parent ID to current process ID
		}

		if(current_process!=NULL)
		{
			current_process->children->push_back(new_pcb->ID); // add the new process to the children of the
														   	   // current process
		}

		/*
		 STAGE(6) Insert PCB into Processes Tables
		*/

		P_TABLE_BY_NAME[new_pcb->PROCESS_NAME] = new_pcb;
		P_TABLE_BY_ID[new_pcb->ID]= new_pcb;

		break;



	case SYSNUM_TERMINATE_PROCESS:

		 ID = (INT32)  Z502_ARG1.VAL;

		/*
			 STAGE(1): Test for id validity
		*/

		 if(ID<-2)
		 {
			*(INT32 *)Z502_ARG2.PTR = ERR_INVALID_PROCESS_ID;		// set error state to INVALID_PROCESS_ID
			break;
		 }

		 /*
		 STAGE(2): Terminate current process
		 */

		 if(ID==-1)
		 {
			 if(current_process==NULL)
			 {
				 ZCALL(Z502_HALT());
			 }
			 else
			 {
				ID = current_process->ID;
				terminate_process(ID);
			 }
			break;
		 }

		 /*
		 STAGE(3): Terminate current process and all children processes
		 */

		 if(ID==-2)
		 {
			 if(current_process==NULL)
			 {
				 ZCALL(Z502_HALT());
			 }
			 else
			 {
	 /*
	  *********************************************************************
	  *																	  *
	  * 				DELETE ALL CHILDREN PROCESSES OF  				  *
	  * 						CURRENT PROCESS							  *
	  * 																  *
	  *********************************************************************
	  */
				ID = current_process->ID;
				ID_TABLE_IT = P_TABLE_BY_ID.find(ID);

				if(ID_TABLE_IT != P_TABLE_BY_ID.end())			// ID found
				{
					PCB* temp = ID_TABLE_IT->second;
					std::list<int>::iterator iterator;

					// terminate all children processes
					for (iterator = temp->children->begin(); iterator != temp->children->end(); ++iterator)
					{
					  CHILD_ID = (INT32)*iterator;
					  terminate_process(CHILD_ID);
					}
					terminate_process(ID);						// terminate self
				}
			 }
			break;
		 }

		 /*
			 STAGE(4): Terminate A process By ID
		 */

		 ID_TABLE_IT = P_TABLE_BY_ID.find(ID);
		 if(ID_TABLE_IT == P_TABLE_BY_ID.end())						// ID not found
		 {
			*(INT32 *)Z502_ARG2.PTR = ERR_INVALID_PROCESS_ID;		// set error state to INVALID_PROCESS_ID
			break;
		 }
		 else														// ID found
		 {
	 /*
	  *********************************************************************
	  *																	  *
	  * CHECK IF THE PROCESS GIVEN BY THAT ID IS IN THE HIERARCHY OF THE  *
	  * 						CURRENT PROCESS							  *
	  * 																  *
	  *********************************************************************
	  */
			 PCB* temp = ID_TABLE_IT->second;
			 if((current_process!=NULL && current_process->ID == temp->PARENT_ID)||(current_process == NULL && temp->PARENT_ID == 0))
			 {
				 terminate_process(ID);
			 }
			 else
			 {
				 *(INT32 *)Z502_ARG2.PTR = ERR_UNAUTHORIZED_TERMINATION;		// set error state to SUCCESS
			 }
			break;
		 }
	break;

	default:
		// if bogus call, report error
		printf("** ERROR! call_type in svc is undefined call_type = %d\n", call_type);
		break;
	}

}

void terminate_process(INT32 ID)
{
	ID_TABLE_IT = P_TABLE_BY_ID.find(ID);
	if(ID_TABLE_IT != P_TABLE_BY_ID.end())			// ID found
	{
		PCB* temp = ID_TABLE_IT->second;

		// Delete PCB from first table
		P_TABLE_BY_NAME.erase(temp->PROCESS_NAME);

		// Delete PCB from second table
		P_TABLE_BY_ID.erase(temp->ID);

		// Destroy Process Context
		ZCALL(Z502_DESTROY_CONTEXT(&(temp->CONTEXT)));

		// Free all PCB Data
//					delete (Z502CONTEXT *)temp->CONTEXT;
//					delete temp->PROCESS_NAME;
//					delete temp->STARTING_ADDRESS;
//					delete temp->children;
		delete temp;
		*(INT32 *)Z502_ARG2.PTR = ERR_SUCCESS;		// set error state to SUCCESS
		NUM_OF_PROCESSES--;
	}

}





