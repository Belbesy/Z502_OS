/*
 * system_calls.cpp
 *
 *  Created on: Dec 6, 2012
 *      Author: mahmoudel-maghraby
 */
#include			"const.h"
#include 			"alarmman.h"
#include 			 "scheduler.h"
#include 			 "system_calls.h"
#include			 "system_structs.h"
#include             "global.h"
#include             "z502.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "global.h"

#include             <string.h>
#include 			 <stdio.h>
#include 			 <string.h>
#include 			 <map>
#include 			 <string>
#include 			 <cstdlib>
using namespace std;

extern Z502_ARG Z502_ARG1;
extern Z502_ARG Z502_ARG2;
extern Z502_ARG Z502_ARG3;
extern Z502_ARG Z502_ARG4;
extern Z502_ARG Z502_ARG5;
extern Z502_ARG Z502_ARG6;

extern scheduler_t scheduler;
extern INT16 Z502_MODE;
// REMOVED ITERATORS, CAUSED a BUG in RECURSIVE CALLS

map<string, PCB*> P_TABLE_BY_NAME;
map<INT32, PCB*> P_TABLE_BY_ID;

map<INT32, MAIL*> SENDER_MAIL_BOX;
map<INT32, MAIL*> RECEIVER_MAIL_BOX;

INT32 BROADCAST_ID = -5;
PCB* current_process = NULL;

INT32 NEXT_ID = 0;
INT32 NUM_OF_PROCESSES = 1;
INT32 NUM_OF_MESSAGES = 0;

void sys_suspend_process() {
	int err;
	INT32 id = Z502_ARG1.VAL;

	PCB * target;
	if (id == -1) {
		if (current_process->ID == 0) {

			*(INT32 *) Z502_ARG2.PTR = ERR_INVALID_STATE_TRANSITION;
			return;
		} else {
			// suspend self
			target = current_process;
		}
	} else {
		map<INT32, PCB*>::iterator it = P_TABLE_BY_ID.find(id);
		if (it == P_TABLE_BY_ID.end()) {
			//invalid process id given
			*(INT32 *) Z502_ARG2.PTR = ERR_INVALID_PROCESS_ID;
			return;
		} else {
			target = it->second;
		}
	}

	scheduler.suspend(target, &err);
	if (err) {
		*(INT32 *) Z502_ARG2.PTR = ERR_INVALID_STATE_TRANSITION;
	} else {
		*(INT32 *) Z502_ARG2.PTR = ERR_SUCCESS;
	}
}

void sys_resume_process() {
	int err;
	INT32 id = Z502_ARG1.VAL;

	PCB * target;

	map<INT32, PCB*>::iterator it = P_TABLE_BY_ID.find(id);
	if (it == P_TABLE_BY_ID.end()) {
		//invalid process id given
		*(INT32 *) Z502_ARG2.PTR = ERR_INVALID_PROCESS_ID;
		return;
	} else {
		target = it->second;
	}
	scheduler.resume(target, &err);
	if (err) {
		*(INT32 *) Z502_ARG2.PTR = ERR_INVALID_STATE_TRANSITION;
	} else {
		*(INT32 *) Z502_ARG2.PTR = ERR_SUCCESS;
	}
}

void sys_change_priority() {
	int err;
	INT32 ID = (INT32) Z502_ARG1.VAL;
	PCB* target;
	// STAGE(1): Test for id validity
	if (ID < -1) {
		*(INT32 *) Z502_ARG3.PTR = ERR_INVALID_PROCESS_ID; // set error state to INVALID_PROCESS_ID
		return;
	}

	//	STAGE(2): validate given priority
	if ((INT32) Z502_ARG2.VAL < 0 || (INT32) Z502_ARG2.VAL > MAX_PRIORITY) {
		*(INT32 *) Z502_ARG3.PTR = ERR_INVALID_PROCESS_PRIORITY;
		return;
	}

	//	STAGE(3): change process priority
	if (ID == -1) {
		target = current_process;
	} else {
		map<INT32, PCB*>::iterator it = P_TABLE_BY_ID.find(ID);
		if (it == P_TABLE_BY_ID.end()) {
			// ID not found set error state to INVALID_PROCESS_ID
			*(INT32 *) Z502_ARG3.PTR = ERR_INVALID_PROCESS_ID;
			return;
		} else {
			// ID found
			target = it->second;
		}
	}
	scheduler.change_priority(target, (INT32) Z502_ARG2.VAL, &err);
	*(INT32 *) Z502_ARG3.PTR = ERR_SUCCESS;
}

void sys_get_time_of_day() {
	INT32 time;
	/*
	 STAGE(1) : calling the hardware function and return value to time
	 */
	ZCALL(MEM_READ(Z502ClockStatus, &time));

	/*
	 STAGE(2) : Assign the returned time value
	 */

	*(INT32 *) Z502_ARG1.PTR = time;
}

void sys_sleep() {

	INT32 time; // Time Variable for GET_TIME_OF_DAY call
	INT32 err;
	time = (INT32) Z502_ARG1.VAL;
	//TODO : check for errors
	scheduler.sleep(current_process, time, &err);
}

void sys_create_process() {
	PCB* new_pcb; // Process Control Block Variable for CREATE_PROCESS call
	int err;
	/*
	 STAGE(1): Test for name validity
	 */

	if (strcmp((char*) Z502_ARG1.PTR, "") == 0) {
		*(INT32 *) Z502_ARG5.PTR = ERR_INVALID_PROCESS_NAME; // set error state to INVALID_PROCESS_NAME
		return;
	}

	map<string, PCB*>::iterator NAME_TABLE_IT = P_TABLE_BY_NAME.find(((string) (char *) Z502_ARG1.PTR));
	if (NAME_TABLE_IT != P_TABLE_BY_NAME.end()) {
		// name found
		*(INT32 *) Z502_ARG5.PTR = ERR_DUPLICATE_PROCESS_NAME; // set error state to DUPLCIATE_PROCESS_NAME
		return;
	}

	/*
	 STAGE(2): Test for priority validity
	 */

	if (Z502_ARG3.VAL <= 0) {
		*(INT32 *) Z502_ARG5.PTR = ERR_INVALID_PROCESS_PRIORITY; // set error state to INVALID_PROCESS_PRIORITY
		return;
	}

	/*
	 STAGE(3): Test numer of processes
	 */

	if (NUM_OF_PROCESSES > MAX_NUM_OF_PROCESSES) {
		*(INT32 *) Z502_ARG5.PTR = ERR_MAX_NUM_EXCEEDED; // set error state to INVALID_PROCESS_PRIORITY
		return;
	} else {
		NUM_OF_PROCESSES++;
	}

	/*
	 STAGE(4):Prepare a new Process Control Block
	 */

	new_pcb = new PCB;
	ZCALL( Z502_MAKE_CONTEXT( &new_pcb->CONTEXT, (void *)Z502_ARG2.PTR, USER_MODE ));
	// make a new context

	new_pcb->STATE = PROCESS_STATE_CREATED;
	new_pcb->PROCESS_NAME = (char*) Z502_ARG1.PTR; // set process name
	new_pcb->STARTING_ADDRESS = Z502_ARG2.PTR; // set process starting address
	new_pcb->PRIORITY = Z502_ARG3.VAL; // set process initial priority
	new_pcb->ID = ++NEXT_ID; // set process ID
	new_pcb->PARENT_ID = current_process->ID; // set process parent ID to current process ID

	current_process->children->push_front(new_pcb->ID); // add the new process to the children of the
														// current process

	*(INT32 *) Z502_ARG4.PTR = new_pcb->ID; // return the process ID
	*(INT32 *) Z502_ARG5.PTR = ERR_SUCCESS; // set error state to "SUCCESS"
	/*
	 STAGE(5) Insert PCB into Processes Tables
	 */

	P_TABLE_BY_NAME[new_pcb->PROCESS_NAME] = new_pcb;
	P_TABLE_BY_ID[new_pcb->ID] = new_pcb;

	scheduler.create(new_pcb, &err);
}

void sys_get_process_id() {
	/*
	 STAGE(1): Test name parameter
	 */

	if (strcmp((char*) Z502_ARG1.PTR, "") == 0) // get current process ID
			{
		if (current_process == NULL) // root process (i.e. one of the tests processes)
		{
			*(INT32 *) Z502_ARG2.PTR = 0;
		} else // not root process
		{
			*(INT32 *) Z502_ARG2.PTR = current_process->ID;
		}
		*(INT32 *) Z502_ARG3.PTR = ERR_SUCCESS; // set error state to SUCCESS
		return;
	}

	/*#define			ERR_INVALID_PROCESS_NAME				22L
	 #define			ERR_INVALID_PROCESS_PRIORITY			23L
	 #define			ERR_DUPLICATE_PROCESS_NAME				24L
	 #define			ERR_INVALID_PROCESS_ID					25L
	 #define			ERR_MAX_NUM_EXCEEDED					26L
	 #define			ERR_UNAUTHORIZED_TERMINATION			27L
	 #define			ERR_BUFFER_LENGTH_EXCEEDED				28L
	 STAGE(2): Search for process by name
	 */

	map<string, PCB*>::iterator NAME_TABLE_IT = P_TABLE_BY_NAME.find(((string) (char *) Z502_ARG1.PTR));

	if (NAME_TABLE_IT != P_TABLE_BY_NAME.end()) // name found
			{
		*(INT32 *) Z502_ARG2.PTR = NAME_TABLE_IT->second->ID;
		*(INT32 *) Z502_ARG3.PTR = ERR_SUCCESS; // set error state to SUCCESS
	} else // name not found
	{
		*(INT32 *) Z502_ARG3.PTR = ERR_INVALID_PROCESS_NAME; // set error state to INVALID_PROCESS_NAME
	}

}
void sys_terminate_process() {
	INT32 ID; // ID Variable
	ID = (INT32) Z502_ARG1.VAL;

	/*
	 STAGE(1): Test for id validity
	 */

	if (ID < -2) {
		*(INT32 *) Z502_ARG2.PTR = ERR_INVALID_PROCESS_ID; // set error state to INVALID_PROCESS_ID
		return;
	}

	/*
	 STAGE(2): Apply termination according to ID
	 */

	if (ID == -1) {
		// terminate self
		terminate_process(current_process->ID);
		return;
	}

	if (ID == -2) {
		// terminate self and all descendants
		terminate_descendants(current_process->ID);
		return;
	}

	map<INT32, PCB*>::iterator it = P_TABLE_BY_ID.find(ID);
	if (it == P_TABLE_BY_ID.end()) {
		// ID not found set error state to INVALID_PROCESS_ID
		*(INT32 *) Z502_ARG2.PTR = ERR_INVALID_PROCESS_ID;
	} else // ID found
	{
		//check if the current process is an ancestor of the process to be terminated
		PCB* temp = it->second;
		INT32 current_id = temp->PARENT_ID;

		bool is_ancestor = false;

		while (current_id != -1) {
			if (current_id == current_process->ID) {
				is_ancestor = true;
				break;
			}
			it = P_TABLE_BY_ID.find(current_id);
			current_id = it->second->PARENT_ID;
		}

		if (is_ancestor) {
			terminate_process(ID);
		} else {
			*(INT32 *) Z502_ARG2.PTR = ERR_UNAUTHORIZED_TERMINATION; // set error state to UNAUTHORIZED_TERMINATION
		}
	}
}

void sys_send_message() {
	INT32 ID;
	MAIL* new_mail; // Mail Variable for SEND_MESSAGE call

	/*
	 STAGE(1): Test for target ID  validity
	 */
	ID = Z502_ARG1.VAL;
	if (ID < -1) {
		*(INT32 *) Z502_ARG4.PTR = ERR_INVALID_PROCESS_ID; // set error state to INVALID_PROCESS_ID
		return;
	}


	map<INT32, PCB*>::iterator it = P_TABLE_BY_ID.find(ID);
	if (it == P_TABLE_BY_ID.end()&&ID!=-1) // ID not found
			{
		*(INT32 *) Z502_ARG4.PTR = ERR_INVALID_PROCESS_ID; // set error state to INVALID_PROCESS_ID
		return;
	}

	/*
	 STAGE(2): Test for buffer length
	 */
	int message_length = strlen((char*) Z502_ARG2.PTR);
	if (Z502_ARG3.VAL < message_length || Z502_ARG3.VAL > 64) {
		*(INT32 *) Z502_ARG4.PTR = ERR_BUFFER_LENGTH_EXCEEDED; // set error state to BUFFER_LENGTH_EXCEEDED
		return;
	}

	/*
	 STAGE(3): Test number of messages
	 */

	if (NUM_OF_MESSAGES > MAX_NUM_OF_MESSAGES) {
		*(INT32 *) Z502_ARG4.PTR = ERR_MAX_NUM_EXCEEDED; // set error state to MAX_NUM_EXCEEDED
		return;
	} else {
		NUM_OF_MESSAGES++;
	}

	/*
	 STAGE (4): Broadcast message if required
	 */
	if (ID == -1) {
		new_mail = new MAIL;
		new_mail->SENDER_ID = current_process->ID;
		new_mail->RECEIVER_ID = BROADCAST_ID;
		new_mail->MESSAGE = (char*) Z502_ARG2.PTR;
		new_mail->SENDER_BUFFER_LENGTH = Z502_ARG3.VAL;

		SENDER_MAIL_BOX[new_mail->SENDER_ID] = new_mail;
		RECEIVER_MAIL_BOX[new_mail->RECEIVER_ID] = new_mail;

		//resume any process waiting for your message
		PCB* waiting_me = scheduler.get_waiting_for_message(new_mail->SENDER_ID);

		if (waiting_me == NULL)
			waiting_me = scheduler.get_waiting_for_message(BROADCAST_ID);

		if (waiting_me != NULL) {
			waiting_me->waiting_for_message = false;
			strcpy(waiting_me->mail, new_mail->MESSAGE);
			//TODO check law 3ayz teb2a bub 3la el lengths
			*(waiting_me->recieved_from) = new_mail->SENDER_ID;
			*(waiting_me->recieved_length) = new_mail->SENDER_BUFFER_LENGTH;
			*(waiting_me->mailing_error) = ERR_SUCCESS;

			SENDER_MAIL_BOX.erase(new_mail->SENDER_ID);
			RECEIVER_MAIL_BOX.erase(new_mail->RECEIVER_ID);
			NUM_OF_MESSAGES--;
			int err;
			scheduler.resume(waiting_me, &err);

		}

		*(INT32 *) Z502_ARG4.PTR = ERR_SUCCESS; // set error state to SUCCESS
		return;
	}

	/*
	 STAGE(5): Send message to mail box
	 */
//TODO .. consumable bla bla
	new_mail = new MAIL;
	new_mail->SENDER_ID = current_process->ID;
	new_mail->RECEIVER_ID = Z502_ARG1.VAL;
	new_mail->MESSAGE = (char*) Z502_ARG2.PTR;
	new_mail->SENDER_BUFFER_LENGTH = Z502_ARG3.VAL;

	SENDER_MAIL_BOX[new_mail->SENDER_ID] = new_mail;
	RECEIVER_MAIL_BOX[new_mail->RECEIVER_ID] = new_mail;

	it = P_TABLE_BY_ID.find(new_mail->RECEIVER_ID);

	if (it->second->STATE == PROCESS_STATE_SUSPENDED && it->second->waiting_for_message == true) {
		INT32 err;
		it->second->waiting_for_message = false;
		strcpy(it->second->mail, new_mail->MESSAGE);
		*(it->second->recieved_from) = new_mail->SENDER_ID;
		*(it->second->recieved_length) = new_mail->SENDER_BUFFER_LENGTH;
		*(it->second->mailing_error) = ERR_SUCCESS;

		SENDER_MAIL_BOX.erase(new_mail->SENDER_ID);
		RECEIVER_MAIL_BOX.erase(new_mail->RECEIVER_ID);
		NUM_OF_MESSAGES--;
		scheduler.resume(it->second, &err);

	}

	*(INT32 *) Z502_ARG4.PTR = ERR_SUCCESS; // set error state to SUCCESS

}

void sys_recieve_message() {
	INT32 SOURCE_ID;
	MAIL* mail; // Mail Variable for SEND_MESSAGE call
	INT32 message_length; // Message Length Variable
	/*
	 STAGE(1): Test for target ID  validity
	 */

	SOURCE_ID = Z502_ARG1.VAL;
	if (SOURCE_ID < -1) {
		*(INT32 *) Z502_ARG6.PTR = ERR_INVALID_PROCESS_ID; // set error state to INVALID_PROCESS_ID
		return;
	}

	/*
	 STAGE(2): Handle broadcast if required
	 */

	if (SOURCE_ID == -1) {
		map<INT32, MAIL*>::iterator RECEIVER_MAIL_BOX_IT = RECEIVER_MAIL_BOX.find(current_process->ID);
		if (RECEIVER_MAIL_BOX_IT == RECEIVER_MAIL_BOX.end()) // no one left a message
				{
			RECEIVER_MAIL_BOX_IT = RECEIVER_MAIL_BOX.find(BROADCAST_ID);
			if (RECEIVER_MAIL_BOX_IT == RECEIVER_MAIL_BOX.end()) // no one left a message yet
					{
				INT32 err;
				current_process->waiting_for_message = true;
				current_process->waiting_for = BROADCAST_ID;
				current_process->mail = (char *) Z502_ARG2.PTR;
				current_process->allowed_mail_length = Z502_ARG3.VAL;

				current_process->recieved_length = (INT32 *) Z502_ARG4.PTR;
				current_process->recieved_from = (INT32 *) Z502_ARG5.PTR;
				current_process->mailing_error = (INT32 *) Z502_ARG6.PTR;
				scheduler.suspend(current_process, &err);

				return;
			}
		}
		mail = RECEIVER_MAIL_BOX_IT->second;
		//check for buffer length
		message_length = strlen(mail->MESSAGE);
		if (message_length > Z502_ARG3.VAL || Z502_ARG3.VAL > 64) {
			*(INT32 *) Z502_ARG6.PTR = ERR_BUFFER_LENGTH_EXCEEDED; // set error state to BUFFER_LENGTH_EXCEEDED
			return;
		}
		strcpy((char*) Z502_ARG2.PTR, mail->MESSAGE);
		*(INT32 *) Z502_ARG4.PTR = mail->SENDER_BUFFER_LENGTH;
		*(INT32 *) Z502_ARG5.PTR = mail->SENDER_ID;
		SENDER_MAIL_BOX.erase(mail->SENDER_ID);
		RECEIVER_MAIL_BOX.erase(mail->RECEIVER_ID);
		delete mail;
		*(INT32 *) Z502_ARG6.PTR = ERR_SUCCESS; // set error state to SUCCESS
		NUM_OF_MESSAGES--;
		return;
	}

	/*
	 STAGE(3): Check if the source sent the message
	 */

	map<INT32, PCB*>::iterator it = P_TABLE_BY_ID.find(SOURCE_ID);
	if (it == P_TABLE_BY_ID.end()) // ID not found
			{
		*(INT32 *) Z502_ARG6.PTR = ERR_INVALID_PROCESS_ID; // set error state to INVALID_PROCESS_ID
		return;
	}

	map<INT32, MAIL*>::iterator SENDER_MAIL_BOX_IT = SENDER_MAIL_BOX.find(SOURCE_ID);
	if (SENDER_MAIL_BOX_IT == SENDER_MAIL_BOX.end()) // Source didn't leave a message
			{
		INT32 err;
		current_process->waiting_for_message = true;
		current_process->waiting_for = SOURCE_ID;

		current_process->mail = (char*) Z502_ARG2.PTR;
		current_process->allowed_mail_length = Z502_ARG3.VAL;

		current_process->recieved_length = (INT32 *) Z502_ARG4.PTR;
		current_process->recieved_from = (INT32 *) Z502_ARG5.PTR;
		current_process->mailing_error = (INT32 *) Z502_ARG6.PTR;
		scheduler.suspend(current_process, &err);
		return;

	}

	mail = SENDER_MAIL_BOX_IT->second;

	//check for buffer length

	message_length = strlen(mail->MESSAGE);
	if (message_length > Z502_ARG3.VAL || Z502_ARG3.VAL > 64) {
		*(INT32 *) Z502_ARG6.PTR = ERR_BUFFER_LENGTH_EXCEEDED; // set error state to BUFFER_LENGTH_EXCEEDED
		return;
	}

	strcpy((char*) Z502_ARG2.PTR, mail->MESSAGE);
	SENDER_MAIL_BOX.erase(mail->SENDER_ID);
	RECEIVER_MAIL_BOX.erase(mail->RECEIVER_ID);
	delete mail;
	*(INT32 *) Z502_ARG6.PTR = ERR_SUCCESS; // set error state to SUCCESS
	NUM_OF_MESSAGES--;
	return;
}
void execute_system_call(int call_type) {

	// Switch on system calls
	switch (call_type) {

	case SYSNUM_GET_TIME_OF_DAY:
		CALL(sys_get_time_of_day());
		break;

	case SYSNUM_SLEEP:
		CALL(sys_sleep());

		break;

	case SYSNUM_GET_PROCESS_ID:
		CALL(sys_get_process_id());

		break;

	case SYSNUM_CHANGE_PRIORITY:
		CALL(sys_change_priority());
		break;

	case SYSNUM_CREATE_PROCESS:
		CALL(sys_create_process());
		break;

	case SYSNUM_SUSPEND_PROCESS:
		CALL(sys_suspend_process());
		break;

	case SYSNUM_RESUME_PROCESS:
		CALL(sys_resume_process());
		break;

	case SYSNUM_TERMINATE_PROCESS:
		CALL(sys_terminate_process());
		break;

	case SYSNUM_SEND_MESSAGE:
		CALL(sys_send_message());
		break;

	case SYSNUM_RECEIVE_MESSAGE:
		CALL(sys_recieve_message());
		break;

	case SYSNUM_MEM_READ:
		if (Z502_MODE == USER_MODE)
			printf("error!\n");
		else
			Z502_MEM_READ(Z502_ARG1.VAL, (INT32 *) Z502_ARG2.PTR);
		break;
	case SYSNUM_MEM_WRITE:
		if (Z502_MODE == USER_MODE)
			printf("error!\n");
		else
			Z502_MEM_WRITE(Z502_ARG1.VAL, (INT32 *) Z502_ARG2.PTR);
		break;
	case SYSNUM_READ_MODIFY:
		Z502_READ_MODIFY(Z502_ARG1.VAL, Z502_ARG2.VAL, Z502_ARG3.VAL, (INT32 *) Z502_ARG4.PTR);
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

void create_root_process(void* starting_address, void* context) {
	PCB* root_process = new PCB;
	root_process->PROCESS_NAME = (char*) "root_process"; // set process name
	root_process->STARTING_ADDRESS = (void *) starting_address; // set process starting address
	root_process->PRIORITY = 1; // set process initial priority
	root_process->ID = 0; // set process ID
	root_process->PARENT_ID = -1; // set process parent ID to current process ID
	root_process->CONTEXT = context;
	root_process->STATE = PROCESS_STATE_RUNNING;

	P_TABLE_BY_NAME[root_process->PROCESS_NAME] = root_process;
	P_TABLE_BY_ID[root_process->ID] = root_process;

	current_process = root_process;

	NUM_OF_PROCESSES++;

}

/*
 Method to terminate a process by it's ID
 */

void terminate_process(INT32 ID) {
	INT32 err;
	if (ID == 0) {
		ZCALL(Z502_HALT());
	} else {
		map<INT32, PCB*>::iterator it = P_TABLE_BY_ID.find(ID);
		if (it != P_TABLE_BY_ID.end()) // ID found
				{
			PCB* temp = it->second;

			// Delete PCB from first table
			P_TABLE_BY_NAME.erase(temp->PROCESS_NAME);

			// Delete PCB from second table
			P_TABLE_BY_ID.erase(temp->ID);

			//TODO needs checking for parent if exists
			P_TABLE_BY_ID[temp->PARENT_ID]->children->remove(temp->ID);
			*(INT32 *) Z502_ARG2.PTR = ERR_SUCCESS; // set error state to SUCCESS
			NUM_OF_PROCESSES--;
			scheduler.terminate(temp, &err);
			// Free all PCB Data
			delete temp->children;
			delete temp;
			// Destroy Process Context
			//ZCALL(Z502_DESTROY_CONTEXT(&(temp->CONTEXT)));
		}

	}

}

/*
 Method to terminate a process with all of its descendants
 */

void terminate_descendants(INT32 ID) {
	map<INT32, PCB*>::iterator it = P_TABLE_BY_ID.find(ID);
	INT32 child_id;

	if (it != P_TABLE_BY_ID.end()) // ID found
			{
		PCB* temp = it->second;
		// REMOVED ITERATORS scheme , CAUSED BUG because they are deleted inside terminate_process();
		// terminate all children processes
		while (temp->children->size()) {
			child_id = temp->children->front();
			terminate_descendants(child_id);
		}
		terminate_process(ID); // terminate self
	}
}

