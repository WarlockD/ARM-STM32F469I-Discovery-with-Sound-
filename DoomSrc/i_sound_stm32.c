
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

//music_module_t music_stm32_module;
typedef struct chunk_s {
	uint8_t* abuf;
	size_t alen;
	uint8_t allocated;
	int samplerate;
	int bit_size;
	uint8_t volume;
} chunk_t;

// Load and convert a sound effect
// Returns true if successful
typedef struct sfxlump_s {
	uint16_t type;
	uint16_t samplerate;
	uint32_t length;
	const int8_t* data;
} sfxlump_t ;

typedef struct allocated_sound_s
{
    sfxinfo_t *sfxinfo;
    chunk_t chunk;
    int use_count;
    struct allocated_sound_s *prev, *next;
}allocated_sound_t ;

static allocated_sound_t *allocated_sounds_head = NULL;
static allocated_sound_t *allocated_sounds_tail = NULL;
static int allocated_sounds_size = 0;


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
#define LOW_PASS_FILTER
//#define DEBUG_DUMP_WAVS
#define NUM_CHANNELS 8

// So, if DOOM operates at 30 frames a second
// we have to make a sound buffer atleast 2 times this
// also, we will need to make TWO.  sort of a back buffer to be mixed
// when a new sound comes in or if we are killing off another sound
// This is the built in buffer frequency

#define BUFFER_TIME_SIZE_MS 30
#define MIXER_FREQUENCY  ((uint32_t)11025)
#define SAMPLECOUNT		1024
// two channels, and second buffer for the circle
#define MIXBUFFERSIZE	(SAMPLECOUNT*2*2)

// mixing buffer

volatile int16_t* mixbuffer = NULL; // this size is MIXBUFFERSIZE*2

volatile uint8_t half_completed = 0;
static int audio_volume = 128; // 255 for STM32
static boolean sound_initialized = false;
int use_libsamplerate = 0;
uint8_t left_vol=70, right_vol = 70;
boolean use_sfx_prefix = true;
//
// Department of Redundancy Department.
//



static channel_t channels[NUM_CHANNELS];
extern const int steptable[256];
extern const short vol_lookup[128 * 256];


static void AllocatedSoundLink(allocated_sound_t *snd)
{
    snd->prev = NULL;
    snd->next = allocated_sounds_head;
    allocated_sounds_head = snd;
    if (allocated_sounds_tail == NULL) allocated_sounds_tail = snd;
    else snd->next->prev = snd;
}

// Unlink a sound from the linked list.

static void AllocatedSoundUnlink(allocated_sound_t *snd)
{
    if (snd->prev == NULL) allocated_sounds_head = snd->next;
    else snd->prev->next = snd->next;
    if (snd->next == NULL)  allocated_sounds_tail = snd->prev;
    else snd->next->prev = snd->prev;
}

static void FreeAllocatedSound(allocated_sound_t *snd)
{
    AllocatedSoundUnlink(snd);  // Unlink from linked list.
    snd->sfxinfo->driver_data = NULL; // Unlink from higher-level code.
    allocated_sounds_size -= snd->chunk.alen; // Keep track of the amount of allocated sound data:
    Z_Free(snd);
}

// Search from the tail backwards along the allocated sounds list, find
// and free a sound that is not in use, to free up memory.  Return true
// for success.

static boolean FindAndFreeSound(void)
{
    allocated_sound_t *snd;

    snd = allocated_sounds_tail;

    while (snd != NULL)
    {
        if (snd->use_count == 0)
        {
            FreeAllocatedSound(snd);
            return true;
        }

        snd = snd->prev;
    }

    // No available sounds to free...

    return false;
}

// Enforce SFX cache size limit.  We are just about to allocate "len"
// bytes on the heap for a new sound effect, so free up some space
// so that we keep allocated_sounds_size < snd_cachesize

static void ReserveCacheSpace(size_t len)
{
    if (snd_cachesize <= 0) return;
    // Keep freeing sound effects that aren't currently being played,
    // until there is enough space for the new sound.

    // Free a sound.  If there is nothing more to free, stop.
    while (allocated_sounds_size + len > snd_cachesize)
    	if (!FindAndFreeSound())  break;

}

// Allocate a block for a new sound effect.

static chunk_t *AllocateSound(sfxinfo_t *sfxinfo, size_t len)
{
    allocated_sound_t *snd;

    // Keep allocated sounds within the cache size.

    ReserveCacheSpace(len);

    // Allocate the sound structure and data.  The data will immediately
    // follow the structure, which acts as a header.

    do
    {
        snd = Z_Malloc(sizeof(allocated_sound_t) + len,PU_STATIC,NULL);

        // Out of memory?  Try to free an old sound, then loop round
        // and try again.

        if (snd == NULL && !FindAndFreeSound())
        {
            return NULL;
        }


    } while (snd == NULL);
    printf("Allocated more sounds: len=%i total=%i\r\n", len, allocated_sounds_size);
    // Skip past the chunk structure for the audio buffer

    snd->chunk.abuf = (byte *) (snd + 1);
    snd->chunk.alen = len;
    snd->chunk.allocated = 1;
    snd->chunk.volume = MIX_MAX_VOLUME;

    snd->sfxinfo = sfxinfo;
    snd->use_count = 0;

    // driver_data pointer points to the allocated_sound structure.

    sfxinfo->driver_data = snd;

    // Keep track of how much memory all these cached sounds are using...

    allocated_sounds_size += len;

    AllocatedSoundLink(snd);

    return &snd->chunk;
}

// Lock a sound, to indicate that it may not be freed.

static void LockAllocatedSound(allocated_sound_t *snd)
{
    // Increase use count, to stop the sound being freed.
    ++snd->use_count;
    //printf("++ %s: Use count=%i\n", snd->sfxinfo->name, snd->use_count);

    // When we use a sound, re-link it into the list at the head, so
    // that the oldest sounds fall to the end of the list for freeing.
    AllocatedSoundUnlink(snd);
    AllocatedSoundLink(snd);
}

// Unlock a sound to indicate that it may now be freed.

static void UnlockAllocatedSound(allocated_sound_t *snd)
{
    if (snd->use_count <= 0)
        I_Error("Sound effect released more times than it was locked...");

    --snd->use_count;

    //printf("-- %s: Use count=%i\n", snd->sfxinfo->name, snd->use_count);
}

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
  //  UnlockAllocatedSound(sfxinfo->driver_data);
}

static boolean CacheSFX(sfxinfo_t *sfxinfo);
static boolean LockSound(sfxinfo_t *sfxinfo)
{
    // If the sound isn't loaded, load it now

    if (sfxinfo->driver_data == NULL)
        if (!CacheSFX(sfxinfo))  return false;

    LockAllocatedSound(sfxinfo->driver_data);

    return true;
}

// no expansion, just a copy
static boolean CopySoundData_STM32(sfxinfo_t *sfxinfo,
                                   byte *data,
                                   int samplerate,
                                   int length){

	chunk_t * chunk = AllocateSound(sfxinfo, length);
	if(chunk == NULL)return false;
	memcpy(chunk->abuf,data,length);
	chunk->samplerate = samplerate;
	chunk->bit_size = 8;
	return true;
}

// Generic sound expansion function for any sample rate.
// Returns number of clipped samples (always 0).

static boolean ExpandSoundData_STM32(sfxinfo_t *sfxinfo,
                                   byte *data,
                                   int samplerate,
                                   int length)
{
    chunk_t *chunk;
    uint32_t expanded_length;

    // Calculate the length of the expanded version of the sample.

    expanded_length = (uint32_t) ((((uint64_t) length) * MIXER_FREQUENCY) / samplerate);
    expanded_length = (uint32_t)(length*2*2); // just double it with sterio
    // Double up twice: 8 -> 16 bit and mono -> stereo

   // expanded_length *= 4;
    printf("Expanding sound '%s' length=%i expanded_length=%lu mixer=%i sample=%i\r\n",sfxinfo->name, length,expanded_length,MIXER_FREQUENCY, samplerate);
    // Allocate a chunk in which to expand the sound

    chunk = AllocateSound(sfxinfo, expanded_length);

    if (chunk == NULL) return false;
    // dumb sign convert no filter

    // If we can, use the standard / optimized SDL conversion routines.
#if 0
    if (samplerate <= MIXER_FREQUENCY
     && ConvertibleRatio(samplerate, MIXER_FREQUENCY)
     && SDL_BuildAudioCVT(&convertor,
                          AUDIO_U8, 1, samplerate,
                          mixer_format, mixer_channels, mixer_freq))
    {
        convertor.buf = chunk->abuf;
        convertor.len = length;
        memcpy(convertor.buf, data, length);

        SDL_ConvertAudio(&convertor);
    }
    else
#endif
    {

        int16_t *expanded = (int16_t *) chunk->abuf;
        int expanded_length;
        int expand_ratio;
        int i;

        // Generic expansion if conversion does not work:
        //
        // SDL's audio conversion only works for rate conversions that are
        // powers of 2; if the two formats are not in a direct power of 2
        // ratio, do this naive conversion instead.

        // number of samples in the converted sound

        expanded_length = ((uint64_t) length * MIXER_FREQUENCY) / samplerate;
        expand_ratio = (length << 8) / expanded_length;

        for (i=0; i<expanded_length; ++i)
        {
            int16_t sample;
            int src;
            uint8_t usample = data[src];

           // src = (i * expand_ratio) >> 8;
           // sample = (data[src] - 0x80) << 8;
            //sample = usample | (usample << 8);
           // sample -= 32768;

            // expand 8->16 bits, mono->stereo

            expanded[i * 2] = vol_lookup[80*256+usample];
            expanded[i * 2+1] = vol_lookup[80*256+usample];

			// expanded[i * 2]=expanded[i * 2 + 1] = sample;
        }

#if 0
        LOW_PASS_FILTER
        // Perform a low-pass filter on the upscaled sound to filter
        // out high-frequency noise from the conversion process.

        {
            float rc, dt, alpha;

            // Low-pass filter for cutoff frequency f:
            //
            // For sampling rate r, dt = 1 / r
            // rc = 1 / 2*pi*f
            // alpha = dt / (rc + dt)

            // Filter to the half sample rate of the original sound effect
            // (maximum frequency, by nyquist)

            dt = 1.0f / MIXER_FREQUENCY;
            rc = 1.0f / (3.14f * samplerate);
            alpha = dt / (rc + dt);

            // Both channels are processed in parallel, hence [i-2]:

            for (i=2; i<expanded_length * 2; ++i)
            {
                expanded[i] = (int16_t) (alpha * expanded[i]
                                      + (1 - alpha) * expanded[i-2]);
            }
        }
#endif /* #ifdef LOW_PASS_FILTER */
    }
    chunk->samplerate = MIXER_FREQUENCY;
    chunk->bit_size = 16;
    return true;
}


//#ifdef DEBUG_DUMP_WAVS

// Debug code to dump resampled sound effects to WAV files for analysis.

static void WriteWAV(const char *filename, byte *data,
                     uint32_t length, int samplerate)
{
    FIL f_wav;
    unsigned int i;
    unsigned short s;
    UINT written;
    FRESULT res;
    if(f_open(&f_wav, filename, FA_WRITE|FA_CREATE_NEW|FA_CREATE_ALWAYS)==FR_OK){
    	// Header

    	if((res=f_write(&f_wav,"RIFF",  4, &written))!=FR_OK) goto error;
    	    i = LONG(36 + samplerate);
    	if((res=f_write(&f_wav,&i, 4,  &written))!=FR_OK) goto error;
    	if((res=f_write(&f_wav,"WAVE", 4, &written))!=FR_OK) goto error;

    	    // Subchunk 1

    	if((res=f_write(&f_wav, "fmt ", 4, &written))!=FR_OK) goto error;

	    if((res=f_write(&f_wav,&i, 4,  &written))!=FR_OK) goto error;           // Length
	    s = SHORT(1);
	    if((res=f_write(&f_wav,&s, 2,  &written))!=FR_OK) goto error;           // Format (PCM)
	    s = SHORT(2);
	    if((res=f_write(&f_wav,&s, 2,  &written))!=FR_OK) goto error;           // Channels (2=stereo)
	    i = LONG(samplerate);
	    if((res=f_write(&f_wav,&i, 4,  &written))!=FR_OK) goto error;           // Sample rate
	    i = LONG(samplerate * 2 * 2);
	    if((res=f_write(&f_wav,&i, 4,  &written))!=FR_OK) goto error;           // Byte rate (samplerate * stereo * 16 bit)
	    s = SHORT(2 * 2);
	    if((res=f_write(&f_wav,&s, 2,  &written))!=FR_OK) goto error;           // Block align (stereo * 16 bit)
	    s = SHORT(16);
	    if((res=f_write(&f_wav,&s, 2,  &written))!=FR_OK) goto error;           // Bits per sample (16 bit)

	    // Data subchunk

	    if((res=f_write(&f_wav,"data",  4, &written))!=FR_OK) goto error;
	     i = LONG(length);
	     if((res=f_write(&f_wav,&i, 4,  &written))!=FR_OK) goto error;          // Data length
	     if((res=f_write(&f_wav,data,  length, &written))!=FR_OK) goto error;    // Data

	    goto done;
	    error:
		printf("error writing '%s' error=%i\r\n", filename,res);
		done:
		f_close(&f_wav);
    }
}

//#endif




static boolean CacheSFX(sfxinfo_t *sfxinfo)
{
    int lumpnum;
    unsigned int lumplen;
    int samplerate;
    unsigned int length;
    byte *data;

    // need to load the sound

    lumpnum = sfxinfo->lumpnum;
    data = W_CacheLumpNum(lumpnum, PU_STATIC);
    lumplen = W_LumpLength(lumpnum);

    // Check the header, and ensure this is a valid sound

    if (lumplen < 8
     || data[0] != 0x03 || data[1] != 0x00)
    {
        // Invalid sound

        return false;
    }

    // 16 bit sample rate field, 32 bit length field

    samplerate = (data[3] << 8) | data[2];
    length = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) | data[4];

    // If the header specifies that the length of the sound is greater than
    // the length of the lump itself, this is an invalid sound lump

    // We also discard sound lumps that are less than 49 samples long,
    // as this is how DMX behaves - although the actual cut-off length
    // seems to vary slightly depending on the sample rate.  This needs
    // further investigation to better understand the correct
    // behavior.

    if (length > lumplen - 8 || length <= 48)
    {
        return false;
    }

    // The DMX sound library seems to skip the first 16 and last 16
    // bytes of the lump - reason unknown.

    data += 16;
    length -= 32;



    if (!CopySoundData_STM32(sfxinfo, data + 8, samplerate, length))return false; // streight copy
   // if (!ExpandSoundData_STM32(sfxinfo, data + 8, samplerate, length)) return false; // Sample rate conversion



#ifdef DEBUG_DUMP_WAVS
    {
        char filename[128];
        allocated_sound_t* snd = sfxinfo->driver_data;

        M_snprintf(filename, sizeof(filename), "0:/WAV/%s.wav",
                   DEH_String(sfxinfo->name));
        WriteWAV(filename, snd->chunk.abuf,
        		snd->chunk.alen, MIXER_FREQUENCY);
    }
#endif

    // don't need the original lump any more

    W_ReleaseLumpNum(lumpnum);

    return true;
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

#ifdef HAVE_LIBSAMPLERATE

// Preload all the sound effects - stops nasty ingame freezes

static void I_STM32_PrecacheSounds(sfxinfo_t *sounds, int num_sounds)
{
    char namebuf[9];
    int i;

    // Don't need to precache the sounds unless we are using libsamplerate.

    if (use_libsamplerate == 0)
    {
	return;
    }

    printf("I_SDL_PrecacheSounds: Precaching all sound effects..");

    for (i=0; i<num_sounds; ++i)
    {
        if ((i % 6) == 0)
        {
            printf(".");
            fflush(stdout);
        }

        GetSfxLumpName(&sounds[i], namebuf, sizeof(namebuf));

        sounds[i].lumpnum = W_CheckNumForName(namebuf);

        if (sounds[i].lumpnum != -1)
        {
            CacheSFX(&sounds[i]);
        }
    }

    printf("\n");
}

#else

static void I_STM32_PrecacheSounds(sfxinfo_t *sounds, int num_sounds)
{
    // no-op
}

#endif



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
	 allocated_sound_t *snd;

	if (!sound_initialized || chan < 0 || chan >= NUM_CHANNELS) return -1;

	// Release a sound effect if there is already one playing
	// on this channel


	ReleaseSoundOnChannel(chan);
#if 0
	ReleaseSoundOnChannel(chan);
	// Get the sound data

	if (!LockSound(sfxinfo)) return -1;
	// Get the sound data
	snd = sfxinfo->driver_data;

#endif
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
	   	leftend= buffer + (MIXBUFFERSIZE/2);// Step in mixbuffer, left and right, thus two.

		// Low-pass filter for cutoff frequency f:
		//
		// For sampling rate r, dt = 1 / r
		// rc = 1 / 2*pi*f
		// alpha = dt / (rc + dt)

		// Filter to the half sample rate of the original sound effect
		// (maximum frequency, by nyquist)

		float dt = 1.0f / MIXER_FREQUENCY;
		float rc = 1.0f / (3.14f * MIXER_FREQUENCY); //samplerate);
		float alpha = dt / (rc + dt);
		int active_channels=0;
	    // Mix sounds into the mixing buffer.
	    // Loop over step*SAMPLECOUNT,
	    //  that is 512 values for two channels.
	    while (leftout != leftend)
	    {
	    	active_channels=0;
		// Reset left/right value.
	    	dl = 0;
	    	dr = 0;

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
				*leftout = *rightout = 0;
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
		*leftout = dr;
		*rightout = dr;

		// Increment current pointers in mixbuffer.
		// stupid simple low pass filter
		// (FIX ME!  GET RID OF THE FLOATS)
	    *leftout = (int16_t) (alpha * *leftout + (1 - alpha) * *rightout);
			leftout += 2;
			rightout += 2;
	    }
	    ticks = HAL_GetTick()-ticks;
	if(active_channels >0) printf("Sound processing time %ul chan=%i\r\n", ticks,active_channels);
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
		_I_STM32_UpdateSound();
	}
}


void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
	if(buffer_state == BUFFER_STATE_PLAYING) {
		buffer_state=BUFFER_STATE_FULL_COMPLETE;
		_I_STM32_UpdateSound();
	}
}
static void I_STM32_ShutdownSound();


static void I_STM32_UpdateSound()
{
	if (!sound_initialized) return;

	if(buffer_state == BUFFER_STATE_INIT) { // first time

		buffer_state = BUFFER_STATE_HALF_COMPLETE;
		_I_STM32_UpdateSound();
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
    printf("mixbuffer_size=%i ", MIXBUFFERSIZE*sizeof(int16_t));
    /* Initialize the Audio codec and all related peripherals (I2S, I2C, IOExpander, IOs...) */
    if(BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_BOTH, audio_volume=60, MIXER_FREQUENCY) != AUDIO_OK)
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

