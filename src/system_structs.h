/*
 * system_structs.h
 *
 *  Created on: Dec 6, 2012
 *      Author: mahmoudel-maghraby
 */

#ifndef SYSTEM_STRUCTS_H_
#define SYSTEM_STRUCTS_H_
#define MAX_LENGTH 50

typedef struct
{
	char PROCESS_NAME[MAX_LENGTH];
	INT32 PRIORITY;
	INT32 ID;
	INT32 PARENT_ID;
}PROCESS_PCB;

typedef struct
{
	void *CONTEXT;
	void *STARTING_ADDRESS;
	PROCESS_PCB PCB;

}PROCESS;


#endif /* SYSTEM_STRUCTS_H_ */
