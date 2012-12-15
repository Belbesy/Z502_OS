#include <system_structs.h>
#include <list>
#include <process_queue.h>
#include <const.h>
#include <stdio.h>
using namespace std;

#define MAX_PRIORITY 300


class pqueue {
public:
	int sz;
	process_queue queues[MAX_PRIORITY];
	pqueue() {
		sz = 0;
	}
	void validate_priority(PCB* p) {
		if (0 > p->PRIORITY && p->PRIORITY >= MAX_PRIORITY) {
#ifdef DEBUG_OS
			printf("=> => Invalid process(%d,%s) priority given(%d)! \n", p->ID, p->PROCESS_NAME, p->PRIORITY);
#endif
			exit(1);
		}
	}

	void add(PCB * p) {
		validate_priority(p);
		queues[p->PRIORITY].enqueue(p);
		sz++;
	}

	void dequeue(PCB ** result) {
		for (int i = 0; i < MAX_PRIORITY; i++) {
			int size;
			this->queues[i].size(&size);
			if (size) {
				this->queues[i].dequeue(result);
				sz--;
				return;
			}
		}
		*result = NULL;
	}

	void remove(PCB* p) {
		validate_priority(p);
		queues[p->PRIORITY].remove(p);
		sz--;
	}

	void size(int* size){
		*size =sz;
	}

};
