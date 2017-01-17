/**
  ******************************************************************************
  * @file    usbh_hid_parser.c 
  * @author  MCD Application Team
  * @version V3.2.2
  * @date    07-July-2015
  * @brief   This file is the header file of the usbh_hid_parser.c        
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
#ifndef __USBH_HID_PARSER_H
#define __USBH_HID_PARSER_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbh_core.h"
#include "usbh_hid_usage.h"

 typedef enum
 {
   HID_REPORT_OK=0,
   HID_REPORT_INVALID=1,
   HID_REPORT_MISSING_COLLECTION_END=2,
   HID_REPORT_MISSING_USAGE=4
 } HID_ReportStatusTypeDef;
/* Type Defines: */
/** Enum for possible Class, Subclass and Protocol values of device and interface descriptors relating to the HID
 *  device class.
 */
enum HID_Descriptor_ClassSubclassProtocol_t
{
	HID_CSCP_HIDClass             = 0x03, /**< Descriptor Class value indicating that the device or interface
										   *   belongs to the HID class.
										   */
	HID_CSCP_NonBootSubclass      = 0x00, /**< Descriptor Subclass value indicating that the device or interface
										   *   does not implement a HID boot protocol.
										   */
	HID_CSCP_BootSubclass         = 0x01, /**< Descriptor Subclass value indicating that the device or interface
										   *   implements a HID boot protocol.
										   */
	HID_CSCP_NonBootProtocol      = 0x00, /**< Descriptor Protocol value indicating that the device or interface
										   *   does not belong to a HID boot protocol.
										   */
	HID_CSCP_KeyboardBootProtocol = 0x01, /**< Descriptor Protocol value indicating that the device or interface
										   *   belongs to the Keyboard HID boot protocol.
										   */
	HID_CSCP_MouseBootProtocol    = 0x02, /**< Descriptor Protocol value indicating that the device or interface
										   *   belongs to the Mouse HID boot protocol.
										   */
};

/** Enum for the HID class specific control requests that can be issued by the USB bus host. */
enum HID_ClassRequests_t
{
	HID_REQ_GetReport       = 0x01, /**< HID class-specific Request to get the current HID report from the device. */
	HID_REQ_GetIdle         = 0x02, /**< HID class-specific Request to get the current device idle count. */
	HID_REQ_GetProtocol     = 0x03, /**< HID class-specific Request to get the current HID report protocol mode. */
	HID_REQ_SetReport       = 0x09, /**< HID class-specific Request to set the current HID report to the device. */
	HID_REQ_SetIdle         = 0x0A, /**< HID class-specific Request to set the device's idle count. */
	HID_REQ_SetProtocol     = 0x0B, /**< HID class-specific Request to set the current HID report protocol mode. */
};

/** Enum for the HID class specific descriptor types. */
enum HID_DescriptorTypes_t
{
	HID_DTYPE_HID           = 0x21, /**< Descriptor header type value, to indicate a HID class HID descriptor. */
	HID_DTYPE_Report        = 0x22, /**< Descriptor header type value, to indicate a HID class HID report descriptor. */
};

/** Enum for the different types of HID reports. */
enum HID_ReportItemTypes_t
{
	HID_REPORT_ITEM_In      = 0, /**< Indicates that the item is an IN report type. */
	HID_REPORT_ITEM_Out     = 1, /**< Indicates that the item is an OUT report type. */
	HID_REPORT_ITEM_Feature = 2, /**< Indicates that the item is a FEATURE report type. */
};
 typedef struct _HID_MinMax
 {
 	uint32_t Minimum; /**< Minimum value for the attribute. */
 	uint32_t Maximum; /**< Maximum value for the attribute. */
 } HID_MinMaxTypeDef;

 typedef struct _HID_Unit {
 	uint32_t Type;     /**< Unit type (refer to HID specifications for details). */
 	uint8_t  Exponent; /**< Unit exponent (refer to HID specifications for details). */
 } HID_UnitTypeDef;

 typedef struct HID_Usage
 {
 	uint16_t Page;  /**< Usage page of the report item. */
 	uint16_t Usage; /**< Usage of the report item. */
 } HID_UsageTypeDef;
// forwards

 struct _HID_ReportItem;

 typedef struct _HID_CollectionPath
 {
 	uint8_t                     Type;   /**< Collection type (e.g. "Generic Desktop"). */
 	HID_UsageTypeDef            Usage;  /**< Collection usage. */
 	struct _HID_CollectionPath* Parent; /**< Reference to parent collection, or \c NULL if root collection. */
 	struct _HID_ReportItem* Reports; // Report list
 } HID_CollectionPathTypeDef;

 typedef struct _HID_ReportID
 {
	 uint16_t ID;
 	HID_UsageTypeDef Usage;
 	uint16_t ReportSizeBits[3];		// size of the ReportID
 	struct _HID_ReportItem* Reports; // list of reports by this ID
 } HID_ReportIDTypeDef;

 typedef struct _HID_ReportItem_Attributes
 {
 	uint8_t      		BitSize;  /**< Size in bits of the report item's data. */
 	//HID_UsageTypeDef  	Usage;    /**< Usage of the report item. */
 	HID_UnitTypeDef   	Unit;     /**< Unit type and exponent of the report item. */
 	HID_MinMaxTypeDef 	Logical;  /**< Logical minimum and maximum of the report item. */
 	HID_MinMaxTypeDef 	Physical; /**< Physical minimum and maximum of the report item. */
 } HID_ReportItem_AttributesTypeDef;

 typedef struct _HID_ReportItem
 {
	HID_UsageTypeDef Usage; // moved here
 	uint16_t                          BitOffset;      /// Bit offset in the IN, OUT or FEATURE report of the item.
 	uint8_t                           ItemType;       /// Report item type, a value in \ref HID_ReportItemTypes_t.
 	uint16_t                    	  ItemFlags;      /// Item data flags, a mask of \c HID_IOF_* constants.
 	/// Current value of the report item - use \ref HID_ALIGN_DATA() when processing
 	///   a retrieved value so that it is aligned to a specific type.
 	uint32_t                    Value;
 	uint32_t                    PreviousValue;  /**< Previous value of the report item. */
 	const HID_ReportIDTypeDef const* ReportID;       /// Report ID this item belongs to, or NULL if only one report
 	const HID_CollectionPathTypeDef const* CollectionPath; /**< Collection path of the item. */
 	const HID_ReportItem_AttributesTypeDef const* Attributes;     /**< Report item attributes. */
 	struct _HID_ReportItem* NextInCollection; // Link list of report items in collection
 	struct _HID_ReportItem* NextInReportID; // Link list of report items by report id
 } HID_ReportItemTypeDef;

 typedef struct _HID_ReportItemCollection
 {
	 uint16_t TotalReportItems;
	 uint16_t TotalDeviceReports;
	 uint16_t LargestReportSizeBits;
	 HID_ReportItemTypeDef* ReportArray; // array of reports
	 HID_ReportIDTypeDef* ReportIDArray; // array of reports IDs
	 HID_CollectionPathTypeDef* RootCollectionList; //  linked list of collections
 } HID_ReportItemCollectionTypeDef;

 #define HID_ALIGN_DATA(ReportItem, Type) ((Type)(ReportItem->Value << ((8 * sizeof(Type)) - ReportItem->Attributes.BitSize)))


typedef struct 
{
  uint8_t  *data;
  uint32_t size;
  uint8_t  shift;
  uint8_t  count;
  uint8_t  sign;
  uint32_t logical_min;  /*min value device can return*/
  uint32_t logical_max;  /*max value device can return*/
  uint32_t physical_min; /*min vale read can report*/
  uint32_t physical_max; /*max value read can report*/
  uint32_t resolution;
} 
HID_Report_ItemTypedef;

// various utility fuctions to pre_setup the size of the collection

// Counts the number of reports in the report as well as validates the report

// This will parse and allocate the report.  You can do a standard USBH_Free on this
// entire structure and it will free up ALL the allocated memory
HID_ReportItemCollectionTypeDef*  USBH_HID_AllocHIDReport(uint8_t* data, size_t data_size);

// both for debugging
void IdentLine(int ident,const char* fmt, ...);
void print_buffer(uint8_t* data, size_t length);

uint32_t HID_ReadItem (HID_Report_ItemTypedef *ri, uint8_t ndx);
uint32_t HID_WriteItem(HID_Report_ItemTypedef *ri, uint32_t value, uint8_t ndx);


/**
  * @}
  */ 

#ifdef __cplusplus
}
#endif

#endif /* __USBH_HID_PARSER_H */

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
