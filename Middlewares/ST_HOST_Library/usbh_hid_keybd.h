/**
  ******************************************************************************
  * @file    usbh_hid_keybd.h
  * @author  MCD Application Team
  * @version V3.2.2
  * @date    07-July-2015
  * @brief   This file contains all the prototypes for the usbh_hid_keybd.c
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Define to prevent recursive -----------------------------------------------*/
#ifndef __USBH_HID_KEYBD_H
#define __USBH_HID_KEYBD_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbh_hid.h"
//#include "usbh_hid_keybd.h"

/** @addtogroup USBH_LIB
  * @{
  */

/** @addtogroup USBH_CLASS
  * @{
  */

/** @addtogroup USBH_HID_CLASS
  * @{
  */
  
/** @defgroup USBH_HID_KEYBD
  * @brief This file is the Header file for usbh_hid_keybd.c
  * @{
  */ 


/** @defgroup USBH_HID_KEYBD_Exported_Types
  * @{
  */ 
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


/**
  * @}
  */ 

#ifdef __cplusplus
}
#endif

#endif /* __USBH_HID_KEYBD_H */

/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */ 
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

