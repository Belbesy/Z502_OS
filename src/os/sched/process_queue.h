
#include "../kernel/system_structs.h"
#include <map>

 struct q_item_t {
	PCB * proc;
	q_item_t* prev,* next;
};

 typedef q_item_t q_item;

 class process_queue{
 private:
	 map<int, PCB *> m; // map for easy retrieval
	 q_item head, tail;
public:
	 process_queue();

	 bool remove(PCB *); // remove by id;
	 bool queue(PCB *);
	 bool dequeue();
	 ~process_queue();

 };
