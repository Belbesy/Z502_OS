/*
 * Author : Belbesy
 * Created : Monday 3rd Dec 2012
 */


#include                 "intman.h"
#include                 "alarm.h"

#include                 "../../global.h"
#include                 "../../syscalls.h"
#include                 "../../z502.h"
#include                 "../../protos.h"
#include                 <stdio.h>
#include                 <stdlib.h>
#include                 <string.h>


// array of interrupt handlers to be changed to map if possible
int_handler int_handlers[MAX_HANDLERS]= {alarm_ih};


/************************************************************************
    INTERRUPT_HANDLER
        When the Z502 gets a hardware interrupt, it transfers control to
        this routine in the OS.

        manages all interrupts and passes it to the responsible handler
************************************************************************/
void    os_interrupt_handler( void ) {
    INT32              device_id;
    INT32              status;
    INT32              Index = 0;

    // Get cause of interrupt
    MEM_READ(Z502InterruptDevice, &device_id );
    // Set this device as target of our query
    MEM_WRITE(Z502InterruptDevice, &device_id );
    // Now read the status of this device
    MEM_READ(Z502InterruptStatus, &status );

    int_handlers[device_id](device_id, status);

    // Clear out this device - we're done with it
    MEM_WRITE(Z502InterruptClear, &Index );
}
/* End of os_interrupt_handler */

