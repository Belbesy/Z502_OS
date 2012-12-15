#include "system_structs.h"
#include "process_queue.h"
#include <map>
#include <cstdlib>

using namespace std;

process_queue::process_queue() {
	len = 0;
	head = (q_item *) malloc(sizeof(q_item));
	tail = (q_item *) malloc(sizeof(q_item));
	head->next = tail;
	tail->prev = head;
}

void process_queue::remove(PCB * p) {

	q_item * item = m[p->ID];

	if (!item) {
#ifdef DEBUG_OS
		eprint();
		printf("=> => Process given (%d,%s) wasn't found in queue of priority! \n", p->ID, p->PROCESS_NAME, p->PRIORITY);
#endif
		exit(1);
	}

	len--;

	q_item * back = item->prev;
	q_item * forw = item->next;

	back->next = forw;
	forw->prev = back;

	m.erase(p->ID);
	free(item);
}

void process_queue::enqueue(PCB * p) {
	q_item* item = (q_item *) malloc(sizeof(q_item));
	item->proc = p;

	q_item* next = head->next;
	head->next = item, item->prev = head;
	item->next = next, next->prev = item;

	m[p->ID] = item;
	len++;
}
void process_queue::dequeue(PCB** result) {
	if (!len) {
#ifdef DEBUG_OS
		eprint();
		printf("=> => Can't dequeue an empty queue \n");
#endif
		exit(1);
	} else
		len--;

	q_item* ret = tail->prev;
	q_item* temp = ret->prev;
	temp->next = tail, tail->prev = temp;

	PCB* p = ret->proc;

	free(ret);
	m.erase(p->ID);
	*result = p;
}

void process_queue::size(int * result) {
	*result = len;
}

