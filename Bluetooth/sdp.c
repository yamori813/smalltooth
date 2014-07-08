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

#include "GenericTypeDefs.h"
#include "bt_utils.h"
#include "debug.h"
#include "sdp.h"

#if DEBUG_SDP
#undef DBG_CLASS
#define DBG_CLASS DBG_CLASS_SDP
#endif

#ifdef SDP_SERVICE_RFCOMM_ENABLE
const BYTE aRFCOMMAttr0_Val[] = {
    /*UUID 32 bit*/
    (SDP_DATA_T_UINT|SDP_DATA_S_32),
    0x00, 0x01, 0x00, 0x01
};
const BYTE aRFCOMMAttr1_Val[] = {
    /*3 byte element data sequence*/
    SDP_DATA_T_DES|SDP_DATA_S_1B, 0x11,
    /*UUID 16 bits*/
    SDP_DATA_T_UUID|SDP_DATA_S_128,
    0x00, 0x00, 0x11, 0x01,
    0x00, 0x00, 0x10, 0x00,
    0x80, 0x00, 0x00, 0x80,
    0x5F, 0x9B, 0x34, 0xFB
};
const BYTE aRFCOMMAttr2_Val[] = {
    /*12 byte element data sequence*/
    SDP_DATA_T_DES|SDP_DATA_S_1B, 0x0C,
    /*
     * L2CAP protocol descriptor:
     * UUID = 0x0100
     */
    SDP_DATA_T_DES|SDP_DATA_S_1B, 0x03,
    SDP_DATA_T_UUID|SDP_DATA_S_16,
    0x01, 0x00,
    /*
     * RFCOMM protocol descriptor:
     * UUID = 0x0003
     * CHANNEL = RFCOMM_CH_NUM
     */
    SDP_DATA_T_DES|SDP_DATA_S_1B, 0x05,
    SDP_DATA_T_UUID|SDP_DATA_S_16,
    0x00, 0x03,
    SDP_DATA_T_UINT|SDP_DATA_S_8,
    RFCOMM_CH_DATA
};
const BYTE aRFCOMMAttr3_Val[] = {
    /*3 byte element data sequence*/
    SDP_DATA_T_DES|SDP_DATA_S_1B, 0x03,
    /*UUID 16 bits (Public Browse Group)*/
    SDP_DATA_T_UUID|SDP_DATA_S_16,
    0x10, 0x02
};
const BYTE aRFCOMMAttr4_Val[] = {
    /* 9 byte element data sequence */
    SDP_DATA_T_DES|SDP_DATA_S_1B, 0x09,
    /*
     * Natural language ID.
     */
    SDP_DATA_T_UINT|SDP_DATA_S_16,
    'e', 'n',
    /*
     * Language encoding:
     * UINT16 = 0x6A (UTF-8)
     */
    SDP_DATA_T_UINT|SDP_DATA_S_16,
    0x00, 0x6A,
    /*
     * Base attribute ID for the language in the Service Record:
     * UINT16 = 0x0100
     */
    SDP_DATA_T_UINT|SDP_DATA_S_16,
    0x01, 0x00
};
const BYTE aRFCOMMAttr5_Val[] = {
    /* 11 byte data element sequence */
    SDP_DATA_T_STR|SDP_DATA_S_1B, 0x05,
    /* Service record name string */
    'C', 'O', 'M', '1', 0x00
};

const SDP_SERVICE_ATTRIBUTE aRFCOMMAttrs[] = {
    /*
     * RFCOMM Attribute 0: Handler
     * ID 0x0000
     * Value:
     *     Service handler (UUID32bit)
     */
    { .uID = 0x0000, .uValueLen = 5, .pValue = (BYTE *)aRFCOMMAttr0_Val },
    /*
     * RFCOMM Attribute 1: Service Class ID List
     * ID 0x0001
     * Value:
     *     Data element sequence of service classes (UUID16bit) that
     *     the service record conforms to.
     */
    { .uID = 0x0001, .uValueLen = 19, .pValue = (BYTE *)aRFCOMMAttr1_Val },
    /*
     * RFCOMM Attribute 2: Protocol Descriptor List
     * ID 0x0004
     * Value:
     *     Data element sequence of protocol stacks that can be used to
     *     access the service described by the record.
     */
    { .uID = 0x0004, .uValueLen = 14, .pValue = (BYTE *)aRFCOMMAttr2_Val },
    /*
     * RFCOMM Attribute 3: Service Class ID List
     * ID 0x0005
     * Value:
     *     Data element sequence of browse groups (UUID16bit) the
     *     service record belongs to.
     */
    { .uID = 0x0005, .uValueLen = 5, .pValue = (BYTE *)aRFCOMMAttr3_Val },
    /*
     * RFCOMM Attribute 4: Language Base ID Attribute List
     * ID 0x0005
     * Value:
     *     A list of language bases. It contains a language identifier,
     *     a character encoding indentifier and a base attribute ID for
     *     the languages used in the service record.
     */
    { .uID = 0x0006, .uValueLen = 11, .pValue = (BYTE *)aRFCOMMAttr4_Val },
    /*
     * RFCOMM Attribute 5: Service Name
     * ID 0x0000 + BaseAttributeID offset
     * Value:
     *     String containing the name of the service specified in the
     *     service record.
     */
    { .uID = 0x0100, .uValueLen = 7, .pValue = (BYTE *)aRFCOMMAttr5_Val }
};

#endif /*SDP_SERVICE_RFCOMM_ENABLE*/

#ifdef SDP_SERVICE_RFCOMM_ENABLE
const SDP_SERVICE sServiceRFCOMM = {
    .pcName = "RFCOMM",
    .uNumAttrs = 6,
    .pAttrs = (SDP_SERVICE_ATTRIBUTE *)aRFCOMMAttrs
};
#endif /*SDP_SERVICE_RFCOMM_ENABLE*/

static SDP_CONTROL_BLOCK *gpsSDPCB = NULL;

/*
 * SDP public functions implementation
 */

BOOL SDP_create()
{
    UINT i;
    L2CAP_API sL2CAP;
    SDP_API sAPI;

    /* Allocate the control block */
    if (NULL == gpsSDPCB)
    {
        gpsSDPCB = BT_malloc(sizeof(SDP_CONTROL_BLOCK));
		if (NULL == gpsSDPCB) {
			DBG_INFO("Can not malloc SDP_CONTROL_BLOCK\n");
		}
    }

    /* Initialise the control block structure */
    gpsSDPCB->bInitialised = TRUE;
    for (i = 0; i < SDP_SERVICE_COUNT; ++i)
    {
        gpsSDPCB->pService[i] = NULL;
    }
    #ifdef SDP_SERVICE_RFCOMM_ENABLE
        gpsSDPCB->pService[0] = (SDP_SERVICE *)&sServiceRFCOMM;
    #else
        #error
    #endif /*SDP_SERVICE_RFCOMM_ENABLE*/

    L2CAP_getAPI(&sL2CAP);
    gpsSDPCB->L2CAPsendData = sL2CAP.sendData;

    sAPI.putData = &SDP_API_putPetition;
    L2CAP_installSDP(&sAPI);

    DBG_INFO("SDP Initialised\n");
    return TRUE;
}

BOOL SDP_destroy()
{
    if (NULL != gpsSDPCB)
    {
        BT_free(gpsSDPCB);
    }
    return TRUE;
}

/*
 * SDP API functions implementation
 */

BOOL SDP_API_putPetition(const BYTE *pData, UINT uLen)
{
    BYTE bPDUID;
    UINT16 uTID, uParameterLen;
    BOOL bRetVal = FALSE;

    /* SDP CB memory not allocated */
    ASSERT(NULL != gpsSDPCB);

    /* SDP not initialised */
    if (!gpsSDPCB->bInitialised)
    {
        DBG_ERROR("SDP layer not initialised\n");
        return FALSE;
    }

    /* Check the PDU data length */
    if (NULL == pData || uLen < SDP_HDR_LEN)
    {
        DBG_ERROR( "Wrong PDU size\n");
        return FALSE;
    }

    /* Read the PDU header */
    bPDUID = pData[0];
    uTID = BT_readBE16(pData, 1);
    uParameterLen = BT_readBE16(pData, 3);

    /* Handle the petition */
    bRetVal = _SDP_handlePetition(bPDUID, uTID, &pData[SDP_HDR_LEN],
            uParameterLen);

    return bRetVal;
}

/*
 * SDP private functions implementation
 */

BOOL _SDP_handlePetition(BYTE bPDUID, UINT16 uTID, const BYTE *pData,
        UINT16 uLen)
{
    BOOL bRetVal = FALSE;
    switch(bPDUID)
    {
        case SDP_ERR_PDU:
            DBG_INFO( "SDP handle ERROR PDU\n\r");
            bRetVal = FALSE;
            break;

        case SDP_SS_PDU:

            /* Input data length check. */
            if ((NULL == pData) || (uLen < SDP_SS_REQ_MIN_LEN))
            {
                DBG_ERROR( "SDP: NULL or wrong request frame.\n\r");
            }

            DBG_INFO( "SDP: Handle SSR. uTID = %02X\n\r", uTID);
            DBG_DUMP(pData, uLen);

            bRetVal = _SDP_sendSSResp(uTID, pData, uLen);
            break;

        case SDP_SA_PDU:

            /* Input data length check. */
            if ((NULL == pData) || (uLen < SDP_SA_REQ_MIN_LEN))
            {
                DBG_ERROR( "SDP: NULL or wrong request frame.\n\r");
            }

            DBG_INFO( "SDP: Handle SAR. uTID = %02X\n\r", uTID);
            DBG_DUMP(pData, uLen);

            bRetVal = _SDP_sendSAResp(uTID, pData, uLen);
            break;

        case SDP_SSA_PDU:

            /* Input data length check. */
            if ((NULL == pData) || (uLen < SDP_SSA_REQ_MIN_LEN))
            {
                DBG_ERROR( "SDP: NULL or wrong request frame.\n\r");
            }

            DBG_INFO( "SDP: Handle SSAR. uTID = %02X\n\r", uTID);
            DBG_DUMP(pData,uLen);

            bRetVal = _SDP_sendSSAResp(uTID, pData, uLen);
            break;

        default:
            DBG_ERROR( "SDP: Incorrect PDU ID.\n\r");
            bRetVal = FALSE;
            break;
    }

   DelayMs(10);
   return bRetVal;
}

BOOL _SDP_sendSSResp(UINT16 uTID, const BYTE *pData, UINT16 uLen)
{
    BYTE pRspData[SDP_MAX_FRAME_SIZE];
    BOOL bRetVal = FALSE;
    UINT32 au32UUID[12], uServiceRecordHandler;
    UINT uNumUUID, uRspDataLen, i;
    UINT16 uTServiceRecordCount, uCServiceRecordCount;
    SDP_SERVICE *apsServiceList[SDP_SERVICE_COUNT];

    uTServiceRecordCount = uCServiceRecordCount = 0;

    /*
     * Prepare the ServiceRecordHandleList:
     * Get the UUIDs from the ServiceSearchPattern and match them with
     * the services in the database.
     */

    /* Get the UUIDs from the ServiceSearchPattern. */
    uNumUUID = _SDP_getUUIDs(pData, au32UUID, NULL);
    /*
     * Get the ServiceRecordCount and store the ServiceRecordHandleList
     * in the Response data frame.
     */
    uCServiceRecordCount =
            _SDP_getServiceRecordList(au32UUID, uNumUUID, apsServiceList);
    uTServiceRecordCount = uCServiceRecordCount;
    uRspDataLen = SDP_SS_RSP_MIN_LEN + 4*uCServiceRecordCount;

    /*
     * Fill the Response frame.
     */

    /* Header: PDU ID, Transaction ID and PDU data length. */
    pRspData[0] = SDP_SSR_PDU;
    BT_storeBE16(uTID, pRspData, 1);
    BT_storeBE16(uRspDataLen, pRspData, 3);
    /* TotalServiceRecordCount and CurrentServiceRecordCount. */
    BT_storeBE16(uTServiceRecordCount, pRspData, 5);
    BT_storeBE16(uCServiceRecordCount, pRspData, 7);
    /* ServiceRecordHandleList: Already stored. */
    for (i = 0; i < uCServiceRecordCount; ++i)
    {
        if (NULL != apsServiceList[i])
        {
            uServiceRecordHandler = BT_readBE32(
                apsServiceList[i]->pAttrs[0].pValue, 1);
            BT_storeBE32(uServiceRecordHandler, pRspData, 9 + 4*i);
        }
    }
    /* ContinuationState (the last byte) */
    pRspData[SDP_HDR_LEN + uRspDataLen - 1] = 0x00;

    /*
     * Send the Response frame.
     */

    DBG_INFO( "SDP: Sending SSResponse.\n\r");

    bRetVal = gpsSDPCB->L2CAPsendData(L2CAP_SDP_PSM,pRspData, SDP_HDR_LEN + uRspDataLen);
    return bRetVal;
}

BOOL _SDP_sendSSAResp(UINT16 uTID, const BYTE *pData, UINT16 uLen)
{
    BYTE pRspData[SDP_MAX_FRAME_SIZE];
    BOOL bRetVal;
    UINT16 uRspDataLen, uAttrByteCount, uAttrListsByteCount, uNumUUID,
            uNumServices, i, uReqOffset, uRspOffset;
    UINT32 au32UUID[12];
    SDP_SERVICE *apsServiceList[SDP_SERVICE_COUNT];

    uAttrListsByteCount = uAttrByteCount = uNumUUID = 0;
    uReqOffset = uRspOffset = 0;
    bRetVal = FALSE;

    /*
     * Prepare the AttributeLists:
     */

    /*
     * Attribute list sequence:
     * Empty sequence
     */
    uAttrListsByteCount = 0x0002;
    pRspData[7] = SDP_DATA_T_DES|SDP_DATA_S_1B;
    pRspData[8] = 0x00;
    /*
     * Get the UUIDs from the ServiceSearchPattern and match them with
     * the services in the database.
     */
    uNumUUID = _SDP_getUUIDs(pData, au32UUID, &uReqOffset);
    uNumServices = _SDP_getServiceRecordList(au32UUID, uNumUUID, apsServiceList);
    /*
     * Fill the Attribute Lists with the attributes of each service (each
     * service is represented with a data element sequence containing its
     * attributes)
     */
    /*Set the offset to the Attribute ID List contained in the request*/
    uReqOffset += 2;

    for (i = 0; i < uNumServices; ++i)
    {
        if (NULL != apsServiceList[i])
        {
            /* Get the IDs and fill the AttributeList. */
            uAttrByteCount = _SDP_getAttrList(apsServiceList[i],
                    &pData[uReqOffset], &pRspData[9]);
        }
        uAttrListsByteCount += uAttrByteCount;
    }
    uRspDataLen = SDP_SSA_RSP_MIN_LEN + uAttrListsByteCount;
    pRspData[8] = uAttrListsByteCount - 2;
    /*
     * Fill the Response frame.
     */

    /* Header: PDU ID, Transaction ID and PDU data length. */
    pRspData[0] = SDP_SSAR_PDU;
    BT_storeBE16(uTID, pRspData, 1);
    BT_storeBE16(uRspDataLen, pRspData, 3);

    /* AttributeListsByteCount and AttributeList (already stored) */
    BT_storeBE16(uAttrListsByteCount, pRspData, 5);
    /* ContinuationSate. */
    pRspData[SDP_HDR_LEN + uRspDataLen - 1] = 0x00;

    /*
     * Send the Response frame.
     */

    DBG_INFO( "SDP: Sending SSAResponse.\n\r");
    bRetVal = gpsSDPCB->L2CAPsendData(L2CAP_SDP_PSM,pRspData, SDP_HDR_LEN + uRspDataLen);
    return bRetVal;
}

BOOL _SDP_sendSAResp(UINT16 uTID, const BYTE *pData, UINT16 uLen)
{
    BYTE pRspData[SDP_MAX_FRAME_SIZE];
    BOOL bFound, bRetVal = FALSE;
    UINT uRspDataLen, uAttrListByteCount, i;
    UINT32 uReqSrvHandle, uLocalSrvHandle;
    SDP_SERVICE *pService = NULL;

    /*
     * Prepare the AttributeList:
     * Get the IDs and ID ranges from the AttributeIDList and store the
     * requested attributes into the Response Data frame.
     */

    bFound = FALSE;
    for (i = 0; (i < SDP_SERVICE_COUNT) && !bFound; ++i)
    {
        uReqSrvHandle = BT_readBE32(pData, 0);
        uLocalSrvHandle = BT_readBE32(gpsSDPCB->pService[i]->pAttrs[0].pValue, 1);

        if (uReqSrvHandle == uLocalSrvHandle)
        {
            bFound = TRUE;
            pService = (SDP_SERVICE *) gpsSDPCB->pService[i];
        }
    }

    /* Get the IDs from the AttributeIDList and create the AttributeList. */
    uAttrListByteCount =  _SDP_getAttrList(pService, &pData[6], &pRspData[7]);
    uRspDataLen = SDP_SA_RSP_MIN_LEN + uAttrListByteCount;

    /*
     * Fill the Response frame.
     */

    /* Header: PDU ID, Transaction ID and PDU data length. */
    pRspData[0] = SDP_SAR_PDU;
    BT_storeBE16(uTID, pRspData, 1);
    BT_storeBE16(uRspDataLen,pRspData,3);
    /* AttributeListByteCount and AttributeList (already stored) */
    BT_storeBE16(uAttrListByteCount, pRspData, 5);
    /* ContinuationSate. */
    pRspData[SDP_HDR_LEN + uRspDataLen - 1] = 0x00;

    /*
     * Send the Response frame.
     */

    DBG_INFO( "SDP: Sending SAResponse.\n\r");
    bRetVal = gpsSDPCB->L2CAPsendData(L2CAP_SDP_PSM,pRspData,
            SDP_HDR_LEN + uRspDataLen);
    return bRetVal;
}

UINT16 _SDP_getUUIDs(const BYTE *pServiceSearchPattern,
        UINT32 *pUUID, UINT16 *pReqOffset)
{
    BYTE bDataDescriptor;
    UINT uNumUUID, i, uDataLen, uOffset;

    uNumUUID = uDataLen = i = uOffset = 0;

    /*
     * NOTICE: It is a assumed that pUUID is at least a 12 element array.
     */

    if (NULL == pServiceSearchPattern)
    {
        DBG_WARN("SDP: Null service search pattern.\n\r");
        return 0;
    }

    /* Get the pattern length and the offset */
    switch (pServiceSearchPattern[0])
    {
        case SDP_DATA_T_DES|SDP_DATA_S_1B:
            uDataLen = pServiceSearchPattern[1];
            uOffset = 2;
            break;
        case SDP_DATA_T_DES|SDP_DATA_S_2B:
            uDataLen = BT_readBE16(pServiceSearchPattern, 1);
            uOffset = 3;
            break;
        default:
            DBG_ERROR("SDP: Wrong service search pattern.\n\r");
            return 0;
            break;
    }

    while (i < uDataLen)
    {
        bDataDescriptor = pServiceSearchPattern[uOffset + i];
        switch(bDataDescriptor)
        {
            /* 16 bit UUID */
            case SDP_DATA_T_UUID|SDP_DATA_S_16:
                    pUUID[uNumUUID] = BT_readBE16(pServiceSearchPattern,
                            uOffset + i + 1);
                    i+=3;
                    ++uNumUUID;
                    break;
            /* 32 bit UUID */
            case SDP_DATA_T_UUID|SDP_DATA_S_32:
                    pUUID[uNumUUID] = BT_readBE32(pServiceSearchPattern,
                            uOffset + i + 1);
                    i+=5;
                    ++uNumUUID;
                    break;

            /* 128 bit UUID */
            case  SDP_DATA_T_UUID|SDP_DATA_S_128:
                    /* Not supported */
                    return uNumUUID;
                    break;

            default:
                    return uNumUUID;
                    break;
        }
    }
    
    /* Return the Offset to to next parameter in the request */
    if(NULL != pReqOffset)
    {
        *pReqOffset = uOffset + uDataLen;
    }

    return uNumUUID;
}

INT16 _SDP_getServiceRecordList(const UINT32 *pUUID, UINT uLen,
        SDP_SERVICE *ppsServiceList[])
{
    UINT i;
    UINT uNumServices = 0;
    BOOL bDiscarded = FALSE;

    /* Do a sanity check on the input variables */
    if (NULL == pUUID    ||
        NULL == ppsServiceList)
    {
        DBG_ERROR("SDP: Wrong input parameters.\n\r");
        return FALSE;
    }

    #ifdef SDP_SERVICE_RFCOMM_ENABLE
    ppsServiceList[0] = NULL;
    bDiscarded = FALSE;
    /* Discard the services according to the UUID list */
    for(i = 0; (i < uLen) && !bDiscarded; ++i)
    {
        if (pUUID[i] != 0x00000003 &&
            pUUID[i] != 0x00000100 &&
            pUUID[i] != 0x00001101 &&
            pUUID[i] != 0x00001002 )
        {
            bDiscarded = TRUE;
        }
    }
    if ((uLen > 0) && !bDiscarded)
    {
        /* Populate the ServiceList with the RFCOMM */
        ppsServiceList[0] = (SDP_SERVICE *) gpsSDPCB->pService[0];
        ++uNumServices;
    }
    #endif

    return uNumServices;
}

INT16 _SDP_getAttrList(const SDP_SERVICE *pService, const BYTE *pAttrIDList,
        BYTE *pAttrList)
{
    UINT i, j, k, uAttrListByteCount, uInOffset, uOutOffset, uLen;
    UINT16 uID, uIDRangeHigh, uIDRangeLow;
    BYTE bDataDescriptor;
    BOOL bFound = FALSE;

    /* Do a sanity check on the input variables */
    if (NULL == pAttrIDList ||
        NULL == pAttrList)
    {
        DBG_ERROR( "SDP: Wrong input parameters.\n\r");
        return FALSE;
    }


    /* The first 2 bytes will contain a data element sequence descriptor */
    uOutOffset = 2;
    uAttrListByteCount = 0;

    /* In case of a NULL service, return an empty list */
    if (NULL == pService)
    {
        /* Fill the first 2 bytes (data element sequence descriptor) */
        pAttrList[0] =  SDP_DATA_T_DES|SDP_DATA_S_1B;
        pAttrList[1] =  uAttrListByteCount;

        return uAttrListByteCount + uOutOffset;
    }

    /* Get the list length and the offset. */
    bDataDescriptor = pAttrIDList[0];
    switch (bDataDescriptor)
    {
        case SDP_DATA_T_DES|SDP_DATA_S_1B:
            uLen = pAttrIDList[1];
            uInOffset = 2;
            break;
        case SDP_DATA_T_DES|SDP_DATA_S_2B:
            uLen = BT_readBE16(pAttrIDList, 1);
            uInOffset = 3;
            break;
        default:
            break;
    }
    i = 0;
    while(i < uLen)
    {
        /*
         * Depending on the descriptor the data will be stored as a single ID
         * or as an ID range.
         */
        bDataDescriptor = pAttrIDList[uInOffset + i];
        switch(bDataDescriptor)
        {
            /* 16 bit ID (single ID) */
            case SDP_DATA_T_UINT|SDP_DATA_S_16:
                uID = BT_readBE16(pAttrIDList, uInOffset + j + 1);
                bFound = FALSE;
                /* Search for that AttrID in the service record */
                for (j = 0; (j < pService->uNumAttrs) && !bFound; ++j)
                {
                    if (pService->pAttrs[j].uID == uID)
                    {
                        /*
                         * Fill the AttrList
                         */

                        /* Store the attribute ID (as an UINT16) */
                        pAttrList[uOutOffset + uAttrListByteCount] =
                                SDP_DATA_T_UINT|SDP_DATA_S_16;
                        BT_storeBE16(pService->pAttrs[j].uID,
                                pAttrList, uOutOffset + uAttrListByteCount + 1);
                        uAttrListByteCount += 3;
                        /* Store the attribute value */
                        for(k = 0; k < pService->pAttrs[j].uValueLen; ++k)
                        {
                            pAttrList[uOutOffset + uAttrListByteCount + k] =
                                    pService->pAttrs[j].pValue[k];
                        }
                        uAttrListByteCount += k;
                        bFound = TRUE;
                    }
                }
                i+=3;
                break;
            /* 32 bit ID (range of IDs) */
            case SDP_DATA_T_UINT|SDP_DATA_S_32:
                uIDRangeLow = BT_readBE16(pAttrIDList, uInOffset + i + 1);
                uIDRangeHigh = BT_readBE16(pAttrIDList, uInOffset + i + 1 + 2);
                /* Search for the AttrID in the service record */
                for (j = 0; j < pService->uNumAttrs; ++j)
                {
                    uID = pService->pAttrs[j].uID;
                    if ((uID >= uIDRangeLow) &&
                        (uID <= uIDRangeHigh))
                    {
                        /*
                         * Fill the AttrList
                         */

                        /* Store the attribute ID (as an UINT16) */
                        pAttrList[uOutOffset + uAttrListByteCount] =
                                SDP_DATA_T_UINT|SDP_DATA_S_16;
                        BT_storeBE16(pService->pAttrs[j].uID,
                                pAttrList, uOutOffset + uAttrListByteCount + 1);
                        uAttrListByteCount += 3;
                        /* Store the attribute value */
                        for(k = 0; k < pService->pAttrs[j].uValueLen; ++k)
                        {
                            pAttrList[uOutOffset + uAttrListByteCount + k] =
                                    pService->pAttrs[j].pValue[k];
                        }
                        uAttrListByteCount += k;
                    }
                }
                i+=5;
                break;
            /* Error, the element is not an ID */
            default:
                return FALSE;
                break;
        }
    }

    /* Fill the first 2 bytes (data element sequence descriptor) */
    pAttrList[0] =  SDP_DATA_T_DES|SDP_DATA_S_1B;
    pAttrList[1] =  uAttrListByteCount;

    return uAttrListByteCount + uOutOffset;
}
