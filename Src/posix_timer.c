#include "main.h"
#include "stm32f4xx_hal.h"
#include "usart.h"

// we config the system tick and a better timer here
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#include <assert.h>
#define TIMx                           TIM2
#define TIMx_CLK_ENABLE()              __HAL_RCC_TIM2_CLK_ENABLE()
/* Definition for TIMx's NVIC */
#define TIMx_IRQn                      TIM2_IRQn
#define TIMx_IRQHandler                TIM2_IRQHandler

static TIM_HandleTypeDef ClockHandle;
static volatile uint32_t tv_sec = 0;

 volatile struct itimerval itimer_real;

int getitimer(int which, struct itimerval *curr_value){
	if(!curr_value) {
			errno = EFAULT ;
			return -1;
		}
	if(which == ITIMER_VIRTUAL || which == ITIMER_PROF) {
		errno = EINVAL;
		return -1;
	}
	if(curr_value) *curr_value = itimer_real;
	return 0;
}
int setitimer(int which, const struct itimerval *new_value,
                     struct itimerval *old_value){
	if(!new_value) {
		errno = EFAULT ;
		return -1;
	}
	if(which == ITIMER_VIRTUAL || which == ITIMER_PROF) {
		errno = EINVAL;
		return -1;
	}
	if(old_value) *old_value = itimer_real;
	itimer_real = *new_value;
	return 0;
}
void print_time(const char* msg, struct timeval * tp) {
	uart_print("%s %i.%i\r\n", msg, (int)tp->tv_sec,(int)tp->tv_usec);
}
void HAL_SYSTICK_Callback() {
	if(timerisset(&itimer_real.it_value)){
		struct timeval current;
		gettimeofday(&current,NULL);
		if(timercmp(&current, &itimer_real.it_value, >=)){
			raise(SIGALRM);
			if(timerisset(&itimer_real.it_interval)){
				timeradd(&current,&itimer_real.it_interval, &itimer_real.it_value);
				print_time("NEXT: ", &itimer_real.it_value);
			} else {
				timerclear(&itimer_real.it_value);
			}
		}
	}
}

// https://github.com/micropython/micropython/blob/master/stmhal/timer.c
// Get the frequency (in Hz) of the source clock for the given timer.
// On STM32F405/407/415/417 there are 2 cases for how the clock freq is set.
// If the APB prescaler is 1, then the timer clock is equal to its respective
// APB clock.  Otherwise (APB prescaler > 1) the timer clock is twice its
// respective APB clock.  See DM00031020 Rev 4, page 115.
uint32_t timer_get_source_freq(uint32_t tim_id) {
    uint32_t source;
    if (tim_id == 1 || (8 <= tim_id && tim_id <= 11)) {
        // TIM{1,8,9,10,11} are on APB2
        source = HAL_RCC_GetPCLK2Freq();
        if ((uint32_t)((RCC->CFGR & RCC_CFGR_PPRE2) >> 3) != RCC_HCLK_DIV1) {
            source *= 2;
        }
    } else {
        // TIM{2,3,4,5,6,7,12,13,14} are on APB1
        source = HAL_RCC_GetPCLK1Freq();
        if ((uint32_t)(RCC->CFGR & RCC_CFGR_PPRE1) != RCC_HCLK_DIV1) {
            source *= 2;
        }
    }
    //__HAL_TIM_GET_COUNTER
    return source;

}
volatile uint8_t sssf = 0;
void test_signal(int meh){
	sssf = 1;
}
int _times(struct tms *buf)
{
	if(buf) {
		buf->tms_stime  = tv_sec;
		buf->tms_cstime  = 0;
		buf->tms_stime = 0;
		buf->tms_cstime = 0;
		return 0;
	}
	errno = EACCES;
	return -1;
}
#if 0

int _EXFUN(setitimer, (int __which, const struct itimerval *__restrict __value,
					struct itimerval *__restrict __ovalue));
int _EXFUN(utimes, (const char *__path, const struct timeval *__tvp));


int _EXFUN(adjtime, (const struct timeval *, struct timeval *));
int _EXFUN(futimes, (int, const struct timeval *));
int _EXFUN(futimesat, (int, const char *, const struct timeval [2]));
int _EXFUN(lutimes, (const char *, const struct timeval *));
int _EXFUN(settimeofday, (const struct timeval *, const struct timezone *));

int _EXFUN(getitimer, (int __which, struct itimerval *__value));
int _EXFUN(gettimeofday, (struct timeval *__restrict __p,
			  void *__restrict __tz));
#endif

int _gettimeofday (struct timeval * tp, struct timezone * tzp)
{
  /* Return fixed data for the timezone.  */
  if (tzp) tzp->tz_dsttime = tzp->tz_minuteswest = 0;
  if(tp) {
	  __HAL_TIM_DISABLE_IT(&ClockHandle, TIM_IT_UPDATE);
	  tp->tv_usec = __HAL_TIM_GET_COUNTER(&ClockHandle);
	  tp->tv_sec = tv_sec;
	  __HAL_TIM_ENABLE_IT(&ClockHandle, TIM_IT_UPDATE);
  }
  return 0;
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	tv_sec++;
}

void TIMx_IRQHandler(void)
{

   HAL_TIM_IRQHandler(&ClockHandle);
}


void SystemTimerConfig() {
	uint32_t  uwPrescalerValue = (uint32_t)((timer_get_source_freq(2) / 1000000) - 1); // 1us
	/* Set TIMx instance */
	ClockHandle.Instance = TIMx;
	ClockHandle.Init.Period            = 999999; /* 1 second */
	ClockHandle.Init.Prescaler         = uwPrescalerValue;
	ClockHandle.Init.ClockDivision     = 0;
	ClockHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
	ClockHandle.Init.RepetitionCounter = 0;
	TIMx_CLK_ENABLE();
	assert (HAL_TIM_Base_Init(&ClockHandle) == HAL_OK);
	__HAL_TIM_ENABLE_IT(&ClockHandle, TIM_IT_UPDATE);
	__HAL_TIM_ENABLE(&ClockHandle);
	//assert(HAL_TIM_Base_Start_IT(&ClockHandle) == HAL_OK);
	HAL_NVIC_SetPriority(TIMx_IRQn, 3, 0); //Enable the TIMx global Interrupt
	HAL_NVIC_EnableIRQ(TIMx_IRQn); //should sync this better
}
