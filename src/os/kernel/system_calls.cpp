/*
 * system_calls.cpp
 *
 *  Created on: Dec 6, 2012
 *      Author: mahmoudel-maghraby
 */
#include 			 "../sched/sched.h"
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
extern 		scheduler_t scheduler;

INT32		BROADCAST_ID = 	-5;
PCB* 		current_process = NULL ;


INT32 		NEXT_ID = 0;
INT32 		NUM_OF_PROCESSES = 1 ;
INT32 		NUM_OF_MESSAGES  = 0 ;






void execute_system_call(int call_type)
{

	INT32 	 time;					// Time Variable for GET_TIME_OF_DAY call
	PCB* 	 new_pcb;				// Process Control Block Variable for CREATE_PROCESS call
    INT32 	 ID;					// ID Variable
    MAIL*	 new_mail;				// Mail Variable for SEND_MESSAGE call
    MAIL*	 mail;				// Mail Variable for SEND_MESSAGE call
    INT32 message_length;			// Message Length Variable
    INT32 err ;

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

	case SYSNUM_SLEEP:
		time = (INT32) Z502_ARG1.VAL;

		if(!scheduler.sleep(current_process, time, &err))
		{
			puts("INTERNAL_ERROR! (system_calls.h->execute_system_call): didn't sleep");
		}
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
//			current_process->PRIORITY =(INT32)Z502_ARG2.VAL;
			scheduler.change_priority(current_process, (INT32)Z502_ARG2.VAL, &err);
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
//			temp->PRIORITY = (INT32)Z502_ARG2.VAL;
			scheduler.change_priority(temp, (INT32)Z502_ARG2.VAL, &err);
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
		 STAGE(3): Test numer of processes
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
		new_pcb->PROCESS_NAME = (char*) Z502_ARG1.PTR ; 	// set process name
		new_pcb->STARTING_ADDRESS = Z502_ARG2.PTR; 			// set process starting address
		new_pcb->PRIORITY = Z502_ARG3.VAL; 					// set process initial priority
		NEXT_ID++; 											// update the next ID
		new_pcb->ID = NEXT_ID; 								// set process ID
		ZCALL( Z502_MAKE_CONTEXT( &new_pcb->CONTEXT, 		// make a new context
				(void *)Z502_ARG2.PTR, USER_MODE ));
		*(INT32 *)Z502_ARG4.PTR = new_pcb->ID;				// return the process ID
		*(INT32 *)Z502_ARG5.PTR = ERR_SUCCESS; 				// set error state to "SUCCESS"
		new_pcb->PARENT_ID = current_process->ID ;			// set process parent ID to current process ID
		current_process->children->push_front(new_pcb->ID); // add the new process to the children of the
														    // current process

		/*
		 STAGE(5) Insert PCB into Processes Tables
		*/

		P_TABLE_BY_NAME[new_pcb->PROCESS_NAME] = new_pcb;
		P_TABLE_BY_ID[new_pcb->ID]= new_pcb;
		scheduler.create(new_pcb);
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
		 STAGE(2): Apply termination according to ID
		 */

		 if(ID==-1)
		 {
			// terminate self
			terminate_process(current_process->ID);
			break;
		 }

		 if(ID==-2)
		 {
			// terminate self and all descendants
			terminate_descendants(current_process->ID);
			break;
		 }

		 ID_TABLE_IT = P_TABLE_BY_ID.find(ID);
		 if(ID_TABLE_IT == P_TABLE_BY_ID.end())						// ID not found
		 {
			*(INT32 *)Z502_ARG2.PTR = ERR_INVALID_PROCESS_ID;		// set error state to INVALID_PROCESS_ID
			break;
		 }
		 else														// ID found
		 {

			 //check if the current process is an ancestor of the process to be terminated

			 PCB* temp = ID_TABLE_IT->second;
			 INT32 current_id = temp->PARENT_ID;

			 bool is_ancestor = false ;

			 while(current_id != -1)
			 {
				 if(current_id == current_process->ID)
				 {
					 is_ancestor = true ;
					 break;
				 }
				 ID_TABLE_IT = P_TABLE_BY_ID.find(current_id);
				 current_id = ID_TABLE_IT->second->PARENT_ID;
			 }

			 if(is_ancestor)
			 {
				 terminate_process(ID);
			 }
			 else
			 {
				 *(INT32 *)Z502_ARG2.PTR = ERR_UNAUTHORIZED_TERMINATION;		// set error state to UNAUTHORIZED_TERMINATION
			 }
			break;
		 }
	break;








	case SYSNUM_SEND_MESSAGE:

		/*
		 STAGE(1): Test for target ID  validity
		 */
		ID = Z502_ARG1.VAL ;
		if(ID<-1)
		{
			*(INT32 *)Z502_ARG4.PTR = ERR_INVALID_PROCESS_ID;		// set error state to INVALID_PROCESS_ID
			break;
		}

		 ID_TABLE_IT = P_TABLE_BY_ID.find(ID);
		 if(ID_TABLE_IT == P_TABLE_BY_ID.end())						// ID not found
		 {
			*(INT32 *)Z502_ARG4.PTR = ERR_INVALID_PROCESS_ID;		// set error state to INVALID_PROCESS_ID
			break;
		 }

		 /*
		 STAGE(2): Test for buffer length
		 */
		 message_length = strlen((char*)Z502_ARG2.PTR);
		 if(Z502_ARG3.VAL < message_length || Z502_ARG3.VAL > 64 )
		 {
			*(INT32 *)Z502_ARG4.PTR = ERR_BUFFER_LENGTH_EXCEEDED;		// set error state to BUFFER_LENGTH_EXCEEDED
			break;
		 }

		/*
		 STAGE(3): Test number of processes
		*/

		if(NUM_OF_MESSAGES > MAX_NUM_OF_MESSAGES)
		{
			*(INT32 *)Z502_ARG4.PTR = ERR_MAX_NUM_EXCEEDED;			// set error state to INVALID_PROCESS_PRIORITY
			break;
		}
		else
		{
			NUM_OF_MESSAGES++;
		}

		 /*
		  STAGE (4): Broadcast message if required
	     */
		 if(ID == -1)
		 {
			 new_mail = new MAIL;
			 new_mail->SENDER_ID = current_process->ID;
			 new_mail->RECEIVER_ID = BROADCAST_ID ;
			 new_mail->MESSAGE = (char*)Z502_ARG2.PTR;
			 new_mail->SENDER_BUFFER_LENGTH = Z502_ARG3.VAL ;

			 SENDER_MAIL_BOX[new_mail->SENDER_ID] = new_mail;
			 RECEIVER_MAIL_BOX[new_mail->RECEIVER_ID] = new_mail;

			 *(INT32 *)Z502_ARG4.PTR = ERR_SUCCESS;				// set error state to SUCCESS
			 break;
		 }


		 /*
		 STAGE(5): Send message to mail box
		 */

		 new_mail = new MAIL;
		 new_mail->SENDER_ID = current_process->ID;
		 new_mail->RECEIVER_ID = Z502_ARG1.VAL ;
		 new_mail->MESSAGE = (char*)Z502_ARG2.PTR;
		 new_mail->SENDER_BUFFER_LENGTH = Z502_ARG3.VAL ;

		 SENDER_MAIL_BOX[new_mail->SENDER_ID] = new_mail;
		 RECEIVER_MAIL_BOX[new_mail->RECEIVER_ID] = new_mail;

		*(INT32 *)Z502_ARG4.PTR = ERR_SUCCESS;				// set error state to SUCCESS

		break;



	case SYSNUM_RECEIVE_MESSAGE:
		/*
		 STAGE(1): Test for target ID  validity
		*/

		ID = Z502_ARG1.VAL ;
		if(ID<-1)
		{
			*(INT32 *)Z502_ARG6.PTR = ERR_INVALID_PROCESS_ID;		// set error state to INVALID_PROCESS_ID
			break;
		}


		/*
		 STAGE(2): Handle broadcast if required
		*/

		if(ID==-1)
		{
			RECEIVER_MAIL_BOX_IT = RECEIVER_MAIL_BOX.find(current_process->ID);
			if(RECEIVER_MAIL_BOX_IT == RECEIVER_MAIL_BOX.end())						// no one left a message
			{
				RECEIVER_MAIL_BOX_IT = RECEIVER_MAIL_BOX.find(BROADCAST_ID);
				if(RECEIVER_MAIL_BOX_IT == RECEIVER_MAIL_BOX.end())					// no one left a message yet
				{
				// suspend process till it receives the message
				break;
				}
			}
			mail = RECEIVER_MAIL_BOX_IT->second;
			//check for buffer length
			message_length = strlen(mail->MESSAGE);
			if(message_length>Z502_ARG3.VAL || Z502_ARG3.VAL > 64)
			{
				*(INT32 *)Z502_ARG6.PTR = ERR_BUFFER_LENGTH_EXCEEDED;		// set error state to BUFFER_LENGTH_EXCEEDED
				break;
			}
			strcpy((char*)Z502_ARG2.PTR,mail->MESSAGE) ;
			*(INT32 *)Z502_ARG4.PTR = mail->SENDER_BUFFER_LENGTH;
			*(INT32 *)Z502_ARG5.PTR = mail->SENDER_ID;
			SENDER_MAIL_BOX.erase(mail->SENDER_ID);
			RECEIVER_MAIL_BOX.erase(mail->RECEIVER_ID);
			delete mail;
			*(INT32 *)Z502_ARG6.PTR = ERR_SUCCESS;							// set error state to SUCCESS
			NUM_OF_MESSAGES--;
			break;
		}

		/*
		 STAGE(3): Check if the source sent the message
		*/

		ID_TABLE_IT = P_TABLE_BY_ID.find(ID);
		if(ID_TABLE_IT == P_TABLE_BY_ID.end())						// ID not found
		{
			*(INT32 *)Z502_ARG6.PTR = ERR_INVALID_PROCESS_ID;		// set error state to INVALID_PROCESS_ID
			break;
		}


		SENDER_MAIL_BOX_IT = SENDER_MAIL_BOX.find(ID);
		if(SENDER_MAIL_BOX_IT == SENDER_MAIL_BOX.end())						// Source didn't leave a message
		{
			// suspend process till it receives the message
			break;
		}

		 mail = SENDER_MAIL_BOX_IT->second;

		//check for buffer length

		message_length = strlen(mail->MESSAGE);
		if(message_length>Z502_ARG3.VAL || Z502_ARG3.VAL >64)
		{
			*(INT32 *)Z502_ARG6.PTR = ERR_BUFFER_LENGTH_EXCEEDED;		// set error state to BUFFER_LENGTH_EXCEEDED
			break;
		}

		strcpy((char*)Z502_ARG2.PTR,mail->MESSAGE) ;
		SENDER_MAIL_BOX.erase(mail->SENDER_ID);
		RECEIVER_MAIL_BOX.erase(mail->RECEIVER_ID);
		delete mail;
		*(INT32 *)Z502_ARG6.PTR = ERR_SUCCESS;							// set error state to SUCCESS
		NUM_OF_MESSAGES--;
		break;

	default:
		// if bogus call, report error
		printf("** ERROR! call_type in svc is undefined call_type = %d\n", call_type);
		break;
	}

}

/*
 Method to create the initial (root) process PCB
 */

void create_root_process(void* starting_address, void* context)
{
	    PCB* root_process = new PCB;
		root_process->PROCESS_NAME = (char*)"root_process" ; 			// set process name
		root_process->STARTING_ADDRESS = (void *)starting_address;		// set process starting address
		root_process->PRIORITY = 1; 									// set process initial priority
		root_process->ID = 0;		 									// set process ID
		root_process->PARENT_ID = -1;									// set process parent ID to current process ID
		root_process->CONTEXT = context ;

		P_TABLE_BY_NAME[root_process->PROCESS_NAME] = root_process;
		P_TABLE_BY_ID[root_process->ID]= root_process;

		current_process = root_process ;

}

/*
 Method to terminate a process by it's ID
 */

void terminate_process(INT32 ID)
{
	INT32 err ;
	if(ID == 0)
	{
		ZCALL(Z502_HALT());
	}
	else
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
			delete temp->children;
			delete temp;
			*(INT32 *)Z502_ARG2.PTR = ERR_SUCCESS;		// set error state to SUCCESS
			NUM_OF_PROCESSES--;
			scheduler.terminate(temp, &err);
		}

	}


}

/*
 Method to terminate a process with all of its descendants
 */


void terminate_descendants(INT32 ID)
{
	ID_TABLE_IT = P_TABLE_BY_ID.find(ID);
	INT32 child_id ;

	if(ID_TABLE_IT != P_TABLE_BY_ID.end())			// ID found
	{
		PCB* temp = ID_TABLE_IT->second;
		std::list<int>::iterator iterator;

		// terminate all children processes
		for (iterator = temp->children->begin(); iterator != temp->children->end(); ++iterator)
		{
		  child_id = (INT32)*iterator;
		  terminate_descendants(child_id);
		}
		terminate_process(ID);						// terminate self
	}
}




