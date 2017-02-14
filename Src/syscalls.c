/* Support files for GNU libc.  Files in the system namespace go here.
   Files in the C namespace (ie those that do not start with an
   underscore) go in .c.  */

#include <_ansi.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#include <reent.h>
#include <unistd.h>
#include <sys/wait.h>
#include "usart.h"
#include "ff.h"
#include "stm32469i_discovery_sdram.h"
#include <setjmp.h>

//#define DEBUG_SBRK

//jmp_buf
//#define FreeRTOS
//#define MAX_STACK_SIZE 0x2000
#if 0
 int __io_putchar(int ch) {
	 uint8_t data = (char)ch;
	 HAL_UART_Transmit(&huart3, &data,1,1000);
	 return data;
 }
 int __io_getchar(void) {
	 uint8_t data = 0;
	 HAL_UART_Receive(&huart3, &data,1,1000);
	 	 return data;
 }

#endif

#ifndef FreeRTOS
 //register char * stack_ptr asm("sp");
#endif



void ManualPrintHeap(uint32_t amount, uint32_t left) {
	uart_puts("Heap expanded by ");
	uart_puti(amount,0,UART_MODE_NONE);

	uart_puts(", memory left: ");
	uart_puti(left,0,UART_MODE_NONE);
	uart_puts("\r\n");
}



#define FB_SIZE (800*480*sizeof(uint32_t))
#define FB_SIZE_TOTAL (FB_SIZE*2)
#define SDRAM_START (SDRAM_DEVICE_ADDR + FB_SIZE_TOTAL)
#define SDRAM_END (SDRAM_START - FB_SIZE_TOTAL + SDRAM_DEVICE_SIZE)
caddr_t _sbrk_sdram(int incr){
	static caddr_t _heap_end_of_sdram=0;
	static caddr_t heap_end=0;
	caddr_t prev_heap_end;
	if (heap_end == 0){
		heap_end = (caddr_t)SDRAM_START;
		_heap_end_of_sdram =  (caddr_t)SDRAM_END;
#ifdef DEBUG_SBRK
		uart_puts("_sbrk_sdram called first : ");
		uart_puti((uint32_t)(_heap_end_of_sdram-heap_end),0,UART_MODE_NONE);
		uart_puts("\r\n");
#endif
	}
	prev_heap_end = heap_end;
	if (heap_end + incr > _heap_end_of_sdram)
	{
		uart_puts("Heap and stack collision\n");
		abort();
		errno = ENOMEM;
		return (caddr_t) -1;
	} else {
		heap_end += incr;
#ifdef DEBUG_SBRK
		ManualPrintHeap(incr, (uint32_t)(_heap_end_of_sdram-heap_end));
#endif
	}

	return (caddr_t) prev_heap_end;
}
caddr_t _sbrk_sram(int incr)
{
	extern char end;// asm("end");
	static char *heap_end;
	char *prev_heap_end;
#ifdef FreeRTOS
	*min_stack_ptr;
#endif
	if (heap_end == 0){
		heap_end = &end;
	}

	prev_heap_end = heap_end;

#ifdef FreeRTOS
	/* Use the NVIC offset register to locate the main stack pointer. */
	min_stack_ptr = (char*)(*(unsigned int *)*(unsigned int *)0xE000ED08);
	/* Locate the STACK bottom address */
	min_stack_ptr -= MAX_STACK_SIZE;

	if (heap_end + incr > min_stack_ptr)
#else
	if (heap_end + incr > (char*)__get_MSP())
#endif
	{
		write(1, "Heap and stack collision\n", 25);
		abort();
		errno = ENOMEM;
		return (caddr_t) -1;
	} else {
		heap_end += incr;
		ManualPrintHeap(incr, (uint32_t)((char*)__get_MSP()-heap_end));
	}



	return (caddr_t) prev_heap_end;
}
caddr_t _sbrk(int incr){
	return _sbrk_sdram(incr);
}

void initialise_monitor_handles()
{
}

int _getpid(void)
{
	return 1;
}

int _kill(int pid, int sig)
{
	UNUSED(pid);
	UNUSED(sig);
	errno = EINVAL;
	return -1;
}

void _exit (int status)
{

	_kill(status, -1);
	while (1) {}
}


int _close(int file)
{
	UNUSED(file);
	return -1;
}

int _fstat(int file, struct stat *st)
{
	UNUSED(file);
	st->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int file) {
    switch (file){
    case STDOUT_FILENO:
    case STDERR_FILENO:
    case STDIN_FILENO:
        return 1;
    default:
        //errno = ENOTTY;
        errno = EBADF;
        return 0;
    }
}

int _lseek(int file, int ptr, int dir)
{
	UNUSED(file);
	UNUSED(ptr);
	UNUSED(dir);
	return 0;
}


int _write(int file, char *ptr, int len)
{
	if(file == 1) { // standard output
		uart_write((uint8_t*)ptr,len);
	} else if(file == 2) { // std error
		uart_write((uint8_t*)ptr,len);
	}
	return len;
}

int _read(int file, char *ptr, int len)
{
	if(file == 0) { // standard input
		//if(HAL_UART_Receive(&huart3, (uint8_t*)ptr,len,100) == HAL_OK) return len;
	}

   return len;
}

int _open(char *path, int flags, ...)
{
	UNUSED(path);
	UNUSED(flags);
	/* Pretend like we always fail */
	return -1;
}

int _wait(int *status)
{
	UNUSED(status);
	errno = ECHILD;
	return -1;
}

int _unlink(char *name)
{
	UNUSED(name);
	errno = ENOENT;
	return -1;
}
#if 0

_PTR	_EXFUN_NOTHROW(_malloc_r,(struct _reent *, size_t));
_PTR	_EXFUN_NOTHROW(_calloc_r,(struct _reent *, size_t, size_t));
_VOID	_EXFUN_NOTHROW(_free_r,(struct _reent *, _PTR));
_PTR	_EXFUN_NOTHROW(_realloc_r,(struct _reent *, _PTR, size_t));
_VOID	_EXFUN(_mstats_r,(struct _reent *, char *));

int	_EXFUN(_system_r,(struct _reent *, const char *));
void *
_malloc_r (struct _reent *ptr, size_t size)
{
  return malloc (size);
}
#endif
int _stat(char *file, struct stat *st)
{
	UNUSED(file);
		UNUSED(st);
	st->st_mode = S_IFCHR;
	return 0;
}

int _link(char *old, char *new)
{
	UNUSED(old);
	UNUSED(new);
	errno = EMLINK;
	return -1;
}

int _fork(void)
{
	errno = EAGAIN;
	return -1;
}

int _execve(char *name, char **argv, char **env)
{
	UNUSED(name);
	UNUSED(argv);
	UNUSED(env);
	errno = ENOMEM;
	return -1;
}
