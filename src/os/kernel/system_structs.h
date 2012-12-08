/*
 * system_structs.h
 *
 *  Created on: Dec 6, 2012
 *      Author: mahmoudel-maghraby
 */
typedef         int                             INT32;
typedef         unsigned int                   UINT32;
typedef         short int                        INT16;
typedef         unsigned short                  UINT16;
typedef         int                             BOOL;

#include 			 <map>
#include 			 <string>
#include			 <list>

#define MAX_NUM_OF_PROCESSES 100
using namespace std;





struct PCB
{
	void *CONTEXT;
	void *STARTING_ADDRESS;
	char* PROCESS_NAME;
	INT32 PRIORITY;
	INT32 ID;
	INT32 PARENT_ID;
	list<INT32>* children;
};




map <string, PCB*> P_TABLE_BY_NAME;
map <INT32, PCB*> P_TABLE_BY_ID;

map<string,PCB*>::iterator NAME_TABLE_IT ;
map<INT32,PCB*>::iterator ID_TABLE_IT ;

PCB* current_process = NULL ;

