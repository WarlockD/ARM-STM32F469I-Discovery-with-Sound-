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

#include <sys\types.h>

typedef void* sem_t;
extern void nisos_boot();
extern void nisos_run();

// semaphore interface
sem_t* sem_open( key_t key );
int sem_init( sem_t * sem, unsigned int value );
int sem_wait( sem_t * sem );
int sem_post( sem_t * sem );
pid_t launch( void * (* task)(void *), size_t stk_size );



#endif /* !KERNEL_KERNEL_H */
