#include "i_internal_stm32.h"

#include "opl/dbopl.h"
#include "opl/opl_timer.h"
#include "opl/opl.h"
#include "opl/opl_internal.h"
#include "opl/opl_queue.h"





typedef struct channel_s {
	sfxinfo_t* sfxinfo; // sound being played now
	int lumpnumber;
	int samplerate;
	int bit_size;
	uint8_t* data; // current position being played
	uint8_t* data_end; // end of data being played
	int seperation;
	int volume;
	// the channel left volume lookup
	const int16_t*		leftvol_lookup;

	// the channel right volume lookup
	const int16_t*		rightvol_lookup;
} channel_t;




// mixing buffer

volatile int16_t* mixbuffer = NULL; // this size is MIXBUFFERSIZE*2

volatile uint8_t half_completed = 0;
static int audio_volume = 128; // 255 for STM32
static boolean sound_initialized = false;

uint8_t left_vol=70, right_vol = 70;
boolean use_sfx_prefix = true;
//
// Department of Redundancy Department.
//



static channel_t channels[NUM_CHANNELS];
extern const int steptable[256];
extern const short vol_lookup[128 * 256];


// When a sound stops, check if it is still playing.  If it is not,
// we can mark the sound data as CACHE to be freed back for other
// means.

static void ReleaseSoundOnChannel(int channel)
{
    sfxinfo_t *sfxinfo = channels[channel].sfxinfo;

    if (sfxinfo == NULL)return;

    W_ReleaseLumpNum(sfxinfo->lumpnum);
    channels[channel].sfxinfo = NULL;
    channels[channel].data = NULL;
    channels[channel].data_end = NULL;
}




static void GetSfxLumpName(sfxinfo_t *sfx, char *buf, size_t buf_len)
{
    // Linked sfx lumps? Get the lump number for the sound linked to.

    if (sfx->link != NULL)
    {
        sfx = sfx->link;
    }

    // Doom adds a DS* prefix to sound lumps; Heretic and Hexen don't
    // do this.

    if (use_sfx_prefix)
    {
        M_snprintf(buf, buf_len, "ds%s", DEH_String(sfx->name));
    }
    else
    {
        M_StringCopy(buf, DEH_String(sfx->name), buf_len);
    }
}



static void I_STM32_PrecacheSounds(sfxinfo_t *sounds, int num_sounds)
{
    // no-op
}



//
// Retrieve the raw data lump index
//  for a given SFX name.
//

static int I_STM32_GetSfxLumpNum(sfxinfo_t *sfx)
{
    char namebuf[9];

    GetSfxLumpName(sfx, namebuf, sizeof(namebuf));

    return W_GetNumForName(namebuf);
}

static void UpdateSoundParams(channel_t* channel, int vol, int sep){
	int leftvol, rightvol;
	channel->seperation = sep;
	channel->volume = vol;
	sep += 1;
	leftvol = vol - (vol*sep*sep)/(256*256); // (x^2 seperation)
	sep = sep - 257;
	rightvol = vol - (vol*sep*sep)/(256*256);	 // (x^2 seperation)
	// sanity check
	if (rightvol < 0 || rightvol > 127) I_Error("rightvol out of bounds");
	if (leftvol < 0 || leftvol > 127) I_Error("leftvol out of bounds");
	channel->leftvol_lookup = &vol_lookup[leftvol*256];
	channel->rightvol_lookup = &vol_lookup[rightvol*256];
}
static void I_STM32_UpdateSoundParams(int handle, int vol, int sep)
{
    if (!sound_initialized || handle < 0 || handle >= NUM_CHANNELS)return;
    channel_t* channel = &channels[handle];
    if(channel->sfxinfo==NULL) return;
    UpdateSoundParams(channel,vol,sep);
}


static int I_STM32_StartSound(sfxinfo_t *sfxinfo, int chan, int vol, int sep)
{
	if (!sound_initialized || chan < 0 || chan >= NUM_CHANNELS) return -1;

	// Release a sound effect if there is already one playing
	// on this channel
	ReleaseSoundOnChannel(chan);
	// que up the sound
	uint8_t* data = W_CacheLumpNum(sfxinfo->lumpnum, PU_STATIC);
	if(data == NULL) return -1;
	int   lumplen = W_LumpLength(sfxinfo->lumpnum);
	// Check the header, and ensure this is a valid sound
    if (lumplen < 8 || data[0] != 0x03 || data[1] != 0x00)  return -1;
    int samplerate = (data[3] << 8) | data[2];
    int	length = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) | data[4];
    if (length > lumplen - 8 || length <= 48) return -1;
    data += 16;
 	length -= 32;

	channel_t* channel = &channels[chan];
	channel->data = data + 8;
	channel->data_end = channel->data + length;
	channel->samplerate = samplerate;
	channel->bit_size = 8;

	// set separation, etc.
	UpdateSoundParams(channel,vol,sep);
	channel->sfxinfo =sfxinfo; // this starts it up, this should be atomic
	return chan;
}


static void I_STM32_StopSound(int handle)
{
    if (!sound_initialized || handle < 0 || handle >= NUM_CHANNELS) return;
    // not really used
    ReleaseSoundOnChannel(handle);
}


static boolean I_STM32_SoundIsPlaying(int handle)
{
    if (!sound_initialized || handle < 0 || handle >= NUM_CHANNELS)return false;
    return channels[handle].sfxinfo != NULL;

}


void BSP_AUDIO_OUT_Error_CallBack()
{
	uint32_t last_tick = HAL_GetTick()+500;
	BSP_LED_Toggle(LED_ORANGE);
	  while (1)
	  {
		  uint32_t current = HAL_GetTick();
		  if(current > last_tick) {
			  last_tick = current + 500;
			  BSP_LED_Toggle(LED_RED);
		  }
	  }
}
typedef enum {
	BUFFER_STATE_INIT,
	BUFFER_STATE_PLAYING,
	BUFFER_STATE_HALF_COMPLETE,
	BUFFER_STATE_FULL_COMPLETE,
} buffer_state_t;

volatile buffer_state_t buffer_state = BUFFER_STATE_INIT;
volatile uint32_t sound_processing_time = 0;
volatile uint32_t sound_processing_count = 0;

static void _I_STM32_Cortex_M4_UpdateSound(void){
	if(buffer_state == BUFFER_STATE_PLAYING) return;
	int16_t* buffer = (buffer_state == BUFFER_STATE_HALF_COMPLETE) ? (int16_t*)(&mixbuffer[0]) : (int16_t*)(&mixbuffer[MIXBUFFERSIZE/2]);
	uint32_t* words = (uint32_t*)buffer;
	uint32_t* end_words = words +  SAMPLECOUNT;
	while(words != end_words){
		register uint32_t out=0;
		register uint32_t tmp=0;
		for (int chan = 0; chan < NUM_CHANNELS; chan++ )
		{
			channel_t* channel = channels + chan;
			if(channel->sfxinfo == NULL) continue; // Check channel, if active.
			uint8_t sample = channel->data[0];// << 8 |  channel->data[1];
			tmp = channel->leftvol_lookup[sample] << 16 | channel->rightvol_lookup[sample];

			//__SSAT16(ARG1,ARG2)
			__asm ("ssat16 %0, %1, %2" : "=r" (out) :  "I" (15), "r" (tmp) );
			channel->data++;
			if (channel->data >= channel->data_end) ReleaseSoundOnChannel(chan);
		}
		*words++ = out;
	}
	buffer_state = BUFFER_STATE_PLAYING;
}

static void _I_STM32_UpdateSound(void)
{
	if(buffer_state == BUFFER_STATE_PLAYING) return;

	uint32_t ticks = HAL_GetTick();
	  // Pointers in global mixbuffer, left, right, end.
		register unsigned int	sample;
	    register int		dl;
	    register int		dr;
	    int16_t* 		buffer ;
	    int16_t* 		leftout;
	    int16_t* 		rightout;
	    int16_t* 		leftend;

	    buffer = (buffer_state == BUFFER_STATE_HALF_COMPLETE) ? (int16_t*)(&mixbuffer[0]) : (int16_t*)(&mixbuffer[MIXBUFFERSIZE/2]);
	    leftout = buffer;			// Left and right channel
	   	rightout = buffer+1;		 //  are in global mixbuffer, alternating.
	   	int32_t* music = NULL;
	   	leftend= buffer + (MIXBUFFERSIZE/2);// Step in mixbuffer, left and right, thus two.

		// Low-pass filter for cutoff frequency f:
		//
		// For sampling rate r, dt = 1 / r
		// rc = 1 / 2*pi*f
		// alpha = dt / (rc + dt)

		// Filter to the half sample rate of the original sound effect
		// (maximum frequency, by nyquist)

		int active_channels=0;
	    // Mix sounds into the mixing buffer.
	    // Loop over step*SAMPLECOUNT,
	    //  that is 512 values for two channels.
	    while (!(leftout >= leftend))
	    {
	    	active_channels=0;
	    	dl = 0 ;
	        dr =  0;
		// Reset left/right value.


		// Love thy L2 chache - made this a loop.
		// Now more channels could be set at compile time
		//  as well. Thus loop those  channels.

		for (int chan = 0; chan < NUM_CHANNELS; chan++ )
		{
			channel_t* channel = channels + chan;
			if(channel->sfxinfo == NULL) continue; // Check channel, if active.
			active_channels++;
			sample = channel->data[0];// << 8 |  channel->data[1];



			dl += channel->leftvol_lookup[sample];
			dr += channel->rightvol_lookup[sample];
			channel->data++;
			if (channel->data >= channel->data_end){

				ReleaseSoundOnChannel(chan);
			}
		}

		if(active_channels==0) {
			while (leftout != leftend){
				*leftout = music == NULL ? 0 : (int16_t)(*music++);
				*rightout = music == NULL ? 0 : (int16_t)(*music++);
				leftout += 2; rightout += 2;
			}
			break; // no audio or sound, so don't bother
		}


		// Clamp to range. Left hardware channel.
		// Has been char instead of short.
		// if (dl > 127) *leftout = 127;
		// else if (dl < -128) *leftout = -128;
		// else *leftout = dl;

		if (dl > 0x7fff) dl = 0x7fff;
		else if (dl < -0x8000) dl = -0x8000;

		// Same for right hardware channel.
		if (dr > 0x7fff)dl = 0x7fff;
		else if (dr < -0x8000)  *rightout = -0x8000;

			*leftout = dl;
			*rightout = dr;

			leftout += 2;
			rightout += 2;
	    }
	    ticks = HAL_GetTick()-ticks;
	//if(active_channels >0) printf("Sound processing time %ul chan=%i\r\n", ticks,active_channels);
	buffer_state = BUFFER_STATE_PLAYING;
}
//
// Periodically called to update the sound system
//
//volatile uint32_t s_count = 0;

void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
	if(buffer_state == BUFFER_STATE_PLAYING) {
		buffer_state=BUFFER_STATE_HALF_COMPLETE;
		_I_STM32_Cortex_M4_UpdateSound();
		//_I_STM32_UpdateSound();
	}
}


void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
	if(buffer_state == BUFFER_STATE_PLAYING) {
		buffer_state=BUFFER_STATE_FULL_COMPLETE;
		_I_STM32_Cortex_M4_UpdateSound();
				//_I_STM32_UpdateSound();
	}
}
static void I_STM32_ShutdownSound();


static void I_STM32_UpdateSound()
{
	if (!sound_initialized) return;

	if(buffer_state == BUFFER_STATE_INIT) { // first time

		buffer_state = BUFFER_STATE_HALF_COMPLETE;
		_I_STM32_Cortex_M4_UpdateSound();
				//_I_STM32_UpdateSound();
		 if(BSP_AUDIO_OUT_Play((uint16_t*)mixbuffer, MIXBUFFERSIZE*sizeof(int16_t))!=0) {
			printf("BSP_AUDIO_OUT_Play: Unable to set up sound.\r\n");
			I_STM32_ShutdownSound();
		}
	//	 return;
	}

	//_I_STM32_UpdateSound();
}

static void I_STM32_ShutdownSound()
{
	printf("I_STM32_ShutdownSound()\r\n");
    if (!sound_initialized) return;
    free(mixbuffer);
    mixbuffer = NULL;
    sound_initialized = false;
}


static boolean I_STM32_InitSound(boolean _use_sfx_prefix)
{
    printf("I_STM32_InitSound(): ");
    use_sfx_prefix = _use_sfx_prefix;
    memset(channels,0,sizeof(channels));// MIXBUFFERSIZE// MIXBUFFERSIZE2
    mixbuffer = (volatile int16_t*)malloc(MIXBUFFERSIZE*sizeof(int16_t));
    if(mixbuffer == NULL) {
    	printf("unable to allocate mixbuffer\r\n");
    	return false;
    }
    memset((int8_t*)mixbuffer,0,MIXBUFFERSIZE*sizeof(int16_t));
    printf("mixbuffer_size=%i ", (int)(MIXBUFFERSIZE*sizeof(int16_t)));
    /* Initialize the Audio codec and all related peripherals (I2S, I2C, IOExpander, IOs...) */
    if(BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_BOTH, audio_volume=70, MIXER_FREQUENCY) != AUDIO_OK)
    {
    	fprintf(stderr, "BSP_AUDIO_OUT_Init() Unable to set up sound.\r\n");
    	return false;
    }
      BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);
      BSP_AUDIO_OUT_SetOutputMode(BSP_AUDIO_OUT_STEREOMODE|BSP_AUDIO_OUT_CIRCULARMODE);

      buffer_state = BUFFER_STATE_INIT;

    printf("Success!\r\n ");
    sound_initialized = true;
    return true;
}

static snddevice_t sound_stm32_devices[] =
{
    SNDDEVICE_SB,
    SNDDEVICE_PAS,
    SNDDEVICE_GUS,
    SNDDEVICE_WAVEBLASTER,
    SNDDEVICE_SOUNDCANVAS,
    SNDDEVICE_AWE32,
};

sound_module_t sound_stm32_module =
{
		sound_stm32_devices,
    arrlen(sound_stm32_devices),
    I_STM32_InitSound,
    I_STM32_ShutdownSound,
    I_STM32_GetSfxLumpNum,
    I_STM32_UpdateSound,
    I_STM32_UpdateSoundParams,
    I_STM32_StartSound,
    I_STM32_StopSound,
    I_STM32_SoundIsPlaying,
    I_STM32_PrecacheSounds,
};



