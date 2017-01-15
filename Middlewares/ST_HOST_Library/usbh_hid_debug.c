#include "usbh_hid_parser.h"


void print_buffer(uint8_t* data, size_t length){
	while(length){
		if(length>8) {
			printf("%2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X \r\n",
					data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
			length-=8;
			data+=8;
		} else {
			 while(length--) printf("%2.2X ", *data++);
			 printf("\r\n");
			 break;
		}
	}
}
void print_buffer_line(uint8_t* data, size_t length){
	while(length--) printf("%2.2X ", *data++);
}

void bBCD(unsigned int x)
{
	uint8_t upper = x >> 8;
	uint8_t lower = x & 0xFF;
	if(lower < 0x10)
		printf("%i.0%i",upper,lower);
	else
		printf("%i.%i",upper,lower);
}
/*
void pIndentComment(int ident, int bs, constx, bs)
{
	var y = " // ";
	var preSpace = 2 - bs;
	if (preSpace < 0) {
		preSpace = 0;
	}
	for (var i = 0; i < preSpace; i++) {
		y = "      " + y;
	}
	for (var i = 0; i < indent; i++) {
		y += "  ";
	}
	y += x;
	return y + "\r\n";
}
*/
typedef enum {
	HID_REPORTFEATURE_CONST = 0x01,
	HID_REPORTFEATURE_VAR = 0x02,
	HID_REPORTFEATURE_RELATIVE = 0x04,
	HID_REPORTFEATURE_WRAP = 0x08,
	HID_REPORTFEATURE_NONLINEAR = 0x10,
	HID_REPORTFEATURE_NOPREFEREDSTATE= 0x20,
//	HID_REPORTFEATURE_NULLSTATE = 0x40,
} t_HID_InputOutputFeature;
const char* pInputOutputFeature(int s,int  v, int t)
{
	static char buffer[200];

	if (s <= 0) return "";
	buffer[0] = 0;
	strcat(buffer, (v & 0x01) == 0? "Data": "Const");
	strcat(buffer, (v & 0x02) == 0? ",Array": ",Var");
	strcat(buffer, (v & 0x04) == 0? ",Abs": ",Rel");
	strcat(buffer, (v & 0x08) == 0? ",No Wrap,": ",Wrap");
	strcat(buffer, (v & 0x10) == 0? ",Linear": ",Nonlinear");
	strcat(buffer, (v & 0x20) == 0? ",Preferred State": ",No Preferred State");
	strcat(buffer, (v & 0x20) == 0? ",No Null Position": ",Null State");
	if (t != 0x08) strcat(buffer, (v & 0x80) == 0? ",Non-volatile": ",Volatile");
	if (s > 1) strcat(buffer, (v & 0x100) == 0? ",Bit Field": ",Buffered Bytes");
	return buffer;
}
typedef enum {
	HID_USAGEPAGE_UNDEFINED = 0,
	HID_USAGEPAGE_GENERIC_DESKTOP_CTRL,
	HID_USAGEPAGE_GENERIC_SIM_CTRL,
	HID_USAGEPAGE_GENERIC_VR_CTRL,
	HID_USAGEPAGE_GENERIC_GAME_CTRL,
	HID_USAGEPAGE_GENERIC_GENERIC_DEV_CTRL,
	HID_USAGEPAGE_GENERIC_KEYBOARD_KEYPAD,
	HID_USAGEPAGE_GENERIC_LEDS,
	HID_USAGEPAGE_GENERIC_BUTTON,
	HID_USAGEPAGE_GENERIC_ORDINAL,
	HID_USAGEPAGE_GENERIC_TELEPHONY,
	HID_USAGEPAGE_GENERIC_CONSUMER,
	HID_USAGEPAGE_GENERIC_DIGITIZER,
	HID_USAGEPAGE_GENERIC_RESERVED_0E,
	HID_USAGEPAGE_GENERIC_PID_PAGE,
	HID_USAGEPAGE_GENERIC_UNICODE,
	HID_USAGEPAGE_GENERIC_RESERVED_11,
	HID_USAGEPAGE_GENERIC_RESERVED_12,
	HID_USAGEPAGE_GENERIC_RESERVED_13,
	HID_USAGEPAGE_GENERIC_ALPHANUMERIC_DISPLAY,
} t_HID_USAGEPAGE;


