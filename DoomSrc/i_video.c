// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";
#include "main.h"
#include "chocdoom/config.h"
#include "chocdoom/v_video.h"
#include "chocdoom/m_argv.h"
#include "chocdoom/d_event.h"
#include "chocdoom/d_main.h"
#include "chocdoom/i_video.h"
#include "chocdoom/z_zone.h"

#include "chocdoom/tables.h"
#include "chocdoom/doomkeys.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "stm32469i_discovery.h"
#include "stm32469i_discovery_sdram.h"
#include "stm32469i_discovery_lcd.h"
#include "stm32469i_discovery_sd.h"


#define  HID_KEY_NONE                               0x00
#define  HID_KEY_ERRORROLLOVER                      0x01
#define  HID_KEY_POSTFAIL                           0x02
#define  HID_KEY_ERRORUNDEFINED                     0x03
#define  HID_KEY_A                                  0x04
#define  HID_KEY_B                                  0x05
#define  HID_KEY_C                                  0x06
#define  HID_KEY_D                                  0x07
#define  HID_KEY_E                                  0x08
#define  HID_KEY_F                                  0x09
#define  HID_KEY_G                                  0x0A
#define  HID_KEY_H                                  0x0B
#define  HID_KEY_I                                  0x0C
#define  HID_KEY_J                                  0x0D
#define  HID_KEY_K                                  0x0E
#define  HID_KEY_L                                  0x0F
#define  HID_KEY_M                                  0x10
#define  HID_KEY_N                                  0x11
#define  HID_KEY_O                                  0x12
#define  HID_KEY_P                                  0x13
#define  HID_KEY_Q                                  0x14
#define  HID_KEY_R                                  0x15
#define  HID_KEY_S                                  0x16
#define  HID_KEY_T                                  0x17
#define  HID_KEY_U                                  0x18
#define  HID_KEY_V                                  0x19
#define  HID_KEY_W                                  0x1A
#define  HID_KEY_X                                  0x1B
#define  HID_KEY_Y                                  0x1C
#define  HID_KEY_Z                                  0x1D
#define  HID_KEY_1_EXCLAMATION_MARK                 0x1E
#define  HID_KEY_2_AT                               0x1F
#define  HID_KEY_3_NUMBER_SIGN                      0x20
#define  HID_KEY_4_DOLLAR                           0x21
#define  HID_KEY_5_PERCENT                          0x22
#define  HID_KEY_6_CARET                            0x23
#define  HID_KEY_7_AMPERSAND                        0x24
#define  HID_KEY_8_ASTERISK                         0x25
#define  HID_KEY_9_OPARENTHESIS                     0x26
#define  HID_KEY_0_CPARENTHESIS                     0x27
#define  HID_KEY_ENTER                              0x28
#define  HID_KEY_ESCAPE                             0x29
#define  HID_KEY_BACKSPACE                          0x2A
#define  HID_KEY_TAB                                0x2B
#define  HID_KEY_SPACEBAR                           0x2C
#define  HID_KEY_MINUS_UNDERSCORE                   0x2D
#define  HID_KEY_EQUAL_PLUS                         0x2E
#define  HID_KEY_OBRACKET_AND_OBRACE                0x2F
#define  HID_KEY_CBRACKET_AND_CBRACE                0x30
#define  HID_KEY_BACKSLASH_VERTICAL_BAR             0x31
#define  HID_KEY_NONUS_NUMBER_SIGN_TILDE            0x32
#define  HID_KEY_SEMICOLON_COLON                    0x33
#define  HID_KEY_SINGLE_AND_DOUBLE_QUOTE            0x34
#define  HID_KEY_GRAVE ACCENT AND TILDE             0x35
#define  HID_KEY_COMMA_AND_LESS                     0x36
#define  HID_KEY_DOT_GREATER                        0x37
#define  HID_KEY_SLASH_QUESTION                     0x38
#define  HID_KEY_CAPS LOCK                          0x39
#define  HID_KEY_F1                                 0x3A
#define  HID_KEY_F2                                 0x3B
#define  HID_KEY_F3                                 0x3C
#define  HID_KEY_F4                                 0x3D
#define  HID_KEY_F5                                 0x3E
#define  HID_KEY_F6                                 0x3F
#define  HID_KEY_F7                                 0x40
#define  HID_KEY_F8                                 0x41
#define  HID_KEY_F9                                 0x42
#define  HID_KEY_F10                                0x43
#define  HID_KEY_F11                                0x44
#define  HID_KEY_F12                                0x45
#define  HID_KEY_PRINTSCREEN                        0x46
#define  HID_KEY_SCROLL LOCK                        0x47
#define  HID_KEY_PAUSE                              0x48
#define  HID_KEY_INSERT                             0x49
#define  HID_KEY_HOME                               0x4A
#define  HID_KEY_PAGEUP                             0x4B
#define  HID_KEY_DELETE                             0x4C
#define  HID_KEY_END1                               0x4D
#define  HID_KEY_PAGEDOWN                           0x4E
#define  HID_KEY_RIGHTARROW                         0x4F
#define  HID_KEY_LEFTARROW                          0x50
#define  HID_KEY_DOWNARROW                          0x51
#define  HID_KEY_UPARROW                            0x52
#define  HID_KEY_KEYPAD_NUM_LOCK_AND_CLEAR          0x53
#define  HID_KEY_KEYPAD_SLASH                       0x54
#define  HID_KEY_KEYPAD_ASTERIKS                    0x55
#define  HID_KEY_KEYPAD_MINUS                       0x56
#define  HID_KEY_KEYPAD_PLUS                        0x57
#define  HID_KEY_KEYPAD_ENTER                       0x58
#define  HID_KEY_KEYPAD_1_END                       0x59
#define  HID_KEY_KEYPAD_2_DOWN_ARROW                0x5A
#define  HID_KEY_KEYPAD_3_PAGEDN                    0x5B
#define  HID_KEY_KEYPAD_4_LEFT_ARROW                0x5C
#define  HID_KEY_KEYPAD_5                           0x5D
#define  HID_KEY_KEYPAD_6_RIGHT_ARROW               0x5E
#define  HID_KEY_KEYPAD_7_HOME                      0x5F
#define  HID_KEY_KEYPAD_8_UP_ARROW                  0x60
#define  HID_KEY_KEYPAD_9_PAGEUP                    0x61
#define  HID_KEY_KEYPAD_0_INSERT                    0x62
#define  HID_KEY_KEYPAD_DECIMAL_SEPARATOR_DELETE    0x63
#define  HID_KEY_NONUS_BACK_SLASH_VERTICAL_BAR      0x64
#define  HID_KEY_APPLICATION                        0x65
#define  HID_KEY_POWER                              0x66
#define  HID_KEY_KEYPAD_EQUAL                       0x67
#define  HID_KEY_F13                                0x68
#define  HID_KEY_F14                                0x69
#define  HID_KEY_F15                                0x6A
#define  HID_KEY_F16                                0x6B
#define  HID_KEY_F17                                0x6C
#define  HID_KEY_F18                                0x6D
#define  HID_KEY_F19                                0x6E
#define  HID_KEY_F20                                0x6F
#define  HID_KEY_F21                                0x70
#define  HID_KEY_F22                                0x71
#define  HID_KEY_F23                                0x72
#define  HID_KEY_F24                                0x73
#define  HID_KEY_EXECUTE                            0x74
#define  HID_KEY_HELP                               0x75
#define  HID_KEY_MENU                               0x76
#define  HID_KEY_SELECT                             0x77
#define  HID_KEY_STOP                               0x78
#define  HID_KEY_AGAIN                              0x79
#define  HID_KEY_UNDO                               0x7A
#define  HID_KEY_CUT                                0x7B
#define  HID_KEY_COPY                               0x7C
#define  HID_KEY_PASTE                              0x7D
#define  HID_KEY_FIND                               0x7E
#define  HID_KEY_MUTE                               0x7F
#define  HID_KEY_VOLUME_UP                          0x80
#define  HID_KEY_VOLUME_DOWN                        0x81
#define  HID_KEY_LOCKING_CAPS_LOCK                  0x82
#define  HID_KEY_LOCKING_NUM_LOCK                   0x83
#define  HID_KEY_LOCKING_SCROLL_LOCK                0x84
#define  HID_KEY_KEYPAD_COMMA                       0x85
#define  HID_KEY_KEYPAD_EQUAL_SIGN                  0x86
#define  HID_KEY_INTERNATIONAL1                     0x87
#define  HID_KEY_INTERNATIONAL2                     0x88
#define  HID_KEY_INTERNATIONAL3                     0x89
#define  HID_KEY_INTERNATIONAL4                     0x8A
#define  HID_KEY_INTERNATIONAL5                     0x8B
#define  HID_KEY_INTERNATIONAL6                     0x8C
#define  HID_KEY_INTERNATIONAL7                     0x8D
#define  HID_KEY_INTERNATIONAL8                     0x8E
#define  HID_KEY_INTERNATIONAL9                     0x8F
#define  HID_KEY_LANG1                              0x90
#define  HID_KEY_LANG2                              0x91
#define  HID_KEY_LANG3                              0x92
#define  HID_KEY_LANG4                              0x93
#define  HID_KEY_LANG5                              0x94
#define  HID_KEY_LANG6                              0x95
#define  HID_KEY_LANG7                              0x96
#define  HID_KEY_LANG8                              0x97
#define  HID_KEY_LANG9                              0x98
#define  HID_KEY_ALTERNATE_ERASE                    0x99
#define  HID_KEY_SYSREQ                             0x9A
#define  HID_KEY_CANCEL                             0x9B
#define  HID_KEY_CLEAR                              0x9C
#define  HID_KEY_PRIOR                              0x9D
#define  HID_KEY_RETURN                             0x9E
#define  HID_KEY_SEPARATOR                          0x9F
#define  HID_KEY_OUT                                0xA0
#define  HID_KEY_OPER                               0xA1
#define  HID_KEY_CLEAR_AGAIN                        0xA2
#define  HID_KEY_CRSEL                              0xA3
#define  HID_KEY_EXSEL                              0xA4
#define  HID_KEY_KEYPAD_00                          0xB0
#define  HID_KEY_KEYPAD_000                         0xB1
#define  HID_KEY_THOUSANDS_SEPARATOR                0xB2
#define  HID_KEY_DECIMAL_SEPARATOR                  0xB3
#define  HID_KEY_CURRENCY_UNIT                      0xB4
#define  HID_KEY_CURRENCY_SUB_UNIT                  0xB5
#define  HID_KEY_KEYPAD_OPARENTHESIS                0xB6
#define  HID_KEY_KEYPAD_CPARENTHESIS                0xB7
#define  HID_KEY_KEYPAD_OBRACE                      0xB8
#define  HID_KEY_KEYPAD_CBRACE                      0xB9
#define  HID_KEY_KEYPAD_TAB                         0xBA
#define  HID_KEY_KEYPAD_BACKSPACE                   0xBB
#define  HID_KEY_KEYPAD_A                           0xBC
#define  HID_KEY_KEYPAD_B                           0xBD
#define  HID_KEY_KEYPAD_C                           0xBE
#define  HID_KEY_KEYPAD_D                           0xBF
#define  HID_KEY_KEYPAD_E                           0xC0
#define  HID_KEY_KEYPAD_F                           0xC1
#define  HID_KEY_KEYPAD_XOR                         0xC2
#define  HID_KEY_KEYPAD_CARET                       0xC3
#define  HID_KEY_KEYPAD_PERCENT                     0xC4
#define  HID_KEY_KEYPAD_LESS                        0xC5
#define  HID_KEY_KEYPAD_GREATER                     0xC6
#define  HID_KEY_KEYPAD_AMPERSAND                   0xC7
#define  HID_KEY_KEYPAD_LOGICAL_AND                 0xC8
#define  HID_KEY_KEYPAD_VERTICAL_BAR                0xC9
#define  HID_KEY_KEYPAD_LOGIACL_OR                  0xCA
#define  HID_KEY_KEYPAD_COLON                       0xCB
#define  HID_KEY_KEYPAD_NUMBER_SIGN                 0xCC
#define  HID_KEY_KEYPAD_SPACE                       0xCD
#define  HID_KEY_KEYPAD_AT                          0xCE
#define  HID_KEY_KEYPAD_EXCLAMATION_MARK            0xCF
#define  HID_KEY_KEYPAD_MEMORY_STORE                0xD0
#define  HID_KEY_KEYPAD_MEMORY_RECALL               0xD1
#define  HID_KEY_KEYPAD_MEMORY_CLEAR                0xD2
#define  HID_KEY_KEYPAD_MEMORY_ADD                  0xD3
#define  HID_KEY_KEYPAD_MEMORY_SUBTRACT             0xD4
#define  HID_KEY_KEYPAD_MEMORY_MULTIPLY             0xD5
#define  HID_KEY_KEYPAD_MEMORY_DIVIDE               0xD6
#define  HID_KEY_KEYPAD_PLUSMINUS                   0xD7
#define  HID_KEY_KEYPAD_CLEAR                       0xD8
#define  HID_KEY_KEYPAD_CLEAR_ENTRY                 0xD9
#define  HID_KEY_KEYPAD_BINARY                      0xDA
#define  HID_KEY_KEYPAD_OCTAL                       0xDB
#define  HID_KEY_KEYPAD_DECIMAL                     0xDC
#define  HID_KEY_KEYPAD_HEXADECIMAL                 0xDD
#define  HID_KEY_LEFTCONTROL                        0xE0
#define  HID_KEY_LEFTSHIFT                          0xE1
#define  HID_KEY_LEFTALT                            0xE2
#define  HID_KEY_LEFT_GUI                           0xE3
#define  HID_KEY_RIGHTCONTROL                       0xE4
#define  HID_KEY_RIGHTSHIFT                         0xE5
#define  HID_KEY_RIGHTALT                           0xE6
#define  HID_KEY_RIGHT_GUI                          0xE7

//#include "lcd.h"
#include <assert.h>

#include "touch.h"
#include "button.h"

#define SCALE_2X
#define GFX_RGB565(r, g, b)			((((r & 0xF8) >> 3) << 11) | (((g & 0xFC) >> 2) << 5) | ((b & 0xF8) >> 3))

#define GFX_RGB565_R(color)			((0xF800 & color) >> 11)
#define GFX_RGB565_G(color)			((0x07E0 & color) >> 5)
#define GFX_RGB565_B(color)			(0x001F & color)

#define GFX_ARGB8888(r, g, b, a)	(((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF))

#define GFX_ARGB8888_R(color)		((color & 0x00FF0000) >> 16)
#define GFX_ARGB8888_G(color)		((color & 0x0000FF00) >> 8)
#define GFX_ARGB8888_B(color)		((color & 0x000000FF))
#define GFX_ARGB8888_A(color)		((color & 0xFF000000) >> 24)

#define GFX_TRANSPARENT				0x00
#define GFX_OPAQUE					0xFF
// The screen buffer; this is modified to draw things to the screen

byte *I_VideoBuffer = NULL;
#ifdef SCALE_2X
byte* I_VideoBufferX2 = NULL;
#endif

// If true, game is running as a screensaver

boolean screensaver_mode = false;

// Flag indicating whether the screen is currently visible:
// when the screen isnt visible, don't render the screen

boolean screenvisible;

// Mouse acceleration
//
// This emulates some of the behavior of DOS mouse drivers by increasing
// the speed when the mouse is moved fast.
//
// The mouse input values are input directly to the game, but when
// the values exceed the value of mouse_threshold, they are multiplied
// by mouse_acceleration to increase the speed.

float mouse_acceleration = 2.0;
int mouse_threshold = 10;

// Gamma correction level to use

int usegamma = 0;

int usemouse = 0;

// If true, keyboard mapping is ignored, like in Vanilla Doom.
// The sensible thing to do is to disable this if you have a non-US
// keyboard.

int vanilla_keyboard_mapping = true;


typedef struct
{
	byte r;
	byte g;
	byte b;
} col_t;

// Palette converted to RGB565

//static uint16_t rgb565_palette[256];
static uint32_t rgb888_palette[256];

// Last touch state

static touch_state_t last_touch_state;

// Last button state

static bool last_button_state;

// run state

static bool run;
//extern uint32_t* layer0_address;
extern DMA2D_HandleTypeDef hdma2d_eval;
extern LTDC_HandleTypeDef  hltdc_eval;
//extern DSI_HandleTypeDef hdsi_eval;

#define DMA2D_WORKING               ((DMA2D->CR & DMA2D_CR_START))
#define DMA2D_WAIT                  do { while (DMA2D_WORKING); DMA2D->IFCR = DMA2D_IFSR_CTCIF;} while (0);
#define DMA2D_CLUT_WORKING          ((DMA2D->FGPFCCR & DMA2D_FGPFCCR_START))
#define DMA2D_CLUT_WAIT              do { while (DMA2D_CLUT_WORKING); DMA2D->IFCR = DMA2D_IFSR_CCTCIF;} while (0);

volatile bool pallete_load_complete = true;
volatile bool frame_finished = true;
void HAL_DMA2D_CLUTLoadingCpltCallback(DMA2D_HandleTypeDef *hdma2d)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdma2d);
  pallete_load_complete =true;

}
void HAL_FrameFinished(DMA2D_HandleTypeDef *hdma2d)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdma2d);
  frame_finished =true;

}
void HAL_FrameError(DMA2D_HandleTypeDef *hdma2d)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdma2d);
  while(1) {
	  BSP_LED_Toggle(LED_GREEN);
	  HAL_Delay(1);
  }

}
void SetupDMA() {
	DMA2D_WAIT;
	hdma2d_eval.Instance = DMA2D;
	//  hdma2d.Init.Mode = DMA2D_M2M;
	hdma2d_eval.Init.Mode = DMA2D_M2M_PFC;
	hdma2d_eval.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
#ifdef SCALE_2X
	hdma2d_eval.Init.OutputOffset = 800-(SCREENWIDTH*2);
#else
	hdma2d_eval.Init.OutputOffset = 800-SCREENWIDTH;
#endif
	hdma2d_eval.XferCpltCallback = HAL_FrameFinished;
	hdma2d_eval.XferErrorCallback = 	HAL_FrameError;
	hdma2d_eval.LayerCfg[1].InputOffset = 0;
	 // hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;

	hdma2d_eval.LayerCfg[1].InputColorMode =DMA2D_INPUT_L8;
	hdma2d_eval.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
	hdma2d_eval.LayerCfg[1].InputAlpha = 0;
	if (HAL_DMA2D_Init(&hdma2d_eval) != HAL_OK)  Error_Handler();
	if (HAL_DMA2D_ConfigLayer(&hdma2d_eval, 1) != HAL_OK)  Error_Handler();

}

void I_InitGraphics (void)
{
//	gfx_image_t keys_img;
//	gfx_coord_t coords;

//	gfx_init_img (&keys_img, 40, 320, GFX_PIXEL_FORMAT_RGB565, RGB565_BLACK);
	//keys_img.pixel_data = (uint8_t*)img_keys;
//	gfx_init_img_coord (&coords, &keys_img);

//	gfx_draw_img (&keys_img, &coords);

//
//	gfx_draw_img (&keys_img, &coords);
//	lcd_refresh ();

	I_VideoBuffer = (byte*)Z_Malloc (SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);
#ifdef SCALE_2X
	I_VideoBufferX2 = (byte*)Z_Malloc (SCREENWIDTH * SCREENHEIGHT*4, PU_STATIC, NULL);
#endif
	screenvisible = true;
	SetupDMA();

}

void I_ShutdownGraphics (void)
{
	Z_Free (I_VideoBuffer);
#ifdef SCALE_2X
	Z_Free (I_VideoBufferX2);
#endif
}

void I_StartFrame (void)
{

}


// we need to sort the keys in order so we can see if we need to send a key up message
// this is by far the fastest 0,10
static __inline__  void key_sorting(int16_t* d) {
#define SWAP(x,y) if (d[y] > d[x]) { int16_t tmp = d[x]; d[x] = d[y]; d[y] = tmp; }
	SWAP(4, 9);
	SWAP(3, 8);
	SWAP(2, 7);
	SWAP(1, 6);
	SWAP(0, 5);
	SWAP(1, 4);
	SWAP(6, 9);
	SWAP(0, 3);
	SWAP(5, 8);
	SWAP(0, 2);
	SWAP(3, 6);
	SWAP(7, 9);
	SWAP(0, 1);
	SWAP(2, 4);
	SWAP(5, 7);
	SWAP(8, 9);
	SWAP(1, 2);
	SWAP(4, 6);
	SWAP(7, 8);
	SWAP(3, 5);
	SWAP(2, 5);
	SWAP(6, 8);
	SWAP(1, 3);
	SWAP(4, 7);
	SWAP(2, 3);
	SWAP(6, 7);
	SWAP(3, 4);
	SWAP(5, 6);
	SWAP(4, 5);
#undef SWAP
}
// turn this into an array so we can configure it easier

int USBHidKeyToDoomKey(int usb_key) {
	switch(usb_key) {
	// wasd
	case HID_KEY_A: return KEY_STRAFE_L;
	case HID_KEY_D: return KEY_STRAFE_R;
	case HID_KEY_W: return KEY_UPARROW;
	case HID_KEY_S: return KEY_DOWNARROW;
	case HID_KEY_Q: return KEY_RIGHTARROW;
	case HID_KEY_E: return KEY_LEFTARROW;

	case HID_KEY_RIGHTARROW: return KEY_RIGHTARROW;
	case HID_KEY_LEFTARROW: return KEY_LEFTARROW;
	case HID_KEY_UPARROW: return KEY_UPARROW;
	case HID_KEY_DOWNARROW: return KEY_DOWNARROW;

	case HID_KEY_LEFTCONTROL: return KEY_USE;
	case HID_KEY_SPACEBAR: return KEY_FIRE;
	case HID_KEY_ESCAPE: return KEY_ESCAPE;
	case HID_KEY_ENTER: return KEY_ENTER;
	case HID_KEY_TAB: return KEY_TAB;
	case HID_KEY_F1: return KEY_F1;
	case HID_KEY_F2: return KEY_F2;
	case HID_KEY_F3: return KEY_F3;
	case HID_KEY_F4: return KEY_F4;
	case HID_KEY_F5: return KEY_F5;
	case HID_KEY_F6: return KEY_F6;
	case HID_KEY_F7: return KEY_F7;
	case HID_KEY_F8: return KEY_F8;
	case HID_KEY_F9: return KEY_F9;
	case HID_KEY_F10: return KEY_F10;
	case HID_KEY_F11: return KEY_F11;
	case HID_KEY_F12: return KEY_F12;
	case HID_KEY_BACKSPACE: return KEY_BACKSPACE;
	case HID_KEY_PAUSE: return KEY_PAUSE;
	case HID_KEY_EQUAL_PLUS: return KEY_EQUALS;
	case HID_KEY_MINUS_UNDERSCORE: return KEY_MINUS;
	default:
		printf("unknown key");
		return 0;
		break;
	}

}



void process_keys(int16_t* in, int16_t* out ){
		event_t event;
		while(*in && *out){
			if(*in !=0 && (*out == 0 || *in < *out)) {
				event.type =  ev_keyup;
				event.data1 = *in;
				event.data2 = -1;
				event.data3 = -1;
				D_PostEvent (&event);
				printf("Key Up %i\r\n",*in);
				in++;
			} else if(*out != 0 && (*in == 0 || *out < *in)){
				event.type =  ev_keydown;
				event.data1 = *out;
				event.data2 = -1;
				event.data3 = -1;
				D_PostEvent (&event);
				printf("Key Down %i\r\n",*out
						);
				out++;
			}
			else {
				if(*in) in++;
				if(*out) out++;
			} // equal
		}
}


void I_GetEvent (void)
{
	event_t event;
	bool button_state;

	button_state = button_read ();

	if (last_button_state != button_state)
	{
		last_button_state = button_state;

		event.type = last_button_state ? ev_keydown : ev_keyup;
		event.data1 = KEY_FIRE;
		event.data2 = -1;
		event.data3 = -1;
		printf("button press\r\n");
		D_PostEvent (&event);
	}

	touch_main ();

	if ((touch_state.x != last_touch_state.x) || (touch_state.y != last_touch_state.y) || (touch_state.status != last_touch_state.status))
	{
		last_touch_state = touch_state;
		printf("Touch State: %i, %i,%i\r\n", last_touch_state.status,last_touch_state.x,last_touch_state.y);
		event.type = (touch_state.status == TOUCH_PRESSED) ? ev_keydown : ev_keyup;
		event.data1 = -1;
		event.data2 = -1;
		event.data3 = -1;

		if ((touch_state.x > 49)
		 && (touch_state.x < 72)
		 && (touch_state.y > 104)
		 && (touch_state.y < 143))
		{
			// select weapon
			if (touch_state.x < 60)
			{
				// lower row (5-7)
				if (touch_state.y < 119)
				{
					event.data1 = '5';
				}
				else if (touch_state.y < 131)
				{
					event.data1 = '6';
				}
				else
				{
					event.data1 = '1';
				}
			}
			else
			{
				// upper row (2-4)
				if (touch_state.y < 119)
				{
					event.data1 = '2';
				}
				else if (touch_state.y < 131)
				{
					event.data1 = '3';
				}
				else
				{
					event.data1 = '4';
				}
			}
		}
		else if (touch_state.x < 40)
		{
			// button bar at bottom screen
			if (touch_state.y < 40)
			{
				// enter
				event.data1 = KEY_ENTER;
			}
			else if (touch_state.y < 80)
			{
				// escape
				event.data1 = KEY_ESCAPE;
			}
			else if (touch_state.y < 120)
			{
				// use
				event.data1 = KEY_USE;
			}
			else if (touch_state.y < 160)
			{
				// map
				event.data1 = KEY_TAB;
			}
			else if (touch_state.y < 200)
			{
				// pause
				event.data1 = KEY_PAUSE;
			}
			else if (touch_state.y < 240)
			{
				// toggle run
				if (touch_state.status == TOUCH_PRESSED)
				{
					run = !run;

					event.data1 = KEY_RSHIFT;

					if (run)
					{
						event.type = ev_keydown;
					}
					else
					{
						event.type = ev_keyup;
					}
				}
				else
				{
					return;
				}
			}
			else if (touch_state.y < 280)
			{
				// save
				event.data1 = KEY_F2;
			}
			else if (touch_state.y < 320)
			{
				// load
				event.data1 = KEY_F3;
			}
		}
		else
		{
			// movement/menu navigation
			if (touch_state.x < 100)
			{
				if (touch_state.y < 100)
				{
					event.data1 = KEY_STRAFE_L;
				}
				else if (touch_state.y < 220)
				{
					event.data1 = KEY_DOWNARROW;
				}
				else
				{
					event.data1 = KEY_STRAFE_R;
				}
			}
			else if (touch_state.x < 180)
			{
				if (touch_state.y < 160)
				{
					event.data1 = KEY_LEFTARROW;
				}
				else
				{
					event.data1 = KEY_RIGHTARROW;
				}
			}
			else
			{
				event.data1 = KEY_UPARROW;
			}
		}

		D_PostEvent (&event);
	}
}

void I_StartTic (void)
{
	I_GetEvent();
}

void I_UpdateNoBlit (void)
{
	 other_things();
}

void TM_INT_DMA2DGRAPHIC_InitAndTransfer(void) {
	/* Wait until transfer is done first from other calls */
	DMA2D_WAIT;

	/* DeInit DMA2D */
	RCC->AHB1RSTR |= RCC_AHB1RSTR_DMA2DRST;
	RCC->AHB1RSTR &= ~RCC_AHB1RSTR_DMA2DRST;

	/* Initialize DMA2D */
	//DMA2D_Init(&GRAPHIC_DMA2D_InitStruct);

	/* Start transfer */
	DMA2D->CR |= (uint32_t)DMA2D_CR_START;

	/* Wait till transfer ends */
	DMA2D_WAIT;
}
void UpdateNoScale() {
	assert(HAL_DMA2D_PollForTransfer(&hdma2d_eval,50) == HAL_OK);
	// MODIFY_REG(hdma2d->Instance->OOR, DMA2D_OOR_LO, 800-SCREENWIDTH);
	HAL_DMA2D_Start(&hdma2d_eval, (uint32_t)I_VideoBuffer, (hltdc_eval.LayerCfg[0].FBStartAdress), SCREENWIDTH,  SCREENHEIGHT);
}
void DoubleScanLIne(uint8_t* src, uint8_t* dst, uint32_t original_size){
	for (int x = 0; x < original_size; x++){
		*dst++ = *src; *dst++ = *src++;
	}
}
void DoubleScreen(uint8_t* src, uint8_t* dst, uint32_t width, uint32_t height){
	uint32_t nwidth = width*2;
	uint32_t nheight = height*2;
	for (uint32_t y = 0; y < height; y++){
		uint8_t* dst_start = dst;
		for (int x = 0; x < width; x++){
			dst[0] = dst[1] = *src;
			//dst[nwidth] = dst[nwidth+1] = *src;
			src++;
			dst+=2;
			//*dst++ = *src; *dst++ = *src++;
		}
		memcpy(dst, dst_start, nwidth);
		dst+= nwidth;
		//memcpy(dst, start_src, nwidth);
	}
}
void Update2XScale() {
	assert(HAL_DMA2D_PollForTransfer(&hdma2d_eval,50) == HAL_OK);
	uint32_t* display_start = (uint32_t*)(hltdc_eval.LayerCfg[0].FBStartAdress);// + 4 * y * tft_width);
	//   (uint8_t*)(display_start + (tft_width*tft_height)); // I_VideoBufferX2;
	DoubleScreen(I_VideoBuffer, I_VideoBufferX2, SCREENWIDTH, SCREENHEIGHT);
//	MODIFY_REG(DMA2D->OOR, DMA2D_OOR_LO, 800-(SCREENWIDTH*2));
	HAL_DMA2D_Start(&hdma2d_eval, (uint32_t)I_VideoBufferX2, (uint32_t)display_start, SCREENWIDTH*2,  SCREENHEIGHT*2);
}
void I_FinishUpdate (void)
{

	DMA2D_WAIT;
	//assert(HAL_DMA2D_PollForTransfer(&hdma2d_eval,50) == HAL_OK);
#ifdef SCALE_2X
	Update2XScale();
#else
	UpdateNoScale();
#endif
	DMA2D_WAIT;
}

//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, I_VideoBuffer, SCREENWIDTH * SCREENHEIGHT);
}


//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
	DMA2D_CLUT_WAIT;
	DMA2D_WAIT;

	//assert(HAL_DMA2D_PollForTransfer(&hdma2d_eval,50) == HAL_OK);
	//while(!pallete_load_complete);
//	pallete_load_complete = false;
	int i;
	col_t* c;
	bool palette_changed = false;
	for (i = 0; i < 256; i++)
	{
		c = (col_t*)palette;

		uint32_t nc = GFX_ARGB8888(c->r, c->g,c->b,GFX_OPAQUE);
		if(nc != rgb888_palette[i]){
			rgb888_palette[i] = nc;
			palette_changed = true;
		}
		//rgb565_palette[i] = GFX_RGB565(gammatable[usegamma][c->r], gammatable[usegamma][c->g],gammatable[usegamma][c->b]);
		palette += 3;
	}
	if(palette_changed){
		DMA2D_CLUTCfgTypeDef lut;
		lut.CLUTColorMode = DMA2D_CCM_ARGB8888;
		lut.Size = 0xFF;
		lut.pCLUT = rgb888_palette;

		HAL_DMA2D_ConfigCLUT(&hdma2d_eval, lut, 1);
		HAL_DMA2D_EnableCLUT(&hdma2d_eval, 1);
		DMA2D_CLUT_WAIT;
	//SET_BIT(hdma2d->Instance->FGPFCCR, DMA2D_FGPFCCR_START);
		//assert(HAL_DMA2D_PollForTransfer(&hdma2d_eval,50) == HAL_OK);
	}

	//while(HAL_DMA2D_PollForTransfer(&hdma2d_eval))
}

// Given an RGB value, find the closest matching palette index.

int I_GetPaletteIndex (int r, int g, int b)
{
    int best, best_diff, diff;
    int i;
    col_t color;

    best = 0;
    best_diff = INT_MAX;

    for (i = 0; i < 256; ++i)
    {
    //	color.r = GFX_RGB565_R(rgb565_palette[i]);
    //	color.g = GFX_RGB565_G(rgb565_palette[i]);
    //	color.b = GFX_RGB565_B(rgb565_palette[i]);
    	color.r = GFX_ARGB8888_R(rgb888_palette[i]);
    	color.g = GFX_ARGB8888_G(rgb888_palette[i]);
    	color.b = GFX_ARGB8888_B(rgb888_palette[i]);

        diff = (r - color.r) * (r - color.r)
             + (g - color.g) * (g - color.g)
             + (b - color.b) * (b - color.b);

        if (diff < best_diff)
        {
            best = i;
            best_diff = diff;
        }

        if (diff == 0)
        {
            break;
        }
    }

    return best;
}

void I_BeginRead (void)
{
}

void I_EndRead (void)
{
}

void I_SetWindowTitle (char *title)
{
}

void I_GraphicsCheckCommandLine (void)
{
}

void I_SetGrabMouseCallback (grabmouse_callback_t func)
{
}

void I_EnableLoadingDisk (void)
{
}

void I_BindVideoVariables (void)
{
}

void I_DisplayFPSDots (boolean dots_on)
{
}

void I_CheckIsScreensaver (void)
{
}
