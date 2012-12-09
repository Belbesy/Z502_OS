#ifndef OS_H
#define OS_H


#include             "./sched/alarm.h"
#include             "./sched/sched.h"
#include             "./sched/intman.h"
#include             "./sched/faultman.h"
#include             "./kernel/system_calls.h"

alarm_manager_t alarm_manager;
scheduler_t scheduler;

#endif
