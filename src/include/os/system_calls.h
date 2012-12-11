/*
 * system_calls.h
 *
 *  Created on: Dec 6, 2012
 *      Author: mahmoudel-maghraby
 */
#ifndef SYSTEM_CALLS_H
#define SYSTEM_CALLS_H

typedef         int                             INT32;


void execute_system_call(int call_type);
void terminate_process(INT32 ID);
void create_root_process(void* starting_address,void* context);
void terminate_descendants(INT32 ID);

#endif
