

#define TIMER_INTERVAL  100 // in milliseconds
typedef void (*alarm_handler)(void);

struct alarmable_t{

	long time;
	alarm_handler alarm_h;

	struct alarmable_t* prev;
	struct alarmable_t* next;

};

typedef struct alarmable_t alarmable;


// Interrupt handlers
void init_alarm(void);
void add_alarm(alarmable* );
void alarm_ih(int a, int b);
void z502_timer_set(void);



