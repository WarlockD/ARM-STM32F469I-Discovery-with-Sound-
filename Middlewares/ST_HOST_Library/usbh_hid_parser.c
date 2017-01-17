/**
  ******************************************************************************
  * @file    usbh_hid_parser.c
  * @author  MCD Application Team
  * @version V3.2.2
  * @date    07-July-2015
  * @brief   This file is the HID Layer Handlers for USB Host HID class.
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
/* Includes ------------------------------------------------------------------*/
#include "usbh_hid_parser.h"
#include "usbh_hid_parser_report_data.h"
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#ifndef  MAX
	#define MAX(x, y)               (((x) > (y)) ? (x) : (y))
#endif
#ifndef  MIN
	#define MIN(x, y)               (((x) < (y)) ? (x) : (y))
#endif
#define HID_STATETABLE_STACK_DEPTH    2
#define HID_USAGE_STACK_DEPTH         8
#define HID_MAX_COLLECTIONS           50
#define HID_MAX_REPORTITEMS           50
#define HID_MAX_REPORT_IDS            10
#define HID_MAX_ATTRIBUT              50 // this is done because attributes are copyed

enum HID_Parse_ErrorCodes_t
{
	HID_PARSE_Successful                  = 0, /**< Successful parse of the HID report descriptor, no error. */
	HID_PARSE_HIDStackOverflow            = 1, /**< More than \ref HID_STATETABLE_STACK_DEPTH nested PUSHes in the report. */
	HID_PARSE_HIDStackUnderflow           = 2, /**< A POP was found when the state table stack was empty. */
	HID_PARSE_InsufficientReportItems     = 3, /**< More than \ref HID_MAX_REPORTITEMS report items in the report. */
	HID_PARSE_UnexpectedEndCollection     = 4, /**< An END COLLECTION item found without matching COLLECTION item. */
	HID_PARSE_InsufficientCollectionPaths = 5, /**< More than \ref HID_MAX_COLLECTIONS collections in the report. */
	HID_PARSE_UsageListOverflow           = 6, /**< More than \ref HID_USAGE_STACK_DEPTH usages listed in a row. */
	HID_PARSE_InsufficientReportIDItems   = 7, /**< More than \ref HID_MAX_REPORT_IDS report IDs in the device. */
	HID_PARSE_NoUnfilteredReportItems     = 8, /**< All report items from the device were filtered by the filtering callback routine. */
};
// private functions, but can be exported to usbh_hid
typedef struct {
	uint16_t Size;
	uint16_t Pos;
	uint8_t* Data;
} HID_ReportDataStreamTypeDef;

typedef struct _HID_ReportDataItem{
	uint8_t Item;
	uint32_t Data;
} HID_ReportDataItemTypeDef;

typedef struct _HID_StateTable
{
	HID_ReportItem_AttributesTypeDef Attributes;
	HID_UsageTypeDef Usage;
	uint8_t                          ReportCount;
	HID_ReportIDTypeDef*           ReportID;
} HID_StateTableTypeDef;

static HID_StateTableTypeDef      			_StateTable[HID_STATETABLE_STACK_DEPTH];
static HID_CollectionPathTypeDef  			_CollectionPaths[HID_MAX_COLLECTIONS];
static HID_ReportIDTypeDef  				_ReportIDTable[HID_MAX_REPORT_IDS];
static HID_ReportItemTypeDef	    		_ReportItems[HID_MAX_REPORTITEMS];
static HID_ReportItem_AttributesTypeDef 	_AttributeCache[HID_MAX_REPORTITEMS];
static HID_ReportItemCollectionTypeDef 		_HIDReportCache;
typedef struct _HID_ReportParser {
	HID_ReportItemCollectionTypeDef* HIDReport;
	HID_ReportItem_AttributesTypeDef Attributes;
	HID_ReportDataStreamTypeDef stream;
	HID_ReportDataItemTypeDef item;
	HID_CollectionPathTypeDef*  CollectionPaths;
	HID_CollectionPathTypeDef* 	CurrCollectionPath;
	HID_StateTableTypeDef*     	CurrStateTable;
	HID_ReportIDTypeDef* 		CurrReportIDInfo;
	HID_ReportItem_AttributesTypeDef* 		AttributeCache;
	uint16_t AttributesUsed;
	uint16_t TotalCollectionPaths;
} HID_ReportParserTypeDef;

enum HID_Parse_ErrorCodes_t  USBH_HID_ParseHIDReport(HID_ReportParserTypeDef* parser);
void USBH_HID_InitParser(HID_ReportParserTypeDef* parser, uint8_t* data, size_t data_size);


uint8_t NextItem(HID_ReportDataStreamTypeDef* stream, HID_ReportDataItemTypeDef*item){
	if(stream->Pos == stream->Size) return 0;
	item->Item = stream->Data[stream->Pos++];
	if(stream->Pos == stream->Size) return 0;

	switch (item->Item & HID_RI_DATA_SIZE_MASK)
	{
		case HID_RI_DATA_BITS_32:
			if((stream->Pos+4) > stream->Size) return 0;
			item->Data  =
					(((uint32_t)stream->Data[stream->Pos +3] << 24) |
					((uint32_t)stream->Data[stream->Pos +2] << 16) |
					((uint16_t)stream->Data[stream->Pos +1] << 8)  |
					((uint16_t)stream->Data[stream->Pos +0]));
			stream->Pos     += 4;
			break;
		case HID_RI_DATA_BITS_16:
			if((stream->Pos+2) > stream->Size) return 0;
			item->Data  = (((uint16_t)stream->Data[stream->Pos +1] << 8) | (stream->Data[stream->Pos +0]));
			stream->Pos     += 2;
			break;
		case HID_RI_DATA_BITS_8:
			if((stream->Pos+1) > stream->Size) return 0;
			item->Data  = stream->Data[stream->Pos];
			stream->Pos     += 1;
			break;
		default:
			item->Data  = 0;
			break;
	}
	return 1;
}


HID_ReportItem_AttributesTypeDef* USBH_HID_FindAttributeMatch(HID_ReportParserTypeDef* parser, HID_ReportItem_AttributesTypeDef* test){
	if(parser->AttributesUsed > 0){
		for(int32_t pos = parser->AttributesUsed-1; pos >=0;pos--){
			// we search backwards because most of the attributes that would be equal would be
			// the last one inserted
			if(memcmp(&parser->AttributeCache[pos], test,sizeof(HID_ReportItem_AttributesTypeDef))==0)
				return &parser->AttributeCache[pos];
		}
	}
	// no match, add and return
	memcpy(&parser->AttributeCache[parser->AttributesUsed],test,sizeof(HID_ReportItem_AttributesTypeDef));
	return &parser->AttributeCache[parser->AttributesUsed++];
}
HID_ReportIDTypeDef* USBH_HID_FindReportID(HID_ReportParserTypeDef* parser, uint16_t report){
	for(int32_t pos = 0; pos < parser->HIDReport->TotalDeviceReports; pos++){
		if(report == parser->HIDReport->ReportIDArray[pos].ID)
			return &parser->HIDReport->ReportIDArray[pos];
	}
	if (parser->HIDReport->TotalDeviceReports == HID_MAX_REPORT_IDS) return NULL;
	HID_ReportIDTypeDef* ret = &parser->HIDReport->ReportIDArray[parser->HIDReport->TotalDeviceReports++];
	// if we cannot find it, make one
	memset(ret,       0x00, sizeof(HID_ReportIDTypeDef));
	ret->ID = report;
	return ret;
}

void USBH_HID_ResetParser(HID_ReportParserTypeDef* parser){
	parser->AttributesUsed=0;
	parser->TotalCollectionPaths = 0;
	parser->HIDReport->LargestReportSizeBits=0;
	parser->HIDReport->TotalDeviceReports = 0;
	parser->HIDReport->TotalReportItems = 0;
	parser->stream.Pos = 0;
	parser->CurrStateTable = &_StateTable[0];
	parser->CurrCollectionPath = &parser->CollectionPaths[0];
	parser->CurrReportIDInfo = USBH_HID_FindReportID(parser, 0);
	memset(parser->CurrStateTable,   0x00, sizeof(HID_StateTableTypeDef));
	memset(parser->CurrCollectionPath,   0x00, sizeof(HID_CollectionPathTypeDef));
}

void USBH_HID_InitParser(HID_ReportParserTypeDef* parser, uint8_t* data, size_t data_size){
	memset(&_HIDReportCache,0,sizeof(HID_ReportItemCollectionTypeDef));
	parser->stream.Data = data;
	parser->stream.Size = data_size;
	parser->HIDReport = &_HIDReportCache;
	parser->CollectionPaths = _CollectionPaths;
	parser->HIDReport->ReportArray = _ReportItems;
	parser->HIDReport->ReportIDArray = _ReportIDTable;
	parser->HIDReport->RootCollectionList = _CollectionPaths;
	parser->AttributeCache = _AttributeCache;
	USBH_HID_ResetParser(parser);
}
void IdentLine(int ident,const char* fmt, ...) {
	char line[128];
	if(ident>0)memset(line,'\t',ident);
	va_list va;
	va_start(va,fmt);
	vsprintf(line+ident,fmt,va);
	va_end(va);
	printf("%s\r\n", line);
}
void USBH_HID_PrintAttribute(int ident, const HID_ReportItem_AttributesTypeDef* item){
	if(item==NULL) IdentLine(ident,"NULL Attributes?");
	IdentLine(ident, "BitSize=%lu",(uint32_t)item->BitSize);
	IdentLine(ident, "Logical  Min=%lu Max=%lu",(uint32_t)item->Logical.Minimum, (uint32_t)item->Logical.Maximum);
	IdentLine(ident, "Physical Min=%lu Max=%lu",(uint32_t)item->Physical.Minimum, (uint32_t)item->Physical.Maximum);
}
void USBH_HID_PrintReportItem(int ident, const HID_ReportItemTypeDef* item){
	IdentLine(ident, "Page=%X Usage=%X ReportID=%lu BitOffset=%lu ItemType=%X ItemFlags=%X",
			(uint32_t)item->Usage.Page,(uint32_t)item->Usage.Usage,
			(uint32_t)item->ReportID->ID, (uint32_t)item->BitOffset,
			(uint32_t)item->ItemType,(uint32_t)item->ItemFlags);
	ident++;
	USBH_HID_PrintAttribute(ident, item->Attributes);
}
void USBH_HID_PrintReportID(int ident, const HID_ReportIDTypeDef* id){
	IdentLine(ident, "ID=%lu Page=%X Usage=%X Sizes=%u,%u,%u",
			(uint32_t)id->ID,(uint32_t)id->Usage.Page, (uint32_t)id->Usage.Page, (uint32_t)id->ReportSizeBits[0],
			(uint32_t)id->ReportSizeBits[1], (uint32_t)id->ReportSizeBits[2]);

}
void USBH_HID_PrintReports(HID_ReportItemCollectionTypeDef* HIDReport){
	int ident=0;
	IdentLine(ident, "LargestReportSizeBits=%lu Bytes=%u", (uint32_t)HIDReport->LargestReportSizeBits,(uint32_t)HIDReport->LargestReportSizeBits/8);
	IdentLine(ident, "TotalDeviceReports=%lu",(uint32_t) HIDReport->TotalDeviceReports);
	++ident;
	for(uint32_t i=0; i < HIDReport->TotalDeviceReports;i++)
		USBH_HID_PrintReportID(ident,&HIDReport->ReportIDArray[i]);
	--ident;
	IdentLine(ident, "TotalReportItems=%lu", (uint32_t)HIDReport->TotalReportItems);
	++ident;
	for(uint32_t i=0; i < HIDReport->TotalReportItems;i++)
		USBH_HID_PrintReportItem(ident,&HIDReport->ReportArray[i]);
	--ident;
}
HID_ReportItemCollectionTypeDef*  USBH_HID_AllocHIDReport(uint8_t* data, size_t data_size){
	print_buffer(data,data_size);
	HID_ReportParserTypeDef parser; // ho much stack does this take?
	HID_ReportItemCollectionTypeDef* HIDReport  = NULL;
	USBH_HID_InitParser(&parser,data,data_size);
	if(USBH_HID_ParseHIDReport(&parser)==HID_PARSE_Successful){
		// parsed! Ok, lets build things allocate the reports
		uint32_t ReportItemCollectionSize = sizeof(HID_ReportItemCollectionTypeDef);
		uint32_t ReportArraySize = sizeof(HID_ReportItemTypeDef) * parser.HIDReport->TotalReportItems;
		uint32_t ReportIDArraySize = sizeof(HID_ReportIDTypeDef) * parser.HIDReport->TotalDeviceReports;
		uint32_t AttributeCacheSize = sizeof(HID_ReportItem_AttributesTypeDef) * parser.AttributesUsed;
		uint32_t CollectionPathSize = sizeof(HID_ReportItemCollectionTypeDef) * parser.TotalCollectionPaths;
		uint32_t TotalSize = ReportItemCollectionSize + ReportArraySize + ReportIDArraySize + AttributeCacheSize+CollectionPathSize;
		printf("Parse happening %lu, %lu, %lu, %lu, %lu\r\n",
				(uint32_t)parser.HIDReport->TotalReportItems,
				(uint32_t)parser.HIDReport->TotalDeviceReports,
				(uint32_t)parser.AttributesUsed,
				(uint32_t)parser.TotalCollectionPaths,
				(uint32_t)TotalSize
		);
		uint8_t* buffer = USBH_malloc(TotalSize);
		memset(buffer,0,TotalSize); // might not need this?
		HIDReport = (HID_ReportItemCollectionTypeDef*)buffer;
		buffer+=ReportItemCollectionSize;
		parser.HIDReport = HIDReport;
		HIDReport->ReportArray = (HID_ReportItemTypeDef*)buffer;
		buffer+=ReportArraySize;
		HIDReport->ReportIDArray = (HID_ReportIDTypeDef*)buffer;
		buffer+=ReportIDArraySize;
		parser.AttributeCache  = (HID_ReportItem_AttributesTypeDef*)buffer;
		buffer+=AttributeCacheSize;
		parser.CurrCollectionPath = (HID_CollectionPathTypeDef*)buffer;
		buffer+=CollectionPathSize;
		USBH_HID_ResetParser(&parser); // reset it
		// then do it again.  Hate how you have to do this twice, but I cannot
		// think of an easier way with it fixing all the links and such
		if(USBH_HID_ParseHIDReport(&parser)==HID_PARSE_Successful){
			USBH_HID_PrintReports(HIDReport);
			return HIDReport;
		} else {
			printf("Parser fail: The sequal!\r\n");
			USBH_free(buffer);
		}
	}
	printf("Parser fail!\r\n");
	return NULL;
}

// This will parse and allocate the report.  You can do a standard USBH_Free on this
// entire structure and it will free up ALL the allocated memory
enum HID_Parse_ErrorCodes_t  USBH_HID_ParseHIDReport(HID_ReportParserTypeDef* parser){
	HID_ReportDataItemTypeDef item = {0,0 };
	uint16_t              UsageList[HID_USAGE_STACK_DEPTH];
	uint8_t               UsageListSize      = 0;
	HID_MinMaxTypeDef     UsageMinMax        = {0, 0};

	while (NextItem(&parser->stream,&item))
	{
		switch (item.Item & (HID_RI_TYPE_MASK | HID_RI_TAG_MASK))
		{
			case HID_RI_PUSH(0):
				if (parser->CurrStateTable == &_StateTable[HID_STATETABLE_STACK_DEPTH - 1])
				  return HID_PARSE_HIDStackOverflow;

				memcpy((parser->CurrStateTable + 1),
						parser->CurrStateTable,
					   sizeof(HID_StateTableTypeDef));

				parser->CurrStateTable++;
				break;

			case HID_RI_POP(0):
				if (parser->CurrStateTable == &_StateTable[0])
				  return HID_PARSE_HIDStackUnderflow;

			parser->CurrStateTable--;
				break;

			case HID_RI_USAGE_PAGE(0):
				if ((item.Item  & HID_RI_DATA_SIZE_MASK) == HID_RI_DATA_BITS_32)
					parser->CurrStateTable->Usage.Page = (item.Data  >> 16);

			parser->CurrStateTable->Usage.Page       = item.Data ;
				break;

			case HID_RI_LOGICAL_MINIMUM(0):
		parser->CurrStateTable->Attributes.Logical.Minimum  = item.Data;
				break;

			case HID_RI_LOGICAL_MAXIMUM(0):
		parser->CurrStateTable->Attributes.Logical.Maximum  = item.Data;
				break;

			case HID_RI_PHYSICAL_MINIMUM(0):
		parser->CurrStateTable->Attributes.Physical.Minimum = item.Data;
				break;

			case HID_RI_PHYSICAL_MAXIMUM(0):
		parser->CurrStateTable->Attributes.Physical.Maximum = item.Data;
				break;

			case HID_RI_UNIT_EXPONENT(0):
		parser->CurrStateTable->Attributes.Unit.Exponent    = item.Data;
				break;

			case HID_RI_UNIT(0):
		parser->CurrStateTable->Attributes.Unit.Type        = item.Data;
				break;

			case HID_RI_REPORT_SIZE(0):
		parser->CurrStateTable->Attributes.BitSize          = item.Data;
				break;

			case HID_RI_REPORT_COUNT(0):
		parser->CurrStateTable->ReportCount                 = item.Data;
				break;

			case HID_RI_REPORT_ID(0):
				parser->CurrStateTable->ReportID = USBH_HID_FindReportID(parser,item.Data);
				if(parser->CurrStateTable->ReportID == NULL) return HID_PARSE_InsufficientReportIDItems;
				break;

			case HID_RI_USAGE(0):
				if (UsageListSize == HID_USAGE_STACK_DEPTH)
				  return HID_PARSE_UsageListOverflow;

				UsageList[UsageListSize++] = item.Data;
				break;

			case HID_RI_USAGE_MINIMUM(0):
				UsageMinMax.Minimum = item.Data;
				break;

			case HID_RI_USAGE_MAXIMUM(0):
				UsageMinMax.Maximum = item.Data;
				break;

			case HID_RI_COLLECTION(0):
				if (parser->CurrCollectionPath == NULL)
				{
					parser->CurrCollectionPath = &parser->CollectionPaths[0];
				}
				else
				{
					HID_CollectionPathTypeDef* ParentCollectionPath = parser->CurrCollectionPath;
					parser->CurrCollectionPath = &parser->CollectionPaths[1];

					while (parser->CurrCollectionPath->Parent != NULL)
					{
						if (parser->CurrCollectionPath == &parser->CollectionPaths[HID_MAX_COLLECTIONS - 1])
						  return HID_PARSE_InsufficientCollectionPaths;

						parser->CurrCollectionPath++;
					}

					parser->CurrCollectionPath->Parent = ParentCollectionPath;
				}

				parser->CurrCollectionPath->Type       = item.Data;
				parser->CurrCollectionPath->Usage.Page = parser->CurrStateTable->Usage.Page;
				parser->TotalCollectionPaths++;

				if (UsageListSize)
				{
					parser->CurrCollectionPath->Usage.Usage = UsageList[0];

					for (uint8_t i = 1; i < UsageListSize; i++)
					  UsageList[i - 1] = UsageList[i];

					UsageListSize--;
				}
				else if (UsageMinMax.Minimum <= UsageMinMax.Maximum)
				{
					parser->CurrCollectionPath->Usage.Usage = UsageMinMax.Minimum++;
				}

				break;

			case HID_RI_END_COLLECTION(0):
				if (parser->CurrCollectionPath == NULL)
				  return HID_PARSE_UnexpectedEndCollection;

			parser->CurrCollectionPath = parser->CurrCollectionPath->Parent;
				break;

			case HID_RI_INPUT(0):
			case HID_RI_OUTPUT(0):
			case HID_RI_FEATURE(0):
				for (uint8_t ReportItemNum = 0; ReportItemNum < parser->CurrStateTable->ReportCount; ReportItemNum++)
				{
					HID_ReportItemTypeDef* NewReportItem = &parser->HIDReport->ReportArray[parser->HIDReport->TotalReportItems];

					NewReportItem->Attributes     = USBH_HID_FindAttributeMatch(parser, &parser->CurrStateTable->Attributes);
					NewReportItem->Usage		  = parser->CurrStateTable->Usage;
					NewReportItem->ItemFlags      = item.Data;
					NewReportItem->CollectionPath = parser->CurrCollectionPath;
					NewReportItem->ReportID       = parser->CurrStateTable->ReportID;
					NewReportItem->Value		  = 0;
					NewReportItem->PreviousValue  = 0; // not sure I want to keep?

					if (UsageListSize)
					{
						NewReportItem->Usage.Usage = UsageList[0];

						for (uint8_t i = 1; i < UsageListSize; i++)
						  UsageList[i - 1] = UsageList[i];

						UsageListSize--;
					}
					else if (UsageMinMax.Minimum <= UsageMinMax.Maximum)
					{
						NewReportItem->Usage.Usage = UsageMinMax.Minimum++;
					}

					uint8_t ItemTypeTag = (item.Data & (HID_RI_TYPE_MASK | HID_RI_TAG_MASK));

					if (ItemTypeTag == HID_RI_INPUT(0))
					  NewReportItem->ItemType = HID_REPORT_ITEM_In;
					else if (ItemTypeTag == HID_RI_OUTPUT(0))
					  NewReportItem->ItemType = HID_REPORT_ITEM_Out;
					else
					  NewReportItem->ItemType = HID_REPORT_ITEM_Feature;

					NewReportItem->BitOffset = parser->CurrReportIDInfo->ReportSizeBits[NewReportItem->ItemType];

					parser->CurrReportIDInfo->ReportSizeBits[NewReportItem->ItemType] += parser->CurrStateTable->Attributes.BitSize;

					parser->HIDReport->LargestReportSizeBits = MAX(parser->HIDReport->LargestReportSizeBits, parser->CurrReportIDInfo->ReportSizeBits[NewReportItem->ItemType]);

					if (parser->HIDReport->TotalReportItems == HID_MAX_REPORTITEMS)
					  return HID_PARSE_InsufficientReportItems;
					// link the collection list
					if(parser->CurrCollectionPath){
						if(parser->CurrCollectionPath->Reports==NULL) {
							parser->CurrCollectionPath->Reports = NewReportItem;
						} else {
							HID_ReportItemTypeDef* root = parser->CurrCollectionPath->Reports;
							while(root->NextInCollection) root = root->NextInCollection;
							root->NextInCollection = NewReportItem;
						}
						NewReportItem->NextInCollection = NULL;
					}
					// link the report id list
					if(parser->CurrStateTable->ReportID){
						if(parser->CurrStateTable->ReportID->Reports==NULL) {
							parser->CurrStateTable->ReportID->Reports = NewReportItem;
						} else {
							HID_ReportItemTypeDef* root = parser->CurrStateTable->ReportID->Reports;
							while(root->NextInReportID) root = root->NextInReportID;
							root->NextInReportID = NewReportItem;
						}
						NewReportItem->NextInReportID = NULL;
					}

					if(!(item.Data & HID_IOF_CONSTANT)) parser->HIDReport->TotalReportItems++;
				}

				break;

			default:
				break;
		}

		if ((item.Data & HID_RI_TYPE_MASK) == HID_RI_TYPE_MAIN)
		{
			UsageMinMax.Minimum = 0;
			UsageMinMax.Maximum = 0;
			UsageListSize       = 0;
		}
	}

	return HID_PARSE_Successful;
}



/**
  * @brief  HID_ReadItem 
  *         The function read a report item.
  * @param  ri: report item
  * @param  ndx: report index
* @retval status (0 : fail / otherwise: item value)
  */
uint32_t HID_ReadItem(HID_Report_ItemTypedef *ri, uint8_t ndx)
{
  uint32_t val=0;
  uint32_t x=0;
  uint32_t bofs;
  uint8_t *data=ri->data;
  uint8_t shift=ri->shift;
  
  /* get the logical value of the item */
  
  /* if this is an array, wee may need to offset ri->data.*/
  if (ri->count > 0)
  { 
    /* If app tries to read outside of the array. */
    if (ri->count <= ndx)
    {
      return(0);
    }
    
    /* calculate bit offset */
    bofs = ndx*ri->size;
    bofs += shift;
    /* calculate byte offset + shift pair from bit offset. */    
    data+=bofs/8;
    shift=(uint8_t)(bofs%8);
  }
  /* read data bytes in little endian order */
  for(x=0; x < ((ri->size & 0x7) ? (ri->size/8)+1 : (ri->size/8)); x++)
  {
    val=(uint32_t)(*data << (x*8));
  }    
  val=(val >> shift) & ((1<<ri->size)-1);
  
  if (val < ri->logical_min || val > ri->logical_max)
  {
    return(0);
  }
  
  /* convert logical value to physical value */
  /* See if the number is negative or not. */
  if ((ri->sign) && (val & (1<<(ri->size-1))))
  {
    /* yes, so sign extend value to 32 bits. */
    int vs=(int)((-1 & ~((1<<(ri->size))-1)) | val);
    
    if(ri->resolution == 1)
    {
      return((uint32_t)vs);
    }
    return((uint32_t)(vs*ri->resolution));
  }
  else
  {
    if(ri->resolution == 1)
    {
      return(val);
    }
    return(val*ri->resolution);    
  }  
}

/**
  * @brief  HID_WriteItem 
  *         The function write a report item.
  * @param  ri: report item
  * @param  ndx: report index
  * @retval status (1: fail/ 0 : Ok)
  */
uint32_t HID_WriteItem(HID_Report_ItemTypedef *ri, uint32_t value, uint8_t ndx)
{
  uint32_t x;
  uint32_t mask;
  uint32_t bofs;
  uint8_t *data=ri->data;
  uint8_t shift=ri->shift;
  
  if (value < ri->physical_min || value > ri->physical_max)  
  {
    return(1);
  }
  
    /* if this is an array, wee may need to offset ri->data.*/
  if (ri->count > 0)
  { 
    /* If app tries to read outside of the array. */
    if (ri->count >= ndx)
    {
      return(0);
    }
    /* calculate bit offset */
    bofs = ndx*ri->size;
    bofs += shift;
    /* calculate byte offset + shift pair from bit offset. */    
    data+=bofs/8;
    shift=(uint8_t)(bofs%8);

  }

  /* Convert physical value to logical value. */  
  if (ri->resolution != 1)
  {
    value=value/ri->resolution;
  }

  /* Write logical value to report in little endian order. */
  mask=(uint32_t)((1<<ri->size)-1);
  value = (value & mask) << shift;
  
  for(x=0; x < ((ri->size & 0x7) ? (ri->size/8)+1 : (ri->size/8)); x++)
  {
    *(ri->data+x)=(uint8_t)((*(ri->data+x) & ~(mask>>(x*8))) | ((value>>(x*8)) & (mask>>(x*8))));
  }
  return(0);
}

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


/**
  * @}
  */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
