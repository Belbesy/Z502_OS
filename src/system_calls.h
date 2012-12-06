/*
 * system_calls.h
 *
 *  Created on: Dec 6, 2012
 *      Author: mahmoudel-maghraby
 */





void execute_system_call(int call_type);


/*
	unneeded call code, we are using a c++ compiler anyway
#ifdef __cplusplus // only actually define the class if this is C++

class system_calls {
public:
	void execute_system_call(INT16 call_type);
};

#else

// C doesn't know about classes, just say it's a struct
typedef struct system_calls system_calls;

#endif

// access functions
#ifdef __cplusplus
    #define EXPORT_C extern "C"
#else
    #define EXPORT_C
#endif

EXPORT_C system_calls* system_calls_new(void);
EXPORT_C void system_calls_delete(system_calls*);
EXPORT_C void execute_system_call(system_calls*, INT16);

*/
