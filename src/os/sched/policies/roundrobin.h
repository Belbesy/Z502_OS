
#include <stdio.h>

class roundrobin_scheduler{
	start;

	void k(){
		 __asm
		   {
		      mov eax, num    ; Get first argument
		      mov ecx, power  ; Get second argument
		      shl eax, cl     ; EAX = EAX * ( 2 to the power of CL )
		   }
	}
};
