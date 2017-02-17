/*
*  Nisos - A basic multitasking Operating System for ARM Cortex-M3
*          processors
*
*  Copyright (c) 2014 Uditha Atukorala
*
*  This file is part of Nisos.
*
*  Nisos is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  Nisos is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with Nisos.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#ifndef KERNEL_KERNEL_H
#define KERNEL_KERNEL_H


#include "main.h"



#include <stddef.h>
#include <stdint.h>

#include <sys/types.h>
#include <queue.h>

#define KERNEL_SCHEDULER_FLAG 0x0001

#define TASK_ACTIVE_FLAG      0x0001
#define TASKS_SEM_WAIT_FLAG   0x0002


typedef uint32_t systicks_t;

typedef uint32_t status_t;

typedef uint32_t memaddr_t;




// context saved by the hardware
typedef struct stkctx_t {
	memaddr_t r0;
	memaddr_t r1;
	memaddr_t r2;
	memaddr_t r3;
	memaddr_t r12;
	memaddr_t lr;
	memaddr_t pc;
	memaddr_t psr;
}stkctx_t;

// context saved by the software
typedef struct tskctx_t {
	memaddr_t r4;
	memaddr_t r5;
	memaddr_t r6;
	memaddr_t r7;
	memaddr_t r8;
	memaddr_t r9;
	memaddr_t r10;
	memaddr_t r11;
	memaddr_t lr;
}tskctx_t;

typedef struct __event_t {
	//SIMPLEQ_ENTRY(__event_t) events;		/*< Used to reference a task from an event list. */
} event_t;

typedef TAILQ_HEAD(__t_task_queue, tskTaskControlBlock) t_task_queue;
typedef struct tskTaskControlBlock
{
	volatile uint32_t	*pxTopOfStack;	/*< Points to the location of the last item placed on the tasks stack.  THIS MUST BE THE FIRST MEMBER OF THE TCB STRUCT. */
	#if ( portUSING_MPU_WRAPPERS == 1 )
		xMPU_SETTINGS	xMPUSettings;		/*< The MPU settings are defined as part of the port layer.  THIS MUST BE THE SECOND MEMBER OF THE TCB STRUCT. */
		BaseType_t		xUsingStaticallyAllocatedStack; /* Set to pdTRUE if the stack is a statically allocated array, and pdFALSE if the stack is dynamically allocated. */
	#endif
	TAILQ_ENTRY(tskTaskControlBlock) Queue;	// The current queue the task is in
	TAILQ_ENTRY(tskTaskControlBlock) Event;	// The current queue if we have an event
	t_task_queue* EventQueue;
	status_t status;
//	SIMPLEQ_HEAD(__t_event_queue, __event_t) Events; 	/*< Used to reference a task from an event list. */
	TAILQ_ENTRY(tskTaskControlBlock) SemaphoreEntry;		/*< Used to reference a task from an event list. */
	pid_t	   			pid;
	uint32_t			*pxStack;			/*< Points to the start of the stack. */
	char				pcTaskName[ 64 ];/*< Descriptive name given to the task when created.  Facilitates debugging only. */ /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
	uint32_t			delay;  // time delay till we go again
	uint32_t			runTime;
	uint32_t			counter;
	uint32_t			priority;
	uint32_t			rt_priority;
	uint32_t			policy;
	uint32_t			state;
	uint32_t			timeout;
	uint32_t			signal;
	uint32_t			blocked;
	#if ( portSTACK_GROWTH > 0 )
		StackType_t		*pxEndOfStack;		/*< Points to the end of the stack on architectures where the stack grows up from low memory. */
	#endif

	#if ( portCRITICAL_NESTING_IN_TCB == 1 )
		UBaseType_t 	uxCriticalNesting; 	/*< Holds the critical section nesting depth for ports that do not maintain their own count in the port layer. */
	#endif

	#if ( configUSE_TRACE_FACILITY == 1 )
		UBaseType_t		uxTCBNumber;		/*< Stores a number that increments each time a TCB is created.  It allows debuggers to determine when a task has been deleted and then recreated. */
		UBaseType_t  	uxTaskNumber;		/*< Stores a number specifically for use by third party trace code. */
	#endif

	#if ( configUSE_MUTEXES == 1 )
		UBaseType_t 	uxBasePriority;		/*< The priority last assigned to the task - used by the priority inheritance mechanism. */
		UBaseType_t 	uxMutexesHeld;
	#endif

	#if ( configUSE_APPLICATION_TASK_TAG == 1 )
		TaskHookFunction_t pxTaskTag;
	#endif

	#if( configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0 )
		void *pvThreadLocalStoragePointers[ configNUM_THREAD_LOCAL_STORAGE_POINTERS ];
	#endif

	#if ( configGENERATE_RUN_TIME_STATS == 1 )
		uint32_t		ulRunTimeCounter;	/*< Stores the amount of time the task has spent in the Running state. */
	#endif

	#if ( configUSE_NEWLIB_REENTRANT == 1 )
		/* Allocate a Newlib reent structure that is specific to this task.
		Note Newlib support has been included by popular demand, but is not
		used by the FreeRTOS maintainers themselves.  FreeRTOS is not
		responsible for resulting newlib operation.  User must be familiar with
		newlib and must provide system-wide implementations of the necessary
		stubs. Be warned that (at the time of writing) the current newlib design
		implements a system-wide malloc() that must be provided with locks. */
		struct 	_reent xNewLib_reent;
	#endif

	#if ( configUSE_TASK_NOTIFICATIONS == 1 )
		volatile uint32_t ulNotifiedValue;
		volatile eNotifyValue eNotifyState;
	#endif

} task_t;


typedef SLIST_HEAD(__t_task_list,__task_t) t_task_list;
// semaphore data structure
typedef struct __sem_t {
	key_t                       key;      ///< semaphore key
	volatile unsigned int       value;    ///< semaphore value
	SLIST_ENTRY(__sem_t) entry;
	t_task_list tasks;
} sem_t;

#define SEM_INIT { 0, 0, { NULL }, { NULL } }
extern void nisos_boot();
extern void nisos_run();

extern uint32_t __stack;
void context_switch( memaddr_t ** next_spp, memaddr_t ** current_spp );

// semaphore interface
sem_t * sem_open( key_t key );
int sem_init( sem_t * sem, unsigned int value );
int sem_wait( sem_t * sem );
int sem_post( sem_t * sem );
pid_t launch( void * (* task)(void *), size_t stk_size );
#endif /* !KERNEL_KERNEL_H */
