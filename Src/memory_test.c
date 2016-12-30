/* USER CODE BEGIN Includes */
#include "stm32469i_discovery.h"
#include "stm32469i_discovery_sdram.h"
#include <string.h>
#include <time.h>
#include <stdlib.h>

static uint32_t* memory_start = (uint32_t*)SDRAM_DEVICE_ADDR;

int randint(int n) {
  if ((n - 1) == RAND_MAX) {
    return rand();
  } else {
    // Chop off all of the values that would cause skew...
    long end = RAND_MAX / n; // truncate skew
    assert (end > 0L);
    end *= n;

    // ... and ignore results from rand() that fall above that limit.
    // (Worst case the loop condition should succeed 50% of the time,
    // so we can expect to bail out of this loop pretty quickly.)
    int r;
    while ((r = rand()) >= end);

    return r % n;
  }
}
void FillMemory(uint32_t value, uint32_t inc) {
	printf("Filling memory with 0x%8.8X\r\n", value);
	for(uint32_t i=0; i < SDRAM_DEVICE_SIZE/4; i++){
		memory_start[i] = value;
		value+= inc;
	}

}
uint8_t TestMemory(uint32_t value, uint32_t inc) {
	printf("Testing memory with 0x%8.8X\r\n", value);
	for(uint32_t i=0; i < SDRAM_DEVICE_SIZE/4; i++) {
		if(memory_start[i] != value) {
			printf("MEMORY FAILURE 0x%8.8X:  read 0x%8.8X instead of 0x%8.8X\r\n", (uint32_t)(memory_start + i),memory_start[i],value);
			return 0;
		}
		value+= inc;
	}
	return 1;
}
void RandomFill(uint32_t seed) {
	printf("Filling memory with random values with starting seed 0x%8.8X\r\n", seed);
	srand(seed);
	for(uint32_t i=0; i < SDRAM_DEVICE_SIZE/4; i++){
		memory_start[i] = rand();
	}
}
uint8_t RandomTest(uint32_t seed) {
	printf("Testing memory with random values with starting seed 0x%8.8X\r\n", seed);
	srand(seed);
	for(uint32_t i=0; i < SDRAM_DEVICE_SIZE/4; i++){
		uint32_t value =  rand();
		if(memory_start[i] != value) {
					printf("MEMORY FAILURE 0x%8.8X:  read 0x%8.8X instead of 0x%8.8X\r\n", (uint32_t)(memory_start + i),memory_start[i],value);
					return 0;
		}
	}
	return 1;
}
void MemoryTest() {
	printf("Memory Test start\r\n");

	printf("Memory size is %lu\r\n", SDRAM_DEVICE_SIZE); //(int)(memory_end- memory_start) *4);
	FillMemory(0,0);
	if(!TestMemory(0,0))  return;
	FillMemory(0xFFFFFFFF,0);
	if(!TestMemory(0xFFFFFFFF,0))  return;
	FillMemory(0,1);
	if(!TestMemory(0,1))  return;
	RandomFill(0);
	if(!RandomTest(0))  return;
	printf("All finished and passed!\r\n");
}
