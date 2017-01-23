#ifndef __USBH_FIFO_H
#define __USBH_FIFO_H

#ifdef __cplusplus
 extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct
{
     uint8_t  *buf;
     uint16_t head;
     uint16_t tail;
     uint16_t size;
     uint32_t lock; // WRITE lock is 0x1 and read is 0x2 mask
} FIFO_TypeDef;

typedef struct {
	uint32_t*	Heap;
	uint32_t*	Free;
	uint16_t 	TotalMemory;
	uint16_t	FreeMemory;
} StaticMemTypeDef;

void staticmem_init(StaticMemTypeDef* m, uint8_t * buf, uint16_t size);
void staticmem_reset(StaticMemTypeDef* m);
void* staticmem_alloc(StaticMemTypeDef* m, size_t size);
// realloc only works on the last pointer used
void* staticmem_realloc(StaticMemTypeDef* m, void* ptr,size_t size);
void staticmem_free(StaticMemTypeDef* m, void* ptr);
size_t staticmem_used(StaticMemTypeDef* m);

void fifo_init(FIFO_TypeDef * f, uint8_t * buf, uint16_t size);

uint16_t  fifo_read(FIFO_TypeDef * f, void * buf, uint16_t  nbytes);
uint16_t  fifo_write(FIFO_TypeDef * f, const void * buf, uint16_t  nbytes);


#ifdef __cplusplus
 }
#endif


#endif
