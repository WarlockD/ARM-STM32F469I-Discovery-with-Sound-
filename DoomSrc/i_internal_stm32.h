/*
 * i_internal.stm32.h
 *
 *  Created on: Jan 12, 2017
 *      Author: Paul
 */

#ifndef I_INTERNAL_STM32_H_
#define I_INTERNAL_STM32_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include <chocdoom\i_sound.h>

#include "chocdoom\deh_str.h"
#include "chocdoom\i_sound.h"
#include "chocdoom\i_system.h"
#include "chocdoom\i_swap.h"
#include "chocdoom\m_argv.h"
#include "chocdoom\m_misc.h"
#include "chocdoom\w_wad.h"
#include "chocdoom\z_zone.h"
#include "chocdoom\i_timer.h"
#include "chocdoom\doomtype.h"
#define MIX_MAX_VOLUME 127


#include "stm32469i_discovery_audio.h"

#define LOW_PASS_FILTER
//#define DEBUG_DUMP_WAVS
#define NUM_CHANNELS 8

// So, if DOOM operates at 30 frames a second
// we have to make a sound buffer atleast 2 times this
// also, we will need to make TWO.  sort of a back buffer to be mixed
// when a new sound comes in or if we are killing off another sound
// This is the built in buffer frequency

#define BUFFER_TIME_SIZE_MS 30

#define MIXER_FREQUENCY   AUDIO_FREQUENCY_11K
//#define MIXER_FREQUENCY  ((uint32_t)11025)
#define SAMPLECOUNT		(uint32_t)((float)MIXER_FREQUENCY* ((float)BUFFER_TIME_SIZE_MS/1000.0f))    //1024
// two channels, and second buffer for the circle
#define MIXBUFFERSIZE	(SAMPLECOUNT*2*2)

#endif /* I_INTERNAL_STM32_H_ */
