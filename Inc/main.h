
#ifndef __MAIN_H
#define __MAIN_H


#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/time.h>

#include "stm32f4xx_hal.h"
#include "stm32469i_discovery.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	LOG_USER=0,
	LOG_ERROR,
	LOG_WARN,
	LOG_DEBUG
} LogLevelTypeDef;

#define ANSI_ESCAPE_STRING "\033"
#define ANSI_COLOR_FORGROUND_RED 	ANSI_ESCAPE_STRING "[41m"
#define ANSI_COLOR_FORGROUND_YELLOW ANSI_ESCAPE_STRING "[33m"
#define ANSI_COLOR_FORGROUND_WHITE  ANSI_ESCAPE_STRING "[37m"
#define ANSI_COLOR_RESET		    ANSI_ESCAPE_STRING "[0m"

void uart_putc(char c); /// blocking instant put c
void uart_puts(const char* str); // blocking, instant put string
void uart_write(const uint8_t* data, size_t len); // blocking instant put data
void DebugMessage(LogLevelTypeDef level, const char* message);
void PrintDebugMessage(LogLevelTypeDef level, const char* fmt, ...);
void DebugPrintBufferByte(uint8_t* data, size_t length);
#define DbgUsr(...) PrintDebugMessage(LOG_USER,__VA_ARGS__);
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
