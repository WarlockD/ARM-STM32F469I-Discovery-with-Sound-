#include "usbh_fifo.h"
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

// I REALLY didn't want there to be over head in this simple alloc system
// but if I want here are some functions that can make free work
#define USED 1
// flipcode for the win
// http://www.flipcode.com/archives/Simple_Malloc_Free_Functions.shtml

static uint32_t* compact( uint32_t *p, unsigned nsize )
{
       size_t bsize, psize;
       uint32_t *best;

       best = p;
       bsize = 0;

       while( psize = *p, psize )
       {
              if( psize & USED )
              {
                  if( bsize != 0 )
                  {
                      *best = bsize;
                      if( bsize >= nsize )
                      {
                          return best;
                      }
                  }
                  bsize = 0;
                  best = p = (uint32_t*)(((uint8_t*)p) + (psize & ~USED));
              }
              else
              {
                  bsize += psize;
                  p = (uint32_t*)(((uint8_t*)p) + psize );
              }
       }

       if( bsize != 0 )
       {
           *best = bsize;
           if( bsize >= nsize )return best;
       }
       return 0;
}
void staticmem_free(StaticMemTypeDef* m, void *ptr)
{
	if( ptr )
	 {
		uint32_t *p;
		m->FreeMemory += *p;
		 p = (uint32_t *)( (unsigned)ptr - sizeof(uint32_t) );
		 *p &= ~USED;

	 }
}

void *staticmem_alloc(StaticMemTypeDef* m, size_t size , void**user)
{
	//size_t fsize;
	uint32_t *p;
	if( size == 0 ) return 0;

	size  += 3 + sizeof(uint32_t);
	size >>= 2;
	size <<= 2; // aligment VERY IMPORTANT, meh
	assert(m->FreeMemory > size);
	if(m->FreeMemory < size) return NULL;
	p = m->Free;
	*p = size | USED;
	p++;
	m->Free += size;
	m->FreeMemory -= size;
	*m->Free = m->FreeMemory;
	return (void*)p; // easy and simple
}
void *staticmem_realloc(StaticMemTypeDef* m, void* ptr, size_t size){
	if( ptr == NULL) return staticmem_alloc(m,size,NULL);
	uint32_t* mptr = ((uint32_t*)ptr) - 1;
	assert(mptr + *mptr != m->Free); // not the last ptr
	// it is so just free it and reallocate it
	 m->FreeMemory += *mptr;
	 *mptr = m->FreeMemory;
	 m->Free = mptr;
	 return staticmem_alloc(m,size,NULL);
}
void *staticmem_alloc2(StaticMemTypeDef* m, size_t size )
{
     size_t fsize;
     uint32_t *p;
     if( size == 0 ) return 0;

	 size  += 3 + sizeof(uint32_t);
	 size >>= 2;
	 size <<= 2; // aligment VERY IMPORTANT, meh


     if(m->Free == 0 || size > *m->Free)
     {
    	 m->Free = compact( m->Heap, size );
         if( m->Free == 0 ) return 0;
     }

     p = m->Free;
     fsize = *m->Free;

     if( fsize >= size + sizeof(uint32_t) )
     {
    	 m->Free = (uint32_t *)(((uint8_t*)p) + size );
    	 *m->Free = fsize - size;
     }
     else
     {
    	 m->Free = 0;
         size = fsize;
     }

     *p = size | USED;
     m->FreeMemory -= size;
     return (void *)( ((uint8_t*)p) + sizeof(uint32_t) );
}

void staticmem_init(StaticMemTypeDef* m, uint8_t * buf, uint16_t size){
	m->Heap = (uint32_t*)buf;
	m->TotalMemory = size- sizeof(uint32_t);
	staticmem_reset(m);
}
void staticmem_reset(StaticMemTypeDef* m) {
	m->Free = m->Heap;
	*m->Free = 0;
	m->FreeMemory = m->TotalMemory;
}



/**
  * @brief  fifo_init
  *         Initialize FIFO.
  * @param  f: Fifo address
  * @param  buf: Fifo buffer
  * @param  size: Fifo Size
  * @retval none
  */



void fifo_init_static(FIFO_TypeDef * f, uint8_t * buf, uint16_t size) {
	f->lock = 0;
	f->size = size;
	f->buf = buf;
	f->head = f->tail = 0;
}



uint16_t  fifo_read(FIFO_TypeDef * f, void * buf, uint16_t  nbytes)
{
  uint16_t  i;
  uint8_t * p;
  p = (uint8_t*) buf;

  if(f->lock == 0)
  {
    f->lock = 1;
    for(i=0; i < nbytes; i++)
    {
      if( f->tail != f->head )
      {
        *p++ = f->buf[f->tail];
        f->tail++;
        if( f->tail == f->size )
        {
          f->tail = 0;
        }
      } else
      {
        f->lock = 0;
        return i;
      }
    }
  }
  f->lock = 0;
  return nbytes;
}

/**
  * @brief  fifo_write
  *         Read from FIFO.
  * @param  f: Fifo address
  * @param  buf: read buffer
  * @param  nbytes: number of item to write
  * @retval number of written items
  */
uint16_t  fifo_write(FIFO_TypeDef * f, const void * buf, uint16_t  nbytes)
{
  uint16_t  i;
  const uint8_t * p;
  p = (const uint8_t*) buf;
  if(f->lock == 0)
  {
    f->lock = 1;
    for(i=0; i < nbytes; i++)
    {
      if( (f->head + 1 == f->tail) ||
         ( (f->head + 1 == f->size) && (f->tail == 0)) )
      {
        f->lock = 0;
        return i;
      }
      else
      {
        f->buf[f->head] = *p++;
        f->head++;
        if( f->head == f->size )
        {
          f->head = 0;
        }
      }
    }
  }
  f->lock = 0;
  return nbytes;
}
