#include "../kernel/system_structs.h"
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

bool process_queue::remove(PCB * p) {

	q_item * item = m[p->ID];

	if (!item)
		return false;

	len--;

	q_item * back = item->prev;
	q_item * forw = item->next;
	back->next = forw;
	forw->prev = back;

	m.erase(p->ID);
	free(item);
	return true;
}

bool process_queue::enqueue(PCB * p) {
	q_item* item = (q_item *) malloc(sizeof(q_item));
	item->proc = p;

	q_item* next = head->next;
	head->next = item, item->prev = head;
	item->next = next, next->prev = item;

	m[p->ID] = item;
	len++;
	return true;
}
PCB* process_queue::dequeue() {
	if (!len)
		return NULL;
	else
		len--;

	q_item* ret = tail->prev;
	q_item* temp = ret->prev;
	temp->next = tail, tail->prev = temp;

	PCB* p = ret->proc;

	free(ret);
	m.erase(p->ID);
	return p;
}

int process_queue::size() {
	return len;
}

