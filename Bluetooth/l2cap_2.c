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
#include "l2cap_2.h"
#include "bt_utils.h"
#include "debug.h"

/*
 * L2CAP definitions
 */

#if DEBUG_L2CAP
#undef DBG_CLASS
#define DBG_CLASS DBG_CLASS_L2CAP
#endif

/*
 * L2CAP variables
 */

static L2CAP_CONTROL_BLOCK *gpsL2CAPCB = NULL;

/*
 * L2CAP public functions implementation
 */

BOOL L2CAP_create()
{
    UINT i;
    HCI_API sHCI;
    L2CAP_API sAPI;
    
    /*Allocate the control block*/
    if (NULL == gpsL2CAPCB)
    {
        gpsL2CAPCB = BT_malloc(sizeof(L2CAP_CONTROL_BLOCK));
		if (NULL == gpsL2CAPCB) {
			DBG_INFO("Can not malloc L2CAP_CONTROL_BLOCK\n");
		}
    }

    /*Initialise the structure*/
    gpsL2CAPCB->isInitialised = TRUE;
    gpsL2CAPCB->bSigID = 0;

    for (i = 0; i < L2CAP_MAX_CHANNELS; ++i)
    {
        gpsL2CAPCB->pasChannel[i] = NULL;
    }

    /* Get the HCI API */
    HCI_getAPI(&sHCI);
    gpsL2CAPCB->HCIsendData = sHCI.sendData;
    sAPI.sendData = &L2CAP_API_sendData;
    sAPI.putData = &L2CAP_API_putData;
    HCI_installL2CAP(&sAPI);

    DBG_INFO("L2CAP Initialised\n");
    return TRUE;
}

BOOL L2CAP_destroy()
{
    INT i = 0;
    if (NULL != gpsL2CAPCB)
    {
        for (i = 0; i < L2CAP_MAX_CHANNELS; ++i)
        {
            if(NULL != gpsL2CAPCB->pasChannel[i])
            {
//                BT_free(gpsL2CAPCB);
                BT_free(gpsL2CAPCB->pasChannel[i]);
            }
        }
        BT_free(gpsL2CAPCB);
        gpsL2CAPCB = NULL;
    }
    return TRUE;
}

BOOL L2CAP_getAPI(L2CAP_API *psAPI)
{
    ASSERT(NULL != psAPI);
    psAPI->putData = &L2CAP_API_putData;
    psAPI->sendData = &L2CAP_API_sendData;
    psAPI->disconnect = &L2CAP_API_disconnect;
    return TRUE;
}

BOOL L2CAP_installRFCOMM(RFCOMM_API *psAPI)
{
    ASSERT(NULL != gpsL2CAPCB);
    ASSERT(NULL != psAPI);
    gpsL2CAPCB->RFCOMMputData = psAPI->putData;
    return TRUE;
}

BOOL L2CAP_installSDP(SDP_API *psAPI)
{
    ASSERT(NULL != gpsL2CAPCB);
    ASSERT(NULL != psAPI);
    gpsL2CAPCB->SDPputData = psAPI->putData;
    return TRUE;
}

/*
 * L2CAP API functions implementation
 */

BOOL L2CAP_API_putData(const BYTE *pData, UINT16 uLen, BOOL bContinuation)
{
    INT i, uDataLen;
    UINT16 uCID, uCtrlDataLen;
    const BYTE *pCmdData;
    UINT8 bCmdCode, bCmdId;

    ASSERT(NULL != gpsL2CAPCB);
    ASSERT(gpsL2CAPCB->isInitialised);

    /*Continuation state not supported*/
    if (bContinuation)
    {
        DBG_ERROR("Continuation state not supported\n");
        return FALSE;
    }

    /*Check the frame data*/
    if (NULL == pData || uLen < L2CAP_HDR_LEN)
    {
        DBG_ERROR("Wrong frame size\n");
        return FALSE;
    }

    /*Get the packet Lenght*/
    uDataLen = BT_readLE16(pData, 0);
    /*Get the CID*/
    uCID = BT_readLE16(pData, 2);

 	DelayMs(10);
	DBG_INFO("L2CAP putData: ");
    DBG_DUMP(pData, uLen);

    /*
     * Handle the frame according to the CID:
     * 0x0001 = Signalling
     * 0x0040 - 0xFFFF = Dynamically allocated
     */
    /*Control frame*/
    if (uCID == L2CAP_SIG_CID)
    {
        /*Check the length*/
        if (uDataLen < L2CAP_SIGHDR_LEN)
        {
            DBG_ERROR("Wrong frame size\n");
            return FALSE;
        }

        /*Handle all the commands contained in the packet*/
        uCtrlDataLen = 0;
        for(i = 0; i < (uDataLen - L2CAP_SIGHDR_LEN) ; i =+ uCtrlDataLen)
        {
            /*
             * Get the command parameters:
             * Code (1 octet): Command code
             * Id (1 octet): Command identifier
             * Length (2 octet): Command data length
             * Data: Command data
             */
            bCmdCode = pData[L2CAP_HDR_LEN];
            bCmdId = pData[L2CAP_HDR_LEN + 1];
            uCtrlDataLen = BT_readLE16(pData, L2CAP_HDR_LEN + 2);
            
            /*Get a pointer to the command data (if any)*/
            if(uCtrlDataLen == 0)
                pCmdData = NULL;
            else
                pCmdData = &pData[L2CAP_HDR_LEN + L2CAP_SIGHDR_LEN + i];

            /*Handle the command*/
            _L2CAP_cmdHandler(bCmdCode, bCmdId, uCtrlDataLen, pCmdData);
        }
    }
    /*Data frame*/
    else if (uCID >= L2CAP_MIN_CID)
    {
        /*Forward the data to the upper layers*/
        _L2CAP_dataHandler(uCID, uDataLen, &pData[4]);
    }
    /*Unexpected CID*/
    else
    {
        DBG_ERROR("Unexpected CID\n");
        return FALSE;
    }
    return TRUE;
}

BOOL L2CAP_API_sendData(UINT16 uPSM, const BYTE *pData, UINT16 uLen)
{
    BYTE *pL2CAPData = NULL;
    UINT16 i, uL2CAPLength;
    L2CAP_CHANNEL *pChannel = NULL;

    ASSERT(NULL != gpsL2CAPCB);
    ASSERT(gpsL2CAPCB->isInitialised);
    
    pChannel = _L2CAP_getChannelByPSM(uPSM);
    if (NULL == pChannel)
    {
        DBG_INFO("sendData Non-existant channel\n");
        return FALSE;
    }
    
    /*Generate an L2CAP data frame*/
    /*Set the length*/
    uL2CAPLength  = uLen + L2CAP_HDR_LEN;
    /*Allocate the memory (mind the L2CAP HDR)*/
    pL2CAPData = (BYTE *) BT_malloc(uL2CAPLength);
    if(NULL == pL2CAPData)
    {
        DBG_ERROR("Not enough memory!\n");
        return FALSE;
    }

    /*Set the L2CAP frame header*/
    /*Length*/
    BT_storeLE16(uLen, pL2CAPData, 0);
    /*Channel*/
    BT_storeLE16(pChannel->uRemoteCID, pL2CAPData, 2);

    /*Do a simple memcopy to get the payload*/
    for (i=0; i<uLen; ++i)
    {
        pL2CAPData[i + L2CAP_HDR_LEN] = pData[i];
    }

    /*Send the local frame*/
    if (!gpsL2CAPCB->HCIsendData(pL2CAPData, uL2CAPLength))
    {
        DBG_ERROR("Unexpected error\n");
        return FALSE;
    }

    /*Free the memory*/
    BT_free(pL2CAPData);

    DBG_INFO("L2CAP Data sent\n");

    return TRUE;
}

BOOL L2CAP_API_disconnect(UINT16 uPSM)
{
    UINT16 uReqLen;
    BYTE *pReqData = NULL;
    L2CAP_CHANNEL *pChannel = NULL;
    UINT uOffset = 0;
    BOOL bRetVal = FALSE;

    ASSERT(NULL != gpsL2CAPCB);
    ASSERT(gpsL2CAPCB->isInitialised);

    pChannel = _L2CAP_getChannelByPSM(uPSM);
    if (NULL == pChannel)
    {
        DBG_INFO("disconnect Non-existant channel\n");
        return FALSE;
    }

    /*Generate a connection response frame*/
    /*Set the length*/
    uReqLen = L2CAP_DISCONN_REQ_SIZE + L2CAP_SIGHDR_LEN + L2CAP_HDR_LEN;
    /*Allocate the memory (mind the L2CAP HDR)*/
    pReqData = BT_malloc(uReqLen);
    if(NULL == pReqData)
    {
        DBG_ERROR("Not enough memory!\n");
        return FALSE;
    }

    /*Set the L2CAP frame header*/
    /*Length*/
    BT_storeLE16(uReqLen - L2CAP_HDR_LEN, pReqData, 0);
    /*Channel*/
    BT_storeLE16(L2CAP_SIG_CID, pReqData, 2);

    /* Set the response command header values */
    uOffset = L2CAP_HDR_LEN;
    /* Command code */
    pReqData[uOffset + 0] = L2CAP_DISCONN_REQ;
    /* Command id (we can randomize it) */
    pReqData[uOffset + 1] = 0x01;
    /* Command length */
    BT_storeLE16(L2CAP_DISCONN_REQ_SIZE, pReqData, uOffset + 2);

    /*
     * Set the response command data values
     * RemoteCID (2 octet)
     * LocalCID (2 octet)
     */
    uOffset = L2CAP_HDR_LEN + L2CAP_SIGHDR_LEN;
    /*Remote CID*/
    BT_storeLE16(pChannel->uRemoteCID, pReqData, uOffset);
    /*Local CID*/
    BT_storeLE16(pChannel->uLocalCID, pReqData, uOffset + 2);

    /*Send the frame to the remote device*/
    bRetVal = gpsL2CAPCB->HCIsendData(pReqData, uReqLen);
    if(bRetVal)
    {
        DBG_INFO("L2CAP Disconn req sent\n")
    }
    else
    {
        DBG_ERROR("Unable to send data\n");
    }

    /*Free the data*/
    BT_free(pReqData);
    return bRetVal;
}
/*
 * L2CAP private functions implementation
 */

BOOL _L2CAP_cmdHandler(UINT8 bCode, UINT8 bId, UINT16 uLen, const BYTE *pData)
{
    UINT16 uLocalCID, uInfoType = 0;
    L2CAP_CHANNEL *pChannel = NULL;

    ASSERT(NULL != gpsL2CAPCB);
    ASSERT(gpsL2CAPCB->isInitialised);

    switch(bCode)
    {
        case L2CAP_INFO_REQ:
            DBG_INFO("L2CAP Info req received\n");

            uInfoType = BT_readLE16(pData,0);
            /*Accept the configuration*/
            if(!_L2CAP_infoResponse(bId, uInfoType))
            {
                DBG_TRACE();
                return FALSE;
            }
            return TRUE;
            break;

        case L2CAP_CONN_REQ:
            DBG_INFO("L2CAP Conn req received\n");

            pChannel = _L2CAP_createChannel(gpsL2CAPCB);

            /*Get the connection request parameters*/
            pChannel->uPSMultiplexor = BT_readLE16(pData, 0);
            pChannel->uRemoteCID = BT_readLE16(pData, 2);
            /*Create a local CID (random)*/
            pChannel->uLocalCID = pChannel->uRemoteCID + 4;

            /*Accept the connection*/
            if (!_L2CAP_acceptConnetion(bId, pChannel))
            {
                DBG_ERROR("Unexpected error\n");
                return FALSE;
            }
            /*Go to the next state*/
            pChannel->uState = L2CAP_STATE_WAIT_CONFIG;
            return TRUE;
            break;

        default:
            DBG_ERROR("l2cap unknown request ");
//			UART1PutDec(bCode);
            DBG_ERROR("\n");
            break;
    }
	DelayMs(10);

    uLocalCID = BT_readLE16(pData, 0);
    pChannel = _L2CAP_getChannelByLCID(uLocalCID);
    if (NULL == pChannel)
    {
        DBG_ERROR("cmdHandler Non-existant channel\n");
        return FALSE;
    }

    /*Switch according to the L2CAP state*/
    switch (pChannel->uState)
    {      
        case L2CAP_STATE_WAIT_CONFIG:
            /*Switch according to the command code*/
            switch (bCode)
            {
                case L2CAP_CFG_REQ:

                    DBG_INFO("L2CAP Conf req received\n");

                    /*Accept the configuration*/
                    if(!_L2CAP_configResponse(bId, L2CAP_MTU, pChannel))
                    {
                        DBG_TRACE();
                        return FALSE;
                    }

                    /*Request configuration*/
                    if(!_L2CAP_configRequest(bId, L2CAP_MTU, pChannel))
                    {
                        DBG_TRACE();
                        return FALSE;
                    }
                        
                    /*Go to the next state*/
                    pChannel->uState = L2CAP_STATE_WAIT_CONFIG_RSP;
                    break;

                default:
                    break;
            }
            break;

        case L2CAP_STATE_WAIT_SEND_CONFIG:
            /*Not implemented*/
            break;

        case L2CAP_STATE_WAIT_CONFIG_REQ_RSP:
            /*Not implemented*/
            break;

        case L2CAP_STATE_WAIT_CONFIG_RSP:
            /*Switch according to the command code*/
            switch (bCode)
            {
                case L2CAP_CFG_RSP:

                    DBG_INFO("L2CAP Conf resp received\n");

                    /*
                     * Check the response frame parameters
                     * Source CID (LocalCID expected)
                     * Flags (0x0000 expected)
                     * Result (0x0000 expected)
                     */
                    if ((BT_readLE16(pData, 0) != pChannel->uLocalCID) ||
                        (BT_readLE16(pData, 2) != 0x0000) ||
                        (BT_readLE16(pData, 4) != L2CAP_CFG_SUCCESS))
                    {
                        DBG_ERROR("Wrong config\n")
                        return FALSE;
                    }

                    /*Raise the linked flag in the L2CAP control block*/
                    pChannel->isLinked = TRUE;
                    /*Go to the next state*/
                    pChannel->uState = L2CAP_STATE_OPEN;
                    break;

                case L2CAP_DISCONN_REQ:

                    DBG_INFO("L2CAP Disconn req received\n");

                    /*Disconnect the channel as requested*/
                    if (!_L2CAP_disconnResponse(bId, pChannel))
                    {
                        DBG_TRACE();
                        return FALSE;
                    }
                    /*Go to the next state*/
                    _L2CAP_destroyChannel(pChannel);
                    pChannel = NULL;
                    break;

                default:
                    break;
            }
            break;

        case L2CAP_STATE_WAIT_CONFIG_REQ:
            /*Not implemented*/
            break;

        case L2CAP_STATE_OPEN:
            /*Switch according to the command code*/
            switch (bCode)
            {
                case L2CAP_DISCONN_REQ:

                    DBG_INFO("L2CAP Disconn req received\n");

                    /*Disconnect the channel as requested*/
                    if (!_L2CAP_disconnResponse(bId, pChannel))
                    {
                        DBG_ERROR( "Unexpected error\n");
                        return FALSE;
                    }
                    /*Go to the next state*/
                    _L2CAP_destroyChannel(pChannel);
                    pChannel = NULL;
                    break;

                default:
                    break;
            }
            break;

        case L2CAP_STATE_WAIT_DISCONNECT:
            /*Not implemented*/
            break;
                
        default:
            /*Undefined state*/
            return FALSE;
    }

    return TRUE;
}

BOOL _L2CAP_dataHandler(UINT16 uCID, UINT16 uLen, const BYTE *pData)
{
    L2CAP_CHANNEL *pChannel = NULL;

    ASSERT(NULL != gpsL2CAPCB);
    ASSERT(gpsL2CAPCB->isInitialised);

    pChannel = _L2CAP_getChannelByLCID(uCID);
    if (NULL == pChannel)
    {
        DBG_INFO("dataHandler Non-existant channel\n");
        return FALSE;
    }

    /*Check L2CAP state*/
    if (pChannel->uState != L2CAP_STATE_OPEN)
    {
        DBG_TRACE();
        return FALSE;
    }

    /*Switch according to the PSM*/
    switch(pChannel->uPSMultiplexor)
    {
        case L2CAP_SDP_PSM:
            /*Put the data into the SDP layer*/
            DBG_INFO("L2CAP SDP data received\n");
            gpsL2CAPCB->SDPputData(pData, uLen);
            break;
        case L2CAP_RFCOMM_PSM:
            /*Put the data into the RFCOMM layer*/
            DBG_INFO("L2CAP RFCOMM data received\n");
            gpsL2CAPCB->RFCOMMputData(pData, uLen);
            break;
    }

    return TRUE;
}

BOOL _L2CAP_acceptConnetion(UINT8 bId, L2CAP_CHANNEL *pChannel)
{
    UINT16 uRspLen;
    BYTE *pRspData = NULL;
    UINT uOffset = 0;

    ASSERT(NULL != gpsL2CAPCB);
    ASSERT(gpsL2CAPCB->isInitialised);
    ASSERT(NULL != pChannel);

    /*Generate a connection response frame*/
    /*Set the length*/
    uRspLen = L2CAP_CONN_RSP_SIZE + L2CAP_SIGHDR_LEN + L2CAP_HDR_LEN;
    /*Allocate the memory (mind the L2CAP HDR)*/
    pRspData = BT_malloc(uRspLen);
    if(NULL == pRspData)
    {
        DBG_ERROR("Not enough memory!\n");
        return FALSE;
    }

    /*Set the L2CAP frame header*/
    /*Length*/
    BT_storeLE16(uRspLen - L2CAP_HDR_LEN, pRspData, 0);
    /*Channel*/
    BT_storeLE16(L2CAP_SIG_CID, pRspData, 2);

    /*Set the response command header values*/
    uOffset = L2CAP_HDR_LEN;
    /*Command code*/
    pRspData[uOffset + 0] = L2CAP_CONN_RSP;
    /*Command id*/
    pRspData[uOffset + 1] = bId;
    /*Command length*/
    BT_storeLE16(L2CAP_CONN_RSP_SIZE, pRspData, uOffset + 2);

    /*
     * Set the response command data values
     * LocalCID (2 octet)
     * RemoteCID (2 octet)
     * Response (2 octet): 0x0000 = Connection successful
     * Status (2 octet): 0x0000 = No further information
     */
    uOffset = L2CAP_HDR_LEN + L2CAP_SIGHDR_LEN;
    BT_storeLE16(pChannel->uLocalCID, pRspData, uOffset);
    BT_storeLE16(pChannel->uRemoteCID, pRspData, uOffset + 2);
    BT_storeLE16(L2CAP_CONN_SUCCESS, pRspData, uOffset + 4);
    BT_storeLE16(0x0000, pRspData, uOffset + 6);

    /*Send the frame to the remote device*/
    if (gpsL2CAPCB->HCIsendData(pRspData, uRspLen))
    {
        /*Free the data*/
        free(pRspData);
        return TRUE;
    }

    return FALSE;
}

BOOL _L2CAP_configResponse(UINT8 bId, UINT uMTU, L2CAP_CHANNEL *pChannel)
{
    UINT16 uRspLen;
    BYTE *pRspData = NULL;
    UINT uOffset = 0;
    BOOL bRetVal = FALSE;

    ASSERT(NULL != gpsL2CAPCB);
    ASSERT(gpsL2CAPCB->isInitialised);
    ASSERT(NULL != pChannel);

    /*Generate a connection response frame*/
    /*Set the length*/
    uRspLen = L2CAP_CFG_RSP_SIZE + L2CAP_SIGHDR_LEN + L2CAP_HDR_LEN;
    /*Allocate the memory (mind the L2CAP HDR)*/
    pRspData = BT_malloc(uRspLen);
    if(NULL == pRspData)
    {
        DBG_ERROR("Not enough memory!\n");
        return FALSE;
    }

    /*Set the L2CAP frame header*/
    /*Length*/
    BT_storeLE16(uRspLen - L2CAP_HDR_LEN, pRspData, 0);
    /*Channel*/
    BT_storeLE16(L2CAP_SIG_CID, pRspData, 2);

    /*Set the response command header values*/
    uOffset = L2CAP_HDR_LEN;
    /*Command code*/
    pRspData[uOffset + 0] = L2CAP_CFG_RSP;
    /*Command id*/
    pRspData[uOffset + 1] = bId;
    /*Command length*/
    BT_storeLE16(L2CAP_CFG_RSP_SIZE, pRspData, uOffset + 2);

    /*
     * Set the response command data values
     * RemoteCID (2 octet)
     * Flags (2 octet): 0x0000 = No continuation
     * Status (2 octet): 0x0000 = Success
     * Configuration options:
     *     1. MTU
     */
    uOffset = L2CAP_HDR_LEN + L2CAP_SIGHDR_LEN;
    /*Remote CID*/
    BT_storeLE16(pChannel->uRemoteCID, pRspData, uOffset);
    /*Flags + Status*/
    BT_storeLE16(0x0000, pRspData, uOffset + 2);
    BT_storeLE16(L2CAP_CFG_SUCCESS, pRspData, uOffset + 4);

    /*Send the frame to the remote device*/
    bRetVal = gpsL2CAPCB->HCIsendData(pRspData, uRspLen);
    if (bRetVal)
    {
        DBG_INFO("L2CAP Conf resp sent\n");
    }
    else
    {
        DBG_ERROR( "Unable to send data\n");
    }

    /*Free the data*/
//    free(pRspData);
	BT_free(pRspData);
    return bRetVal;
}

BOOL _L2CAP_configRequest(UINT8 bId, UINT uMTU, L2CAP_CHANNEL *pChannel)
{
    UINT16 uRspLen;
    BYTE *pRspData = NULL;
    UINT uOffset = 0;
    BOOL bRetVal = FALSE;

    ASSERT(NULL != gpsL2CAPCB);
    ASSERT(gpsL2CAPCB->isInitialised);
    ASSERT(NULL != pChannel);

    /*Generate a connection response frame*/
    /*Set the length*/
    uRspLen = L2CAP_CFG_REQ_SIZE + L2CAP_SIGHDR_LEN + L2CAP_HDR_LEN + 4;
    /*Allocate the memory (mind the L2CAP HDR)*/
    pRspData = BT_malloc(uRspLen);
    if(NULL == pRspData)
    {
        DBG_ERROR("Not enough memory!\n");
        return FALSE;
    }

    /*Set the L2CAP frame header*/
    /*Length*/
    BT_storeLE16(uRspLen - L2CAP_HDR_LEN, pRspData, 0);
    /*Channel*/
    BT_storeLE16(L2CAP_SIG_CID, pRspData, 2);

    /*Set the response command header values*/
    uOffset = L2CAP_HDR_LEN;
    /*Command code*/
    pRspData[uOffset + 0] = L2CAP_CFG_REQ;
    /*Command id*/
    pRspData[uOffset + 1] = bId;
    /*Command length*/
    BT_storeLE16(L2CAP_CFG_REQ_SIZE + 4, pRspData, uOffset + 2);

    /*
     * Set the response command data values
     * RemoteCID (2 octet)
     * Flags (2 octet): 0x0000 = No continuation
     * Configuration options:
     *     1. MTU
     */
    uOffset = L2CAP_HDR_LEN + L2CAP_SIGHDR_LEN;
    /*Remote CID*/
    BT_storeLE16(pChannel->uRemoteCID, pRspData, uOffset);
    /*Flags*/
    BT_storeLE16(0x0000, pRspData, uOffset + 2);
    /*Configuration options*/
    /*Option1: Type (1 = MTU)*/
    pRspData[uOffset + 4] = L2CAP_CFG_MTU;
    /*Option1: Length*/
    pRspData[uOffset + 5] = L2CAP_CFG_MTU_LEN;
    /*Option1: Value*/
    BT_storeLE16(uMTU, pRspData, uOffset + 6);

    /*Send the frame to the remote device*/
    bRetVal = gpsL2CAPCB->HCIsendData(pRspData, uRspLen);
    if(bRetVal)
    {
        DBG_INFO("L2CAP Conf req sent\n")
    }
    else
    {
        DBG_ERROR("Unable to send data\n");
    }

    /*Free the data*/
    BT_free(pRspData);
    return bRetVal;
}

BOOL _L2CAP_disconnResponse(UINT8 bId, L2CAP_CHANNEL *pChannel)
{
    UINT16 uRspLen;
    BYTE *pRspData = NULL;
    UINT uOffset = 0;
    BOOL bRetVal = FALSE;

    ASSERT(NULL != gpsL2CAPCB);
    ASSERT(gpsL2CAPCB->isInitialised);
    ASSERT(NULL != pChannel);

    /*Generate a connection response frame*/
    /*Set the length*/
    uRspLen = L2CAP_DISCONN_RSP_SIZE + L2CAP_SIGHDR_LEN + L2CAP_HDR_LEN;
    /*Allocate the memory (mind the L2CAP HDR)*/
    pRspData = BT_malloc(uRspLen);
    if(NULL == pRspData)
    {
        DBG_ERROR("Not enough memory!\n");
        return FALSE;
    }
    
    /*Set the L2CAP frame header*/
    /*Length*/
    BT_storeLE16(uRspLen - L2CAP_HDR_LEN, pRspData, 0);
    /*Channel*/
    BT_storeLE16(L2CAP_SIG_CID, pRspData, 2);

    /*Set the response command header values*/
    uOffset = L2CAP_HDR_LEN;
    /*Command code*/
    pRspData[uOffset + 0] = L2CAP_DISCONN_RSP;
    /*Command id*/
    pRspData[uOffset + 1] = bId;
    /*Command length*/
    BT_storeLE16(L2CAP_DISCONN_RSP_SIZE, pRspData, uOffset + 2);

    /*
     * Set the response command data values
     * Destination CID (2 octet)
     * Source CID (2 octet)
     */
    uOffset = L2CAP_HDR_LEN + L2CAP_SIGHDR_LEN;
    /*Destination CID*/
    BT_storeLE16(pChannel->uLocalCID, pRspData, uOffset);
    /*Source CID*/
    BT_storeLE16(pChannel->uRemoteCID, pRspData, uOffset + 2);

    /*Send the frame to the remote device*/
    bRetVal = gpsL2CAPCB->HCIsendData(pRspData, uRspLen);
    if(bRetVal)
    {
        DBG_INFO("L2CAP Disconn resp sent\n");
    }
    else
    {
        DBG_ERROR("Unable to send data\n");
    }

    /*Free the data*/
    BT_free(pRspData);
    return bRetVal;
}

BOOL _L2CAP_infoResponse(UINT8 bId, UINT16 uInfoType)
{
    UINT16 uRspLen;
    BYTE *pRspData = NULL;
    UINT uOffset = 0;
    BOOL bRetVal = FALSE;

    ASSERT(NULL != gpsL2CAPCB);
    ASSERT(gpsL2CAPCB->isInitialised);
    /*InfoType must be 0x0002 (Connection based)*/
    ASSERT(uInfoType == L2CAP_INFO_EXTENDED_FEATURES);
    
    /*Generate a information response frame*/
    /*Set the length*/
    uRspLen = L2CAP_INFO_RSP_SIZE + L2CAP_SIGHDR_LEN + L2CAP_HDR_LEN;
    /*Allocate the memory (mind the L2CAP HDR)*/
    pRspData = BT_malloc(uRspLen);
    if(NULL == pRspData)
    {
        DBG_ERROR("Not enough memory!\n");
        return FALSE;
    }
    
    /*Set the L2CAP frame header*/
    /*Length*/
    BT_storeLE16(uRspLen - L2CAP_HDR_LEN, pRspData, 0);
    /*Channel*/
    BT_storeLE16(L2CAP_SIG_CID, pRspData, 2);

    /*Set the response command header values*/
    uOffset = L2CAP_HDR_LEN;
    /*Command code*/
    pRspData[uOffset + 0] = L2CAP_INFO_RSP;
    /*Command id*/
    pRspData[uOffset + 1] = bId;
    /*Command length*/
    BT_storeLE16(L2CAP_INFO_RSP_SIZE, pRspData, uOffset + 2);

    /*
     * Set the response command data values
     * InfoType (2 octet)
     * Result (2 octet)
     */
    uOffset = L2CAP_HDR_LEN + L2CAP_SIGHDR_LEN;
    /*InfoType*/
    BT_storeLE16(uInfoType, pRspData, uOffset);
    /*Result (Not supported)*/
    BT_storeLE16(L2CAP_INFO_NOT_SUPPORTED, pRspData, uOffset + 2);

    /*Send the frame to the remote device*/
    bRetVal = gpsL2CAPCB->HCIsendData(pRspData, uRspLen);
    if(bRetVal)
    {
        DBG_INFO("L2CAP Info resp sent\n");
    }
    else
    {
        DBG_ERROR("Unable to send data\n");
    }

    /*Free the data*/
    BT_free(pRspData);
    return bRetVal;    
}

L2CAP_CHANNEL* _L2CAP_getChannelByLCID(UINT16 uLocalCID)
{
    UINT i = 0;

    ASSERT(NULL != gpsL2CAPCB);
    
    for (i = 0; i < L2CAP_MAX_CHANNELS; ++i)
    {
        if(gpsL2CAPCB->pasChannel[i] != NULL &&
                gpsL2CAPCB->pasChannel[i]->uState != L2CAP_STATE_CLOSED &&
                gpsL2CAPCB->pasChannel[i]->uLocalCID == uLocalCID)
        {
            return (L2CAP_CHANNEL*) gpsL2CAPCB->pasChannel[i];
        }
        DBG_INFO("_L2CAP_getChannelByLCID\n");
    }
    return NULL;
}

L2CAP_CHANNEL* _L2CAP_getChannelByPSM(UINT16 uPSM)
{
    UINT i;

    ASSERT(NULL != gpsL2CAPCB);

    for (i = 0; i < L2CAP_MAX_CHANNELS; ++i)
    {
        if(gpsL2CAPCB->pasChannel[i] != NULL &&
                gpsL2CAPCB->pasChannel[i]->uState != L2CAP_STATE_CLOSED &&
                gpsL2CAPCB->pasChannel[i]->uPSMultiplexor == uPSM)
        {
            return (L2CAP_CHANNEL*) gpsL2CAPCB->pasChannel[i];
        }
    }
    return NULL;
}

L2CAP_CHANNEL* _L2CAP_createChannel()
{
    UINT i = 0;
    L2CAP_CHANNEL* psRetChannel = NULL;

    ASSERT(NULL != gpsL2CAPCB);

	DBG_INFO("_L2CAP_createChannel\n");

    for (i = 0; i < L2CAP_MAX_CHANNELS; ++i)
    {
        psRetChannel = (gpsL2CAPCB->pasChannel)[i];
        if(NULL == psRetChannel)
        {
            psRetChannel = (L2CAP_CHANNEL*) BT_malloc(sizeof(L2CAP_CHANNEL));
            psRetChannel->uIndex = i;
            psRetChannel->isLinked = FALSE;
            psRetChannel->uLocalCID = 0x00;
            psRetChannel->uRemoteCID = 0x00;
            psRetChannel->uPSMultiplexor = 0x00;
            psRetChannel->uState = L2CAP_STATE_CLOSED;
            
            (gpsL2CAPCB->pasChannel)[i] = psRetChannel;
            return psRetChannel;
        }
    }
    return NULL;
}

BOOL _L2CAP_destroyChannel(L2CAP_CHANNEL* pChannel)
{
    ASSERT(NULL != gpsL2CAPCB);

    if (NULL == pChannel ||
            NULL == gpsL2CAPCB->pasChannel[pChannel->uIndex])
    {
        return FALSE;
    }

    ASSERT(pChannel == gpsL2CAPCB->pasChannel[pChannel->uIndex]);

    gpsL2CAPCB->pasChannel[pChannel->uIndex] = NULL;
    BT_free(pChannel);
    
    return TRUE;
}
