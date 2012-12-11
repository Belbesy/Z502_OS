/*
 * system_structs.h
 *
 *  Created on: Dec 6, 2012
 *      Author: mahmoudel-maghraby
 */

#ifndef SYSTEM_STRUCTS_H
#define SYSTEM_STRUCTS_H

typedef         int                             INT32;
typedef         unsigned int                   UINT32;
typedef         short int                        INT16;
typedef         unsigned short                  UINT16;
typedef         int                             BOOL;

#include 			 <map>
#include 			 <string>
#include			 <list>



#define MAX_NUM_OF_PROCESSES 10
#define MAX_NUM_OF_MESSAGES  20

#define PROCESS_STATE_READY 		1
#define PROCESS_STATE_SLEEPING 		2

#define PROCESS_STATE_BLOCKED 		3 	// blocked by other process
#define PROCESS_STATE_SUSPENDED 	4   // suspended by user

#define PROCESS_STATE_RUNNING       5



using namespace std;





struct PCB
{
	void *CONTEXT;
	void *STARTING_ADDRESS;
	char* PROCESS_NAME;
	INT32 PRIORITY;
	INT32 ID;
	INT32 PARENT_ID;
	INT32 STATE;
	list<INT32>* children;

	PCB()
	{
		CONTEXT = NULL;
		STARTING_ADDRESS = NULL;
		PROCESS_NAME = NULL ;
		PRIORITY = 0 ;
		ID = 0 ;
		PARENT_ID = 0;
		STATE = 0;
		children = new list<INT32>;
	}
};

struct MAIL
{
	INT32 SENDER_ID;
	INT32 RECEIVER_ID;
	INT32 SENDER_BUFFER_LENGTH;
	char* MESSAGE;

	MAIL()
	{
		SENDER_ID = 0;
		RECEIVER_ID = 0;
		SENDER_BUFFER_LENGTH = 0 ;
		MESSAGE = NULL;
	}
};




#endif




