/*
 * HID_Report_Parser.hpp
 *
 *  Created on: Jan 15, 2017
 *      Author: Paul
 */

#ifndef ST_HOST_LIBRARY_HID_REPORT_PARSER_HPP_
#define ST_HOST_LIBRARY_HID_REPORT_PARSER_HPP_


namespace HID {
	enum  PageType {
		Undefined = 0x00,
		GenericDesktopControls = 0x01,
		SimuationControls,
		VRControls,
		SportControls,
		GameControls,
		GenericDeviceControls,
		KeyboardKeypad,
		LEDS,
		Button,
		Ordinal,
		Telephony,
		Consumer,
		Digitizer,
		Reserved,
		PIDPage,
		Unicode = 0x10
	};

	enum class CollectionType {
		Physical,Application,Logical, Report,NamedArray,UsageSwitch,UsageModifier
	};


	class HID_Collection {


		CollectionType type;
		HID_Collection* _parent=nullptr;
		HID_Collection* _children=nullptr;
		HID_Collection* _next=nullptr;
	};
};




#endif /* ST_HOST_LIBRARY_HID_REPORT_PARSER_HPP_ */
