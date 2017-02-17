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

#include "kernel.h"
#include "os_conf.h"

#include <limits.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <assert.h>
#define PRIVILEGED_FUNCTION
#define PRIVILEGED_DATA

//#include <os/cmsis/core_cm3.h>
void SysTick_Handler();
void PendSV_Handler()  __attribute__ ( (  naked ) );
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 0xFF
#ifndef SYS_IDLE_STACK_SIZE
#define SYS_IDLE_STACK_SIZE 16
#endif

#ifndef MAX_SEMS_AVALIABLE
#define MAX_SEMS_AVALIABLE 16
#endif

memaddr_t   _stk_space[KERNEL_TASK_STACK_SIZE];
memaddr_t *   _free_stkp;

task_t        _tasks[MAX_TASKS + 1];
systicks_t    _systicks=0;
status_t      _status=0;
pid_t         _current_task=0;
sem_t		 sem_array[MAX_SEMS_AVALIABLE];
size_t		 sem_used = 0;
volatile task_t * pxCurrentTCB = NULL;
static volatile bool _schedulerSuspended = true;
static volatile bool _yieldPending = false;
static volatile uint32_t _pendingTicks = 0;
static volatile uint32_t _tickCount = 0;

SLIST_HEAD(__sem_list,__sem_t) _sems = SLIST_HEAD_INITIALIZER(_sems);
volatile uint32_t _critical_nesting = 0;
void enter_critical() {
	__disable_irq();
	_critical_nesting++;
	if( _critical_nesting == 1 )
	{
		assert((__get_CONTROL() & 0xFFU) ==0); // recursive?
	}
}
void exit_critical() {
	assert(_critical_nesting); // must atleast be above 0
	_critical_nesting--;
	if(!_critical_nesting) __enable_irq();
}

void nisos_kernel_init() {
	// reserve memory for task stack space
	//_stk_space = new memaddr_t [KERNEL_TASK_STACK_SIZE];
	memset( _stk_space, 0, ( sizeof( memaddr_t ) * KERNEL_TASK_STACK_SIZE ) );

	// set free stack pointer
	_free_stkp = _stk_space;
	_status = 0;

	// set interrupt priority grouping to be preemptive (no sub-priority)
	NVIC_SetPriorityGrouping( 0b011 );

	// configure system tick frequency
	SysTick_Config( SystemCoreClock / SYSTICK_FREQUENCY_HZ );

	// clear base priority for exception processing
	__set_BASEPRI( 0 );

	// set PendSV to the lowest priority
	NVIC_SetPriority( PendSV_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY );
	// set Systick too
	NVIC_SetPriority( SysTick_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY );
	NVIC_SetPriority( SVCall_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY );
}

memaddr_t * reserve_stack( size_t stk_size ) {
	memaddr_t * stack_space = 0;
	enter_critical();
	// add required stack size for context switching
	stk_size += ( sizeof( stkctx_t ) + sizeof( tskctx_t ) ) / sizeof( memaddr_t );

	// do we have enough stack space left?
	if ( ( _free_stkp + stk_size ) < ( _stk_space + KERNEL_TASK_STACK_SIZE ) )  {

		_free_stkp += stk_size;
		stack_space = ( _free_stkp - 1 );

	}
	exit_critical();
	return stack_space;

}
pid_t reschedule() {
		pid_t next_task = 0;
		for ( pid_t task = 1; task <= MAX_TASKS; task++ ) {
			if ( ( _tasks[task].status & TASKS_SEM_WAIT_FLAG )
				|| ( task == _current_task ) ) {
				continue;
			}
			if ( _tasks[task].status & TASK_ACTIVE_FLAG ) {
				next_task = task;
				break;
			}
		}
		return next_task;
}


void system_task() {

	// reset the MSP
	__set_MSP( (uint32_t) &__stack );

	// update task table
	//_tasks[0].sp      = (memaddr_t *)( &__stack );
	_tasks[0].status |= TASK_ACTIVE_FLAG;

	// enable interrupts
	__enable_irq();

	// drop privileges
	__set_CONTROL( 0x01 );

	// system idle loop
	while ( true );

}
void systick_handler() {

}
#if 0
void scheduler() {
	pid_t next_task    = reschedule();
	pid_t current_task = _current_task;
	if ( next_task != 0 ) {

		// get a copy of MSP
		__asm volatile ( "mrs     r12, msp" );

		if ( current_task != 0 ) {

			// copy the saved context onto task stack space
			__asm volatile (
				"push    {r4 - r11}            \n"
				"ldmia   r12!, {r4 - r11, lr}  \n"
				"mrs     r12, psp              \n"
				"stmdb   r12!, {r4 - r11, lr}  \n"
				"pop     {r4 - r11}            \n"
			);

		} else {

			// context switch will try to reset the MSP
			__asm volatile ( "mrs     r11, msp" );

		}

		_current_task = next_task;
		context_switch( &_tasks[next_task].sp, &_tasks[current_task].sp );
	}
	// we are switching back to system task

	// are we already running the system task?
	if ( current_task != 0 ) {

		// save user task context
		__asm volatile (
			"mrs     r12, psp              \n"
			"str     r12, [%0, #0]         \n"
			: : "r" ( &_tasks[current_task].sp )
		);

	}

	// task switch to system task
	__asm volatile (
		"msr      msp, %0           \n"
		"pop      {r4 - r11, lr}    \n"
		"bx       lr                \n"
		: : "r" ( &_tasks[0].sp )
	);

}
#endif
void task_exit() {
	// task exited
	assert(0);
}
// default PSR value
#define DEFAULT_PSR 0x01000000U
#define EXEC_RETURN 0xFFFFFFFDU
void task_return() {
	assert(0);
	while(1); // fuck me we shouldn't return
}
uint32_t* init_stack(uint32_t* topOfStack,  void * (* task)(void *), void* parm) {
	/* Simulate the stack frame as it would be created by a context switch
		interrupt. */

		/* Offset added to account for the way the MCU uses the stack on entry/exit
		of interrupts, and to ensure alignment. */
	topOfStack--;

		*topOfStack = DEFAULT_PSR;	// PSR, oviously
		topOfStack--;
		*topOfStack = ( uint32_t ) task;	// task
		topOfStack--;
		*topOfStack = ( uint32_t ) task_return;	// LR, in case the thread returns
		topOfStack -= 5;
		*topOfStack = ( uint32_t ) parm;	// R0
		// A save method is being used that requires each task to maintain its parm
		topOfStack--;
		*topOfStack = EXEC_RETURN;

		topOfStack -= 8;	/* R11, R10, R9, R8, R7, R6, R5 and R4. */
		return topOfStack;
}



typedef struct _simple_task_t {
	TAILQ_ENTRY(_simple_task_t) queue;
	void * (* func)(void *);
	void* parm;
} simple_task_t;
typedef TAILQ_HEAD(__t_simple_task_queue, _simple_task_t) t_simple_task_queue;
static t_simple_task_queue _taskQueue = TAILQ_HEAD_INITIALIZER(_taskQueue);

typedef _TAILQ_HEAD(__t_vtask_queue, task_t,) t_vtask_queue;

static  t_vtask_queue _running = TAILQ_HEAD_INITIALIZER(_running);
static  t_vtask_queue _waiting = TAILQ_HEAD_INITIALIZER(_waiting);
static volatile uint32_t _numberOfTasks = 0;
static volatile uint32_t _numberOfTasksRunning = 0;
static volatile uint32_t context_swtch = 0;

#define TAILQ_ELM_NOTINQ(head, elm, field) (((elm)->field.tqe_next == NULL) && ((head)->tqh_last != &(elm)->field.tqe_next))

#define SWAP(type, value1, value2) do { \
		type tmp = (value1); \
		(value1) = (value2); \
		(value2) = tmp; } while(0);
bool need_resched = false;
static inline void add_to_runqueue(task_t * p)
{
	assert(TAILQ_ELM_NOTINQ(&_running, p, Queue));
#if 0	/* sanity tests */
	if (p->next_run || p->prev_run) {
		printk("task already on run-queue\n");
		return;
	}
#endif
	if (p->policy != SCHED_OTHER || p->counter > pxCurrentTCB->counter + 3)
		need_resched = true;
	_numberOfTasksRunning++;
	TAILQ_INSERT_TAIL(&_running,p,Queue);
}
static inline void del_from_runqueue(task_t * p)
{
	assert(!TAILQ_ELM_NOTINQ(&_running, p, Queue));
#if 0	/* sanity tests */
	if (p->next_run || p->prev_run) {
		printk("task already on run-queue\n");
		return;
	}
#endif
	_numberOfTasksRunning--;
	TAILQ_REMOVE(&_running,p,Queue);
}
static inline void move_last_runqueue(task_t * p)
{
	TAILQ_REMOVE(&_running,p,Queue);
	TAILQ_INSERT_TAIL(&_running,p,Queue);
}


static inline void wake_up_process(task_t * p)
{
	unsigned long flags;
	save_flags(flags);
	cli();
	p->state = TASK_RUNNING;
	if (TAILQ_ELM_NOTINQ(&_running, p, Queue))
		add_to_runqueue(p);
	restore_flags(flags);
}

static void process_timeout(unsigned long __data)
{
	task_t * p = (task_t*) __data;
	p->timeout = 0;
	wake_up_process(p);
}

static inline int goodness(task_t * p, task_t * prev)
{
	int weight;
	/*
	 * Realtime process, select the first one on the
	 * runqueue (taking priorities within processes
	 * into account).
	 */
	if (p->policy != SCHED_OTHER)
		return 1000 + p->rt_priority;

	/*
	 * Give the process a first-approximation goodness value
	 * according to the number of clock-ticks it has left.
	 *
	 * Don't do any other calculations if the time slice is
	 * over..
	 */
	weight = p->counter;
	if (weight) {
		/* .. and a slight advantage to the current process */
		if (p == prev)
			weight += 1;
	}
	return weight;
}


void run_simple_task_queue() {
	simple_task_t* task;
	TAILQ_FOREACH(task, &_taskQueue, queue) task->func(task->parm);
}
void culinux_scheduler() { /// wow...this is like really simple
	uint32_t timeout = 0;
	// run task queue here
	// run_simple_task_queue(); 	// enable interrupts here?
	task_t* prev, *next, *p;
	prev = (task_t*)pxCurrentTCB;
	if (!prev->counter && prev->policy == SCHED_RR) {
		prev->counter = prev->priority;
		TAILQ_INSERT_TAIL(&_running, prev,Queue);

	}
	if(prev->state == TASK_INTERRUPTIBLE){
		if (prev->signal & ~prev->blocked){
			prev->state = TASK_RUNNING;
		} else {
			timeout = prev->timeout;
			if (timeout && (timeout <= _tickCount)) {
				prev->timeout = 0;
				timeout = 0;
				prev->state = TASK_RUNNING;
			}
		}
	}
	/* if all runnable processes have "counter == 0", re-calculate counters */

	if(prev->state == TASK_RUNNING){
		TAILQ_INSERT_TAIL(&_running, prev,Queue);
	}
	next = TAILQ_FIRST(&_running);
	// modify counters
	int c = -1000;
	TAILQ_FOREACH(p,&_running,Queue) {
		int weight = goodness(p, prev);
		if (weight > c) { c = weight; next = p; }
	}
	if (!c) {
		TAILQ_FOREACH(p,&_running,Queue) {
			p->counter = (p->counter >> 1) + p->priority;
		}
	}
	if(prev!=next) {
		pxCurrentTCB=next; // the switch
	}
}

pid_t launch( void * (* func)(void *),void*parm, size_t stk_size , uint32_t piro,  uint32_t * const buffer) {
	// get the next available pid from the task table
	task_t* task = NULL;
	pid_t pid = -1;
	for ( size_t i = 1; i <= MAX_TASKS; i++ ) {
		if ((_tasks[i].status & TASK_ACTIVE_FLAG)==0 ) {
			task = & _tasks[i];
			task->status |= TASK_ACTIVE_FLAG;
			memaddr_t * stack = buffer ? buffer : reserve_stack( stk_size );
			if ( stack ) {
				pid = task->pid = i;
				task->pxStack = stack;
				task->EventQueue = NULL;
				task->delay = 0;
				task->uxPriority = piro; // don't do anything with this right now.
				task->runTime = 0;
				memset(stack,0,stk_size);
				uint32_t* topOfStack = task->pxStack + (stk_size-(uint16_t)1);
				topOfStack = ( uint32_t * ) ( ( ( uint32_t ) topOfStack ) & ( ~( ( uint32_t ) 0x001f ) ) );
				task->pxTopOfStack = init_stack(topOfStack,func,parm);
				enter_critical();
				_numberOfTasks++;
				if(pxCurrentTCB==NULL) {
					pxCurrentTCB=task;
				} else {
					TAILQ_INSERT_TAIL(&_readyTaskList, task,Queue);
				}

				exit_critical();
				break;
			}
		}
	}
	return -1;
}
#if 0
				stkctx_t * s_ctx = (stkctx_t *)( ( (uint32_t) stack ) - sizeof( stkctx_t ) );
				s_ctx->pc  = (uint32_t) task;
				s_ctx->psr = 0x01000000;              // default PSR value
				tskctx_t * t_ctx = (tskctx_t *)( ( (uint32_t) s_ctx ) - sizeof( tskctx_t ) );
				t_ctx->lr = 0xFFFFFFFD;
				// update task table
				_tasks[pid].stk_end = (memaddr_t *)( ( (uint32_t) stack ) - ( ( sizeof( stkctx_t ) + sizeof( tskctx_t ) ) + ( sizeof( memaddr_t ) * stk_size ) ) );
				_tasks[pid].sp      = (memaddr_t *)( ( (uint32_t) s_ctx ) - sizeof( tskctx_t ) );
				_tasks[pid].status |= TASK_ACTIVE_FLAG;
#endif
void vTaskSwitchContext( void ){
	if(_schedulerSuspended) {
		_yieldPending = true;
	} else {
		_yieldPending = false;
		task_t* ntask = TAILQ_INSERT_TAIL(&_readyTaskList);

	}
}

#if 0
sem_t * sem_open( key_t key ) {
	sem_t * sem = 0;
	SLIST_FOREACH(sem,&_sems,entry) {
		if(sem->key == key) return sem;
	}
	if(sem_used == MAX_SEMS_AVALIABLE) return NULL;
	sem = sem_array + sem_used++;
	memset(sem,0,sizeof(sem_t));
	sem->key = key;
	SLIST_INIT(&sem->tasks);
	SLIST_INSERT_HEAD(&_sems,sem,entry);
	return sem;
}
int sem_wait( sem_t * sem ) {
	pid_t pid = -1;
	task_t* task;
	if ( sem->value == 0 ) {
		SLIST_FOREACH(task,&sem->tasks,semaphore_list) {
			if(task->pid == _current_task ){
				pid = _current_task ;
				break;
			}
		}
		if(pid == -1) {
			task = &_tasks[_current_task];
			pid = _current_task ;
			SLIST_INSERT_HEAD(&sem->tasks,task,semaphore_list);
		}
		// set the task status
		task->status |= TASKS_SEM_WAIT_FLAG;
		// set the scheduler status flag
		_status |= KERNEL_SCHEDULER_FLAG;
		// semaphore wait loop
		while ( sem->value == 0 );
	}
	// consume the semaphore
	sem->value--;
	return 0;
}
int sem_post( sem_t * sem ) {
	if(!SLIST_EMPTY(&sem->tasks)){
		task_t* task = SLIST_FRONT(&sem->tasks);
		SLIST_REMOVE_HEAD(&sem->tasks,semaphore_list);
		task->status ^= TASKS_SEM_WAIT_FLAG;
	}
	// release the semaphore
	sem->value++;
	// set the scheduler status flag
	_status |= KERNEL_SCHEDULER_FLAG;
	return 0;
}
#endif
// os part
void nisos_boot() {
	// disable interrupts
	__disable_irq();
	nisos_kernel_init();

}
void nisos_run() {
	system_task();
}
void PendSV_Handler(void) { // Context switching code
	// save context
	__asm volatile (
		"mrs     r0, msp           \n"
		"push    {r4 - r11, lr}    \n"
		"mov     r11, r0           \n"
	);
	// call the kernel scheduler
	/* This is a naked function. */

	__asm volatile
	(
	"	mrs r0, psp							\n"
	"	isb									\n"
	"										\n"
	"	ldr	r3, pxCurrentTCBConst			\n" /* Get the location of the current TCB. */
	"	ldr	r2, [r3]						\n"
	"										\n"
	"	tst r14, #0x10						\n" /* Is the task using the FPU context?  If so, push high vfp registers. */
	"	it eq								\n"
	"	vstmdbeq r0!, {s16-s31}				\n"
	"										\n"
	"	stmdb r0!, {r4-r11, r14}			\n" /* Save the core registers. */
	"										\n"
	"	str r0, [r2]						\n" /* Save the new top of stack into the first member of the TCB. */
	"										\n"
	"	stmdb sp!, {r3}						\n"
	"	mov r0, %0 							\n"
	"	msr basepri, r0						\n"
	"	dsb									\n"
	"   isb									\n"
	"	bl culinux_scheduler				\n" // changed the schedualer function
	"	mov r0, #0							\n"
	"	msr basepri, r0						\n"
	"	ldmia sp!, {r3}						\n"
	"										\n"
	"	ldr r1, [r3]						\n" /* The first item in pxCurrentTCB is the task top of stack. */
	"	ldr r0, [r1]						\n"
	"										\n"
	"	ldmia r0!, {r4-r11, r14}			\n" /* Pop the core registers. */
	"										\n"
	"	tst r14, #0x10						\n" /* Is the task using the FPU context?  If so, pop the high vfp registers too. */
	"	it eq								\n"
	"	vldmiaeq r0!, {s16-s31}				\n"
	"										\n"
	"	msr psp, r0							\n"
	"	isb									\n"
	"										\n"
	#ifdef WORKAROUND_PMU_CM001 /* XMC4000 specific errata workaround. */
		#if WORKAROUND_PMU_CM001 == 1
	"			push { r14 }				\n"
	"			pop { pc }					\n"
		#endif
	#endif
	"										\n"
	"	bx r14								\n"
	"										\n"
	"	.align 2							\n"
	"pxCurrentTCBConst: .word pxCurrentTCB	\n"
	::"i"(configMAX_SYSCALL_INTERRUPT_PRIORITY)
	);
	scheduler();
}



void SysTick_Handler() {
	_systicks++;
	  HAL_SYSTICK_IRQHandler();
	if ( _status & KERNEL_SCHEDULER_FLAG ) {
		_status   ^= KERNEL_SCHEDULER_FLAG;
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	}
}
// quite posably retarted and not needed
static void prvPortStartFirstTask( void )
{
	__asm volatile(
					" ldr r0, =0xE000ED08 	\n" /* Use the NVIC offset register to locate the stack. */
					" ldr r0, [r0] 			\n"
					" ldr r0, [r0] 			\n"
					" msr msp, r0			\n" /* Set the msp back to the start of the stack. */
					" cpsie i				\n" /* Globally enable interrupts. */
					" cpsie f				\n"
					" dsb					\n"
					" isb					\n"
					" svc 0					\n" /* System call to start first task. */
					" nop					\n"
				);
}
// copied from rtos, we are only using svc 0 sooo we can use other ranges
void SVC_Handler(void)
{
	__asm volatile (
		"	ldr	r3, pxCurrentTCBConst2		\n" /* Restore the context. */
		"	ldr r1, [r3]					\n" /* Use pxCurrentTCBConst to get the pxCurrentTCB address. */
		"	ldr r0, [r1]					\n" /* The first item in pxCurrentTCB is the task top of stack. */
		"	ldmia r0!, {r4-r11, r14}		\n" /* Pop the registers that are not automatically saved on exception entry and the critical nesting count. */
		"	msr psp, r0						\n" /* Restore the task stack pointer. */
		"	isb								\n"
		"	mov r0, #0 						\n"
		"	msr	basepri, r0					\n"
		"	bx r14							\n"
		"									\n"
		"	.align 2						\n"
		"pxCurrentTCBConst2: .word pxCurrentTCB				\n"
	);
}



