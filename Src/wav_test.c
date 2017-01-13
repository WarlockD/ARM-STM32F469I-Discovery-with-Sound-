#include "main.h"
#include "stm32f4xx_hal.h"
#include "dma2d.h"
#include "dsihost.h"
#include "fatfs.h"
#include "ltdc.h"

#include "sdio.h"
#include "usart.h"
#include "gpio.h"
#include "fmc.h"

/* USER CODE BEGIN Includes */
#include "stm32469i_discovery.h"
#include "stm32469i_discovery_sdram.h"
#include "stm32469i_discovery_lcd.h"
#include "stm32469i_discovery_sd.h"
#include "stm32469i_discovery_ts.h"
#include "stm32469i_discovery_audio.h"
#include <string.h>
#include <time.h>
#include <stdlib.h>


#define AUDIO_OUT_BUFFER_SIZE                      8192
#define AUDIO_IN_PCM_BUFFER_SIZE                   4*2304 /* buffer size in half-word */
#define AUDIO_IN_PDM_BUFFER_SIZE                   INTERNAL_BUFF_SIZE
/* Audio buffer control struct */
#define TOUCH_NEXT_XMIN         500
#define TOUCH_NEXT_XMAX         560
#define TOUCH_NEXT_YMIN         260
#define TOUCH_NEXT_YMAX         320

#define TOUCH_PREVIOUS_XMIN     380
#define TOUCH_PREVIOUS_XMAX     440
#define TOUCH_PREVIOUS_YMIN     260
#define TOUCH_PREVIOUS_YMAX     320

#define TOUCH_STOP_XMIN         260
#define TOUCH_STOP_XMAX         320
#define TOUCH_STOP_YMIN         260
#define TOUCH_STOP_YMAX         320

#define TOUCH_PAUSE_XMIN        140
#define TOUCH_PAUSE_XMAX        200
#define TOUCH_PAUSE_YMIN        260
#define TOUCH_PAUSE_YMAX        320

#define TOUCH_VOL_MINUS_XMIN    20
#define TOUCH_VOL_MINUS_XMAX    80
#define TOUCH_VOL_MINUS_YMIN    260
#define TOUCH_VOL_MINUS_YMAX    320

#define TOUCH_VOL_PLUS_XMIN     620
#define TOUCH_VOL_PLUS_XMAX     680
#define TOUCH_VOL_PLUS_YMIN     260
#define TOUCH_VOL_PLUS_YMAX     320

static Point NextPoints[] = {{TOUCH_NEXT_XMIN, TOUCH_NEXT_YMIN},
                             {TOUCH_NEXT_XMAX, (TOUCH_NEXT_YMIN+TOUCH_NEXT_YMAX)/2},
                             {TOUCH_NEXT_XMIN, TOUCH_NEXT_YMAX}};
static Point PreviousPoints[] = {{TOUCH_PREVIOUS_XMIN, (TOUCH_PREVIOUS_YMIN+TOUCH_PREVIOUS_YMAX)/2},
                                 {TOUCH_PREVIOUS_XMAX, TOUCH_PREVIOUS_YMIN},
                                 {TOUCH_PREVIOUS_XMAX, TOUCH_PREVIOUS_YMAX}};
#define DEBUG_FF
#ifdef DEBUG_FF
#define CHECK_FRESULT(R) fresult_debug((R), __FILE__,__LINE__,__FUNCTION__)
/* static functions */
#define DEBUG_MSG(FLAG) case FLAG: msg = #FLAG; break;
FRESULT fresult_debug(FRESULT result, const char* filename, int line, const char* func) {
	const char* msg = NULL;
	 switch (result)
	    {
	        case FR_OK: return FR_OK; // no error
	        DEBUG_MSG(FR_EXIST);
	        DEBUG_MSG(FR_NO_FILE);
	        DEBUG_MSG(FR_NO_PATH);
	        DEBUG_MSG(FR_INVALID_NAME);
	        DEBUG_MSG(FR_WRITE_PROTECTED);
	        DEBUG_MSG(FR_DENIED);
	        DEBUG_MSG(FR_TOO_MANY_OPEN_FILES);
	        DEBUG_MSG(FR_NOT_ENOUGH_CORE);
	        DEBUG_MSG(FR_DISK_ERR);
	        DEBUG_MSG(FR_INT_ERR);
	        DEBUG_MSG(FR_NOT_READY);
	        DEBUG_MSG(FR_INVALID_DRIVE);
	        DEBUG_MSG(FR_NOT_ENABLED);
	        DEBUG_MSG(FR_NO_FILESYSTEM);
	        DEBUG_MSG(FR_MKFS_ABORTED);
	        DEBUG_MSG(FR_TIMEOUT);
	        DEBUG_MSG(FR_LOCKED);
	        DEBUG_MSG(FR_INVALID_PARAMETER);
	        DEBUG_MSG(FR_INVALID_OBJECT);
	        default:
	        	msg= "!!!UNKNOWN!!!";
	        	break;
	    }
	 printf("FRESULT(%s:%i, %s)= %s\r\n", filename, line, func,msg);
}
#else
#define CHECK_FRESULT(R) (R)
#endif

#define LCD_LOG_TEXT_FONT Font12
#define LCD_LOG_HEADER_FONT Font16
typedef enum {
  AUDIO_STATE_IDLE = 0,
  AUDIO_STATE_WAIT,
  AUDIO_STATE_INIT,
  AUDIO_STATE_PLAY,
  AUDIO_STATE_PRERECORD,
  AUDIO_STATE_RECORD,
  AUDIO_STATE_NEXT,
  AUDIO_STATE_PREVIOUS,
  AUDIO_STATE_FORWARD,
  AUDIO_STATE_BACKWARD,
  AUDIO_STATE_STOP,
  AUDIO_STATE_PAUSE,
  AUDIO_STATE_RESUME,
  AUDIO_STATE_VOLUME_UP,
  AUDIO_STATE_VOLUME_DOWN,
  AUDIO_STATE_ERROR,
}AUDIO_PLAYBACK_StateTypeDef;

typedef enum {
  BUFFER_EMPTY = 0,
  BUFFER_FULL,
}WR_BUFFER_StateTypeDef;

typedef struct {
  uint32_t ChunkID;       /* 0 */
  uint32_t FileSize;      /* 4 */
  uint32_t FileFormat;    /* 8 */
  uint32_t SubChunk1ID;   /* 12 */
  uint32_t SubChunk1Size; /* 16*/
  uint16_t AudioFormat;   /* 20 */
  uint16_t NbrChannels;   /* 22 */
  uint32_t SampleRate;    /* 24 */

  uint32_t ByteRate;      /* 28 */
  uint16_t BlockAlign;    /* 32 */
  uint16_t BitPerSample;  /* 34 */
  uint32_t SubChunk2ID;   /* 36 */
  uint32_t SubChunk2Size; /* 40 */
}WAVE_FormatTypeDef;

typedef enum {
  BUFFER_OFFSET_NONE = 0,
  BUFFER_OFFSET_HALF,
  BUFFER_OFFSET_FULL,
}BUFFER_StateTypeDef;
typedef struct {

  BUFFER_StateTypeDef state;
  uint32_t fptr;
  uint8_t buff[AUDIO_OUT_BUFFER_SIZE];
}AUDIO_OUT_BufferTypeDef;

typedef enum {
  AUDIO_ERROR_NONE = 0,
  AUDIO_ERROR_IO,
  AUDIO_ERROR_EOF,
  AUDIO_ERROR_INVALID_VALUE,
}AUDIO_ErrorTypeDef;

#define FILEMGR_LIST_DEPDTH                        24
#define FILEMGR_FILE_NAME_SIZE                     40
#define FILEMGR_FULL_PATH_SIZE                     256
#define FILEMGR_MAX_LEVEL                          4
#define FILETYPE_DIR                               0
#define FILETYPE_FILE                              1

typedef struct _FILELIST_LineTypeDef {
  uint8_t type;
  uint8_t name[FILEMGR_FILE_NAME_SIZE];
}FILELIST_LineTypeDef;

typedef struct _FILELIST_FileTypeDef {
  FILELIST_LineTypeDef  file[FILEMGR_LIST_DEPDTH] ;
  uint16_t              ptr;
}FILELIST_FileTypeDef;
// audo test
static AUDIO_OUT_BufferTypeDef  BufferCtl = { BUFFER_OFFSET_NONE, 0 };
static volatile AUDIO_PLAYBACK_StateTypeDef AudioState = AUDIO_STATE_STOP;
static __IO uint32_t uwVolume = 70;
static FIL WavFile;
static WAVE_FormatTypeDef WaveFormat;
static AUDIO_ErrorTypeDef GetFileInfo(const char* filename, WAVE_FormatTypeDef *info);

AUDIO_ErrorTypeDef AUDIO_PLAYER_Init(void)
{
  if(BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_AUTO, uwVolume, I2S_AUDIOFREQ_48K) == 0)
  {
    return AUDIO_ERROR_NONE;
  }
  else
  {
    return AUDIO_ERROR_IO;
  }
}


/**
  * @brief  Initializes the Wave player.
  * @param  AudioFreq: Audio sampling frequency
  * @retval None
  */
static uint8_t PlayerInit(uint32_t AudioFreq)
{
  /* Initialize the Audio codec and all related peripherals (I2S, I2C, IOExpander, IOs...) */
  if(BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_BOTH, uwVolume, AudioFreq) != 0)
  {
    return 1;
  }
  else
  {
    BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);
    return 0;
  }
}
AUDIO_ErrorTypeDef AUDIO_PLAYER_Stop(void)
{
	 AudioState = AUDIO_STATE_STOP;
  BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
  f_close(&WavFile);
  return AUDIO_ERROR_NONE;
}

AUDIO_ErrorTypeDef AUDIO_PLAYER_Start(const char* filename)
{
  uint32_t bytesread;

  f_close(&WavFile);
    GetFileInfo(filename, &WaveFormat);

    /*Adjust the Audio frequency */
    PlayerInit(WaveFormat.SampleRate);

    BufferCtl.state = BUFFER_OFFSET_NONE;

    /* Get Data from USB Flash Disk */
    f_lseek(&WavFile, 0);

    /* Fill whole buffer at first time */
    if(CHECK_FRESULT(f_read(&WavFile,
              &BufferCtl.buff[0],
              AUDIO_OUT_BUFFER_SIZE,
              (void *)&bytesread)) == FR_OK)
    {
          if(bytesread == 0 || BSP_AUDIO_OUT_Play((uint16_t*)&BufferCtl.buff[0], AUDIO_OUT_BUFFER_SIZE)!= HAL_OK) {
        	  printf("Failed staring BSP_AUDIO_OUT_Play\r\n");
        	  return AUDIO_ERROR_IO;
          } else {
        	  AudioState = AUDIO_STATE_PLAY;
        	 printf("Started BSP_AUDIO_OUT_Play with %ul read\r\n",bytesread);
        	           return AUDIO_ERROR_NONE;
          }
    }
  return AUDIO_ERROR_IO;
}

void test_it() {
	memset(&BufferCtl,0, sizeof(BufferCtl));
	memset(&WavFile,0, sizeof(FIL));
	UINT bytesread=0;
	uint32_t prev_elapsed_time=0;
	char str[128];
	AUDIO_PLAYER_Init();
	AUDIO_PLAYER_Start("TEST.WAV");
	uint32_t last_tick = HAL_GetTick()+500;
//	__HAL_DBGMCU_UNFREEZE_IWDG();
	  while (1)
	  {
		  uint32_t current = HAL_GetTick();
		  if(current > last_tick) {
			  last_tick = current + 500;
			  BSP_LED_Toggle(LED_GREEN);
		  }
		  if(AudioState == AUDIO_STATE_STOP){
			  BSP_LED_On(LED_RED);
			  continue;
		  }
		  if(AudioState == AUDIO_STATE_PLAY){


		  if(BufferCtl.fptr >= WaveFormat.FileSize)
		     {
		       BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
		       printf("Done! \r\n",bytesread);
		     }

		     if(BufferCtl.state == BUFFER_OFFSET_HALF)
		     {
		       if(CHECK_FRESULT(f_read(&WavFile,
		                 &BufferCtl.buff[0],
		                 AUDIO_OUT_BUFFER_SIZE/2,
		                 (void *)&bytesread)) != FR_OK)
		       {
		    	   AUDIO_PLAYER_Stop();
		         BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
		         continue;
		       }
		       BufferCtl.state = BUFFER_OFFSET_NONE;
		       BufferCtl.fptr += bytesread;
		       printf("BUFFER_OFFSET_HALF with %ul read\r\n",bytesread);
		     }

		     if(BufferCtl.state == BUFFER_OFFSET_FULL)
		     {
		       if(CHECK_FRESULT(f_read(&WavFile,
		                 &BufferCtl.buff[AUDIO_OUT_BUFFER_SIZE /2],
		                 AUDIO_OUT_BUFFER_SIZE/2,
		                 (void *)&bytesread)) != FR_OK)
		       {
		    	   AUDIO_PLAYER_Stop();
		         continue;
		       }

		       BufferCtl.state = BUFFER_OFFSET_NONE;
		       BufferCtl.fptr += bytesread;
		       printf("BUFFER_OFFSET_FULL with %ul read\r\n",bytesread);
		     }

		     /* Display elapsed time */
		     uint32_t elapsed_time = BufferCtl.fptr / WaveFormat.ByteRate;
		     if(prev_elapsed_time != elapsed_time)
		     {
		       prev_elapsed_time = elapsed_time;
		       sprintf((char *)str, "[%02d:%02d]", (int)(elapsed_time /60), (int)(elapsed_time%60));
		       BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
		       BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
		       BSP_LCD_DisplayStringAt(263, LINE(8), str, LEFT_MODE);
		     }
		  }
	  }
}

void BSP_AUDIO_OUT_TransferComplete_CallBack1(void)
{
  if(AudioState == AUDIO_STATE_PLAY)
  {
    BufferCtl.state = BUFFER_OFFSET_FULL;
  }
}


void BSP_AUDIO_OUT_HalfTransfer_CallBack1(void)
{
  if(AudioState == AUDIO_STATE_PLAY)
  {
    BufferCtl.state = BUFFER_OFFSET_HALF;
  }
}

void BSP_AUDIO_OUT_Error_CallBack1()
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



/**
  * @brief  Gets the file info.
  * @param  file_idx: File index
  * @param  info: Pointer to WAV file info
  * @retval Audio error
  */
static AUDIO_ErrorTypeDef GetFileInfo(const char* filename, WAVE_FormatTypeDef *info)
{
  uint32_t bytesread;
  uint32_t duration;
  if(CHECK_FRESULT(f_open(&WavFile, (const TCHAR*)filename, FA_OPEN_EXISTING | FA_READ)) == FR_OK)
  {
    /* Fill the buffer to Send */
    if(CHECK_FRESULT(f_read(&WavFile, info, sizeof(WaveFormat), (void *)&bytesread)) == FR_OK)
    {
      printf( "Playing file %s\r\n",filename);
      printf("Sample rate : %d Hz\r\n", (int)(info->SampleRate));
      printf("Channels number : %d\r\n", info->NbrChannels);
      duration = info->FileSize / info->ByteRate;
      printf("File Size : %d KB [%02d:%02d]\r\n", (int)(info->FileSize/1024), (int)(duration/60), (int)(duration%60));
      printf("Volume : %lu\r\n", uwVolume);
      return AUDIO_ERROR_NONE;
    }
    CHECK_FRESULT(f_close(&WavFile));
  }
  return AUDIO_ERROR_IO;
}


