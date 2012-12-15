#ifndef PROCESS_QUEUE_H
#define PROCESS_QUEUE_H


#include <system_structs.h>

#include <map>
#include <cstdlib>



using namespace std;

 struct q_item{
	PCB * proc;
	q_item* prev, *next;
};


struct process_queue {

	int len;
	map<int, q_item *> m; // map for easy retrieval
	q_item* head, *tail;

	process_queue() ;

	void remove(PCB * p);

	void enqueue(PCB * p);
	void dequeue(PCB** p) ;

	void size(int *);

};

#endif
