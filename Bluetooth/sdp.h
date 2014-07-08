/*
   Copyright 2012 Guillem Vinals Gangolells <guillem@guillem.co.uk>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef __SDP_H__
#define __SDP_H__

#include "bt_common.h"

#define SDP_DATA_T_NIL 0x00
#define SDP_DATA_T_UINT 0x08
#define SDP_DATA_T_SINT 0x10
#define SDP_DATA_T_UUID 0x18
#define SDP_DATA_T_STR 0x20
#define SDP_DATA_T_BOOL 0x28
#define SDP_DATA_T_DES 0x30
#define SDP_DATA_T_DEA 0x38
#define SDP_DATA_T_URL 0x40

#define SDP_DATA_S_8 0x0
#define SDP_DATA_S_16 0x1
#define SDP_DATA_S_32 0x2
#define SDP_DATA_S_64 0x3
#define SDP_DATA_S_128 0x4
#define SDP_DATA_S_1B 0x5
#define SDP_DATA_S_2B 0x6
#define SDP_DATA_S_4B 0x7

/* PDU identifiers */
#define SDP_ERR_PDU 0x01
#define SDP_SS_PDU 0x02
#define SDP_SSR_PDU 0x03
#define SDP_SA_PDU 0x04
#define SDP_SAR_PDU 0x05
#define SDP_SSA_PDU 0x06
#define SDP_SSAR_PDU 0x07

/* Response lengths and sizes */
#define SDP_HDR_LEN 5
/*
 * ServiceSearchReq minimum Length: 7byte
 * ServiceSearchPattern (with at least one UUID16) = 4byte
 * MaxServiceRecordCount = 2byte
 * ContinuationSate = 1byte
 */
#define SDP_SS_REQ_MIN_LEN 7
/*
 * ServiceSearchRsp minimum Length: 5byte
 * TotalServiceRecordCount = 2byte
 * CurrentServiceRecordCount = 2byte
 * ServiceRecordHandleList = 0byte
 * ContinuationSate = 1byte
 */
#define SDP_SS_RSP_MIN_LEN 5
/*
 * ServiceAttributeReq minimum Length: 11byte
 * ServiceRecordHandle = 4byte
 * MaxAttributeByteCount = 2byte
 * AttributeIDList (with at least one ID) = 4byte
 * ContinuationSate = 1byte
 */
#define SDP_SA_REQ_MIN_LEN 11
/*
 * ServiceSearchAttributeRsp minimum Length: 3byte
 * AttributeListsByteCount = 2byte
 * AttributeList (with 0 attributes) = 0byte
 * ContinuationSate = 1byte
 */
#define SDP_SA_RSP_MIN_LEN 3
/*
 * ServiceSearchAttributeReq minimum Length: 11byte
 * ServiceSearchPattern (with at least one UUID16) = 4byte
 * MaxAttributeByteCount = 2byte
 * AttributeIDList (with at least one ID) = 4byte
 * ContinuationSate = 1byte
 */
#define SDP_SSA_REQ_MIN_LEN 11
/*
 * ServiceAttributeRsp minimum Length: 3byte
 * AttributeListsByteCount = 2byte
 * AttributeList (with 0 attributes) = 0byte
 * ContinuationSate = 1byte
 */
#define SDP_SSA_RSP_MIN_LEN 3

#define SDP_MAX_FRAME_SIZE 128

#define SDP_SERVICE_RFCOMM_ENABLE
#define SDP_SERVICE_COUNT 1

typedef struct _SDP_SERIVCE_ATTRIBUTE
{
    /* Attribute ID */
    UINT16 uID; 
    /* Attribute Value */
    UINT uValueLen;
    BYTE *pValue;
} SDP_SERVICE_ATTRIBUTE;

typedef struct _SDP_SERVICE
{
    CHAR *pcName;
    UINT uNumAttrs;
    SDP_SERVICE_ATTRIBUTE *pAttrs;
} SDP_SERVICE;

typedef struct _SDP_CONTROL_BLOCK
{
    BOOL bInitialised;
    SDP_SERVICE *pService[SDP_SERVICE_COUNT];

    BOOL (*L2CAPsendData)(UINT16, const BYTE*, UINT16);
} SDP_CONTROL_BLOCK;

/*
 * SDP layer private function prototypes
 */

BOOL SDP_API_putPetition(const BYTE *pData, UINT uLen);

BOOL _SDP_handlePetition(BYTE bPDUID, UINT16 uTID, const BYTE *pData, UINT16 uLen);
BOOL _SDP_sendSSResp(UINT16 uTID, const BYTE *pData, UINT16 uLen);
BOOL _SDP_sendSAResp(UINT16 uTID, const BYTE *pData, UINT16 uLen);
BOOL _SDP_sendSSAResp(UINT16 uTID, const BYTE *pData, UINT16 uLen);

UINT16 _SDP_getUUIDs(const BYTE *pServiceSearchPattern, UINT32 *pUUID,
        UINT16 *pReqOffset);
INT16 _SDP_getServiceRecordList(const UINT32 *pUUID, UINT uLen,
        SDP_SERVICE *ppsServiceList[]);
INT16 _SDP_getAttrList(const SDP_SERVICE *pService,
        const BYTE *pAttrIDList,
        BYTE *pAttrList);

#endif /*__SDP_H__*/
