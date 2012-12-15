/*
 * Author : Belbesy
 * Created : Monday 3rd Dec 2012
 */

#ifndef INTMAN_H
#define INTMAN_H


// constants
#define MAX_HANDLERS 10



// types
typedef void (* int_handler)(int, int);


void    os_interrupt_handler(int, int);
void 	os_fault_handler(int, int);

#endif
