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



#define MAX_NUM_OF_PROCESSES 100


#define PROCESS_STATE_READY 		1
#define PROCESS_STATE_SLEEPING 		2

#define PROCESS_STATE_BLOCKED 		3 	// blocked by other process
#define PROCESS_STATE_SUSPENDED 	4   // suspended by user



using namespace std;





struct PCB
{
	void *CONTEXT;
	void *STARTING_ADDRESS;
	char* PROCESS_NAME;
	INT32 PRIORITY;/*F2_H*/
	INT32 ID;
	INT32 PARENT_ID;
	list<INT32>* children;

	INT32 STATE;
};


#endif




