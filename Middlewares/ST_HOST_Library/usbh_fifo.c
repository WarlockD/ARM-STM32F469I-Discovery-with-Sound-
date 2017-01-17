#include "usbh_fifo.h"

/**
  * @brief  fifo_init
  *         Initialize FIFO.
  * @param  f: Fifo address
  * @param  buf: Fifo buffer
  * @param  size: Fifo Size
  * @retval none
  */
void fifo_init(FIFO_TypeDef * f, uint8_t * buf, uint16_t size)
{
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
