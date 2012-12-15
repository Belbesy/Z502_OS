
#ifndef SCHED_H
#define SCHED_H


#include "process_queue.h"
#include "system_structs.h"

#include "priority_queue.h"


#define STARVING_INTERVAL 1000
#define SCHED_INTERVAL 50

#define USER_ERROR_INVALID_STATE_TRANSITION 		1
#define USER_ERROR_INVALID_PORCESS_ID 				2
#define USER_ERROR_CORRUPTED_PROCESS 				3
#define USER_ERROR_INVALID_ARGUMENT 				4

//TODO:  to be moved to process struct.



typedef void (*condition_predicate)(void *, bool*);

class process_queue;
struct PCB;

class scheduler_t {

	process_queue suspended, starving, timer, blocked;
	pqueue ready;

public:
	scheduler_t();

	void init();

	void create(PCB* , int* err);

	// sleep a process
	void sleep(PCB*, int time, int* err);

	// terminate a process
	void terminate(PCB*, int* err);

	// scheduler;
	void schedule(PCB*);

	// wakeup process after sleeping
	void wakeup(PCB* );



	// resume a procestarvingss
	bool resume(PCB*, int*);

	// increases priority for a process with given id
	bool change_priority(PCB*, int, int*);

	// suspend a process
	bool suspend(PCB*, int*);

};

// main scheduling function
void wakeup_process(void *);

// move process to urgent priority 0
void _feed(void *);

#endif
