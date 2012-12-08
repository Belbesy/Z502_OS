
#include "process_queue.h"

using namespace std;

#define SCHED_INTERVAL 50
#define MAX_PRIORITY 50

#define USER_ERROR_INVALID_STATE_TRANSITION 		1
#define USER_ERROR_INVALID_PORCESS_ID 				2
#define USER_ERROR_CORRUPTED_PROCESS 				3
#define USER_ERROR_INVALID_ARGUMENT 				4

class scheduler_t {

	process_queue suspended, sleeping, blocked, ready[MAX_PRIORITY];

public:
	scheduler_t();

	// scheduler;
	void schedule();

	// sleep a process
	bool sleep(PCB*, int*);

	// resume a process
	bool resume(PCB*, int*);

	// increases priority for a process with given id
	bool change_priority(PCB*, int, int*);

	// suspend a process
	bool suspend(PCB*, int*);

	// terminate a process
	bool terminte(PCB*, int*);

	bool validate_priority(int);

	~scheduler_t();
};

// main scheduling function
void schedule(void);

// move process to urgent priority 0
void feed(void);

