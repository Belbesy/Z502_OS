
#ifndef SCHED_H
#define SCHED_H


#include "process_queue.h"
#include "system_structs.h"


#define STARVING_INTERVAL 1000
#define SCHED_INTERVAL 50

#define MAX_PRIORITY 50

#define USER_ERROR_INVALID_STATE_TRANSITION 		1
#define USER_ERROR_INVALID_PORCESS_ID 				2
#define USER_ERROR_CORRUPTED_PROCESS 				3
#define USER_ERROR_INVALID_ARGUMENT 				4

//TODO:  to be moved to process struct.
#define PROCESS_STATE_ZOMBIE 8


typedef bool (*condition_predicate)(void *);

class scheduler_t {

	process_queue suspended, starving, timer, blocked, ready[MAX_PRIORITY];

public:
	scheduler_t();

	void init();

	bool create(PCB* );

	// scheduler;
	bool schedule(PCB* calling, bool terminate, condition_predicate condition, void* arg, bool suspend);

	// wakeup process after sleeping
	void wakeup(PCB* );

	// move starving process to starving queue;
	void feed(PCB *);

	// sleep a process
	bool sleep(PCB*, int , int*);

	// resume a procestarvingss
	bool resume(PCB*, int*);

	// increases priority for a process with given id
	bool change_priority(PCB*, int, int*);

	// suspend a process
	bool suspend(PCB*, int*);

	// terminate a process
	bool terminate(PCB*, int*);

	bool validate_priority(int);


	bool add_to_ready(PCB *);

	bool remove_from_ready(PCB *, bool);

};

// main scheduling function
void _wakeup(void *);

// move process to urgent priority 0
void _feed(void *);

#endif
