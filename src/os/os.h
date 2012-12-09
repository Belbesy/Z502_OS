#ifndef OS_H
#define OS_H


#include             "./sched/alarm.h"
#include             "./sched/sched.h"
#include             "./sched/intman.h"
#include             "./sched/faultman.h"
#include             "./kernel/system_calls.h"

alarm_manager_t alarm_manager;
scheduler_t scheduler;

map <string, PCB*> 				P_TABLE_BY_NAME;
map<string,PCB*>::iterator 		NAME_TABLE_IT ;

map <INT32, PCB*> 				P_TABLE_BY_ID;
map<INT32,PCB*>::iterator 		ID_TABLE_IT ;

map<INT32, MAIL*>				SENDER_MAIL_BOX;
map<INT32, MAIL*>::iterator		SENDER_MAIL_BOX_IT;

map<INT32, MAIL*>				RECEIVER_MAIL_BOX;
map<INT32, MAIL*>::iterator		RECEIVER_MAIL_BOX_IT;
#endif
