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
#include "rfcomm.h"
#include "debug.h"
#include "rfcomm_fcs.h"
#include "bt_utils.h"

/* Definition in order to print user data */
#include "HardwareProfile.h"

/*
 * RFCOMM definitions
 */

#if DEBUG_RFCOMM
#undef DBG_CLASS
#define DBG_CLASS DBG_CLASS_RFCOMM
#endif


/*
 * RFCOMM variables
 */

static RFCOMM_CONTROL_BLOCK *gpsRFCOMMCB = NULL;

/*
 * RFCOMM public functions implementation
 */

BOOL RFCOMM_create()
{
    UINT i;
    L2CAP_API sL2CAP;
    RFCOMM_API sAPI;

    if (NULL == gpsRFCOMMCB)
    {
        gpsRFCOMMCB = BT_malloc(sizeof(RFCOMM_CONTROL_BLOCK));
    }

    /* Initialise the channels */
    for (i = 0; i < RFCOMM_NUM_CHANNELS; ++i)
    {
        gpsRFCOMMCB->asChannel[i].bDLC = i;
        gpsRFCOMMCB->asChannel[i].bEstablished = FALSE;
        gpsRFCOMMCB->asChannel[i].bLocalCr = 0;
        gpsRFCOMMCB->asChannel[i].bRemoteCr = 0;
    }

    /* Set the role as responder */
    gpsRFCOMMCB->bRole = RFCOMM_ROLE_RESPONDER;

    gpsRFCOMMCB->isInitialised = TRUE;

    L2CAP_getAPI(&sL2CAP);
    gpsRFCOMMCB->L2CAPsendData = sL2CAP.sendData;
    
    sAPI.putData = &RFCOMM_API_putData;
    sAPI.sendData = &RFCOMM_API_sendData;
    sAPI.disconnect = &RFCOMM_API_disconnect;
    L2CAP_installRFCOMM(&sAPI);
    
    return TRUE;
}

BOOL RFCOMM_destroy()
{
    if(NULL != gpsRFCOMMCB)
    {
        BT_free(gpsRFCOMMCB);
    }
    return TRUE;
}

BOOL RFCOMM_getAPI(RFCOMM_API *psAPI)
{
    ASSERT(NULL != psAPI);
    psAPI->putData = &RFCOMM_API_putData;
    psAPI->sendData = &RFCOMM_API_sendData;
    psAPI->disconnect = &RFCOMM_API_disconnect;
    return TRUE;
}

BOOL RFCOMM_installDevCB(DEVICE_API *psAPI)
{
    ASSERT(NULL != psAPI);
    ASSERT(NULL != gpsRFCOMMCB);
    gpsRFCOMMCB->putRFCOMMData = psAPI->putRFCOMMData;
    return TRUE;
}

/*
 * RFCOMM API functions implementation
 */

BOOL RFCOMM_API_putData(const BYTE *pData, UINT uLen)
{
    BYTE bCtrl, bFCS, bMsgType, bMsgLen;
    UINT8 uOffset, uMsgOffset, bChNumber;
    UINT16 uFrameHdrLen, uFrameInfLen;
    BOOL bRetVal, bHasCrField = FALSE;
    RFCOMM_CHANNEL *psChannel = NULL;

	DelayMs(10);
    DBG_INFO ("RFCOMM Data received: \r\n");
    DBG_DUMP(pData, uLen);

    /*
     * Parse the header
     */
    ASSERT(uLen >= RFCOMM_HDR_LEN_1B);

    /* Get the Channel number and Control field */
    bChNumber = pData[0] >> 3;
    bCtrl = pData[1];
    /* Check the channel */
    psChannel = _RFCOMM_getChannel(bChNumber);
    if(NULL == psChannel)
    {
        return FALSE;
    }

    /* Check if the len field has 1 or 2 octets */
    if ((pData[2] & RFCOMM_MASK_LI_1B))
    {
        uFrameHdrLen = RFCOMM_HDR_LEN_1B;
        uFrameInfLen = pData[2] >> 1;
    }
    else
    {
        uFrameHdrLen = RFCOMM_HDR_LEN_2B;
        uFrameInfLen = BT_readBE16(pData, 2) >> 1;
    }
    uOffset = uFrameHdrLen;

    /*
     * Parse the payload according to the frame type and the information
     */
    ASSERT(uLen >= (uFrameHdrLen + uFrameInfLen + 1));

    /* Information frame */
    if (bCtrl == RFCOMM_UIH_FRAME ||
        bCtrl == (RFCOMM_UIH_FRAME|RFCOMM_PF_BIT))
    {
        /* Handle the credit field (if any) */
        if (bCtrl & RFCOMM_PF_BIT)
        {
            bHasCrField = TRUE;
            bFCS = pData[uFrameHdrLen + uFrameInfLen + 1];
        }
        else
        {
            bHasCrField = FALSE;
            bFCS = pData[uFrameHdrLen + uFrameInfLen];
        }

        /* FCS check */
        if (!RFCOMM_FCS_CheckCRC(pData, 2, bFCS))
        {
            DBG_ERROR("RFCOMM Invalid Frame (Wrong CRC)\r\n");
            return FALSE;
        }

        /* If the channel is in DM it cannot handle any information */
        if (!psChannel->bEstablished)
        {
            DBG_ERROR("RFCOMM Channel inactive \r\n");
            return FALSE;
        }

        /*
         * Handle the information field depending on the channel:
         * Multiplexor (DLCI = 0): Message.
         * Data channel (DLCI >= 1): User data.
         */
        if (bChNumber == RFCOMM_CH_MUX)
        {
            bMsgType = pData[uOffset + 0];
            bMsgLen = (pData[uOffset + 1] >> 1);
            uMsgOffset = uOffset + 2;
            /* Switch according to the message type */
            switch (bMsgType)
            {
                case RFCOMM_PN_CMD:
                    bRetVal = _RFCOMM_handlePN(&pData[uMsgOffset], bMsgLen);
                    break;

                case RFCOMM_TEST_CMD:
                    bRetVal = _RFCOMM_handleTEST(&pData[uMsgOffset], bMsgLen);
                    break;

                case RFCOMM_MSC_CMD:
                    bRetVal = _RFCOMM_handleMSC(&pData[uMsgOffset], bMsgLen);
                    break;

                case RFCOMM_RPN_CMD:
                    bRetVal = _RFCOMM_handleRPN(&pData[uMsgOffset], bMsgLen);
                    break;

                case RFCOMM_RLS_CMD:
                    bRetVal = _RFCOMM_handleRLS(&pData[uMsgOffset], bMsgLen);
                    break;

                case RFCOMM_FCON_CMD:
                case RFCOMM_FCOFF_CMD:
                default:
                    DBG_INFO("RFCOMM Message %x not supported \r\n", bMsgType);
                    bRetVal = FALSE;
                    break;
            }
        }
        else
        {
            /* Handle the credit field (if any) */
            if(bHasCrField)
            {
                /* Add the credits on the remote end */
                psChannel->bRemoteCr += (UINT8) pData[uOffset + 0];
                ++uOffset;
            }
            /* Decrease the credit counter if the frame has user data */
            if (uFrameInfLen > 0)
            {
                psChannel->bLocalCr--;
                gpsRFCOMMCB->putRFCOMMData(&pData[uOffset], uFrameInfLen);

                /* In case of running low of credits, allocate some more */
                if (psChannel->bLocalCr < 0x08)
                {
                    bRetVal = _RFCOMM_sendUIHCr(bChNumber, 0x10);
                    if (bRetVal)
                    {
                        psChannel->bLocalCr += 0x010;
                    }
                }
            }
        }
        return bRetVal;
    }
    else
    {
        /* FCS check */
        bFCS = pData[uFrameHdrLen + uFrameInfLen];
        if (!RFCOMM_FCS_CheckCRC(pData, uFrameHdrLen, bFCS))
        {
            DBG_ERROR("RFCOMM Invalid Frame (Wrong CRC)\r\n");
            return FALSE;
        }
        /* Handle the frame according to the control field */
        switch (bCtrl)
        {
            case RFCOMM_SABM_FRAME|RFCOMM_PF_BIT:
                /* Check if the channel is established before sending the UA */
                if (psChannel->bEstablished)
                {
                    DBG_ERROR("RFCOMM Channel already established \r\n");
                    return FALSE;
                }
                /* Send the UA and establish the channel */
                bRetVal = _RFCOMM_sendUA(bChNumber);
                if (bRetVal)
                {
                    psChannel->bEstablished = TRUE;
                }
                break;

            case RFCOMM_UA_FRAME|RFCOMM_PF_BIT:
                break;

            case RFCOMM_DM_FRAME:
            case RFCOMM_DM_FRAME|RFCOMM_PF_BIT:
                break;

            case RFCOMM_DISC_FRAME|RFCOMM_PF_BIT:
                /* Send a UA frame and disconnect the channel */
                bRetVal = _RFCOMM_sendUA(bChNumber);
                if (bRetVal)
                {
                    psChannel->bEstablished = FALSE;
                    psChannel->bLocalCr = 0;
                    psChannel->bRemoteCr = 0;
                }
                break;

            default:
                /*
                 * Frames to be discarded:
                 * SABM, UA and DISC without the P/F bit.
                 */
                break;
        }
    }
}

BOOL RFCOMM_API_sendData(const BYTE *pData, UINT uLen)
{
    BOOL bRetVal = FALSE;
    UINT8 bChNum = RFCOMM_CH_DATA;
    RFCOMM_CHANNEL *psChannel = NULL;

    /* Check if the DATA CHANNEL is active */
    psChannel = _RFCOMM_getChannel(bChNum);
    if(NULL == psChannel ||
       psChannel->bRemoteCr == 0)
    {
        return FALSE;
    }

    /* Send a UIH frame containing the data */
    bRetVal = _RFCOMM_sendUIH(RFCOMM_CH_DATA, pData, uLen);
    if (bRetVal)
    {
        psChannel->bRemoteCr--;
    }

    return bRetVal;
}

BOOL RFCOMM_API_disconnect(UINT8 bChannel)
{
    BYTE aData[RFCOMM_DISC_LEN];
    BOOL bRetVal = FALSE;
    RFCOMM_CHANNEL *psChannel = NULL;

    psChannel = _RFCOMM_getChannel(bChannel);
    if(NULL == psChannel)
    {
        return FALSE;
    }
    /* Prepare the frame */
    /* Address */
    aData[0] = _RFCOMM_getAddress(bChannel, RFCOMM_CMD);
    /* Control */
    aData[1] = RFCOMM_DISC_FRAME|RFCOMM_PF_BIT;
    /* InfoLength */
    aData[2] = (0x00 << 1) | 0x01;
    /* FCS */
    aData[3] = RFCOMM_FCS_CalcCRC(aData, RFCOMM_HDR_LEN_1B);

    /* Send the frame */
    bRetVal = gpsRFCOMMCB->L2CAPsendData(L2CAP_RFCOMM_PSM,
            aData, RFCOMM_DISC_LEN);

    return bRetVal;
}

/*
 * RFCOMM private functions implementation
 */

RFCOMM_CHANNEL* _RFCOMM_getChannel(UINT8 uChNumber)
{
    RFCOMM_CHANNEL *pRetChannel = NULL;
    ASSERT(uChNumber < RFCOMM_NUM_CHANNELS);
    /* Get a handle to the channel */
    pRetChannel = (RFCOMM_CHANNEL *) &(gpsRFCOMMCB->asChannel[uChNumber]);

    return pRetChannel;
}

BYTE _RFCOMM_getAddress(UINT8 bChNumber, BYTE bType)
{
    BYTE bRole, bCR = 0x00;

    ASSERT(NULL != gpsRFCOMMCB);
    bRole = gpsRFCOMMCB->bRole;

    /* Set the Command/Response bit according to the role */
    if ((bType == RFCOMM_DATA) && (bRole == RFCOMM_ROLE_INITIATIOR)||
        (bType == RFCOMM_CMD) && (bRole == RFCOMM_ROLE_INITIATIOR) ||
        (bType == RFCOMM_RSP) && (bRole == RFCOMM_ROLE_RESPONDER))
    {
        bCR = 0x01;
    }
    return (bChNumber << 3) + (bRole << 2) + (bCR << 1) | 0x01;
}

BOOL _RFCOMM_sendUA(UINT8 bChNum)
{
    BYTE aData[RFCOMM_UA_LEN];
    BOOL bRetVal = FALSE;
    RFCOMM_CHANNEL *psChannel = NULL;

    psChannel = _RFCOMM_getChannel(bChNum);
    if(NULL == psChannel)
    {
        return FALSE;
    }
    /* Prepare the frame */
    /* Address */
    aData[0] = _RFCOMM_getAddress(bChNum, RFCOMM_RSP);
    /* Control */
    aData[1] = RFCOMM_UA_FRAME|RFCOMM_PF_BIT;
    /* InfoLength */
    aData[2] = (0x00 << 1) | 0x01;
    /* FCS */
    aData[3] = RFCOMM_FCS_CalcCRC(aData, RFCOMM_HDR_LEN_1B);

    /* Send the frame */
    bRetVal = gpsRFCOMMCB->L2CAPsendData(L2CAP_RFCOMM_PSM,
            aData, RFCOMM_UA_LEN);

    return bRetVal;
}

BOOL _RFCOMM_sendUIH(UINT8 bChNum, const BYTE *pData, UINT uLen)
{
    BYTE aFrame[RFCOMM_UIH_LEN + uLen + 1];
    UINT i;
    UINT8 uOffset = 0;
    BOOL bRetVal;
    RFCOMM_CHANNEL *psChannel = NULL;

    /* Check the channel */
    psChannel = _RFCOMM_getChannel(bChNum);
    if(NULL == psChannel)
    {
        return FALSE;
    }

    /* Fill the header */
    aFrame[0] = _RFCOMM_getAddress(bChNum, RFCOMM_DATA);
    aFrame[1] = RFCOMM_UIH_FRAME;
    /* The lenght field will be one or two octet long depending on uLen */
    if (uLen > 127)
    {
        aFrame[2] = (uLen << 1) & 0x00FE;
        aFrame[3] = (uLen >> 7) & 0x007F;
        uOffset = 4;
    }
    else
    {
        aFrame[2] = (uLen << 1) | 0x01;
        uOffset = 3;
    }

    /* Copy the payload */
    for (i = 0; i < uLen; ++i)
    {
        aFrame[uOffset + i] = pData[i];
    }
    
    /* Calculate the FCS */
    aFrame[uOffset + i] = RFCOMM_FCS_CalcCRC(aFrame, 2);

    /* Send the frame */
    bRetVal = gpsRFCOMMCB->L2CAPsendData(L2CAP_RFCOMM_PSM,
            aFrame, RFCOMM_UIH_LEN + uLen);

    return bRetVal;
}

BOOL _RFCOMM_sendUIHCr(UINT8 bChNum, UINT8 uNumCr)
{
    BYTE aFrame[RFCOMM_UIH_CR_LEN];
    BOOL bRetVal = FALSE;

    /* Send a UIH frame with credit field and no user data */
    aFrame[0] = _RFCOMM_getAddress(bChNum, RFCOMM_DATA);
    aFrame[1] = RFCOMM_UIH_FRAME | RFCOMM_PF_BIT;
    aFrame[2] = (0x00 << 1) | 0x01;
    aFrame[3] = uNumCr;
    aFrame[4] = RFCOMM_FCS_CalcCRC(aFrame, 2);

    bRetVal = gpsRFCOMMCB->L2CAPsendData(L2CAP_RFCOMM_PSM,
            aFrame, RFCOMM_UIH_CR_LEN);

    return bRetVal;
}

BOOL _RFCOMM_handlePN(const BYTE *pMsgData, UINT8 uMsgLen)
{
    UINT8 uChNum;
    UINT16 uMTU;
    BYTE aPNRsp[RFCOMM_MSGHDR_LEN + RFCOMM_PNMSG_LEN];
    BOOL bRetVal = FALSE;
    RFCOMM_CHANNEL *psChannel = NULL;

    /* Check the len */
    ASSERT(uMsgLen == RFCOMM_PNMSG_LEN);

    /* Get the Channel Number to configure */
    uChNum = pMsgData[0] >> 1;
    uMTU = BT_readLE16(pMsgData, 4);
    /* Check if we have that channel */
    psChannel = _RFCOMM_getChannel(uChNum);
    if(NULL == psChannel)
    {
        return FALSE;
    }
    /*
     * Fill the response PN frame header
     * MsgType = PN Response
     * MsgLength = RFCOMM_PNMSG_LEN (+E/A bit)
     */
    aPNRsp[0] = RFCOMM_PN_RSP;   
    aPNRsp[1] = (RFCOMM_PNMSG_LEN << 1) | 0x01;
    /*
     * Fill the default values
     * DLCI = DLCI to configure
     * I_CL = 0xE0 (Enable credit flow control) and 0x00 (Use UIH frames)
     * P = 0x00 (No priority, the lowest one)
     * T = 0x00 (T1 not negotiable in RFCOMM)
     * N = RFCOMM_MTU (Maximum frame size)
     * NA = 0x00 (N2 is always 0 in RFCOMM)
     * K = 0x07 (Starting number of credits)
     */
    aPNRsp[2] = (uChNum << 1) | gpsRFCOMMCB->bRole;
    aPNRsp[3] = (0x0E << 4) | 0x00;
    aPNRsp[4] = 0x00;
    aPNRsp[5] = 0x00;
    BT_storeLE16(RFCOMM_MTU, aPNRsp, 6);
    aPNRsp[8] = 0x00;
    aPNRsp[9] = 0x07;
    /* Check the CL and the I bits */
    if (pMsgData[1] != ((0x0F << 4)|(0x00)))
    {
        DBG_ERROR("RFCOMM Credit flow control MUST be supported! \r\n");
        return FALSE;
    }

    /* Set both the Remote and the Local Credits */
    psChannel->bLocalCr = 0x07;
    psChannel->bRemoteCr = pMsgData[7];
    
    /* 
     * Send the PN response
     */

    /* Same priority */
    aPNRsp[4] = pMsgData[2];
    /* Configure the minimum MTU */
    if (uMTU < RFCOMM_MTU)
    {
        BT_storeLE16(uMTU, aPNRsp, 6);
    }
    
    bRetVal = _RFCOMM_sendUIH(0x00, aPNRsp,
            RFCOMM_MSGHDR_LEN + RFCOMM_PNMSG_LEN);

    return bRetVal;
}

BOOL _RFCOMM_handleMSC(const BYTE *pMsgData, UINT8 uMsgLen)
{
    BOOL bRetVal = FALSE;
    UINT i;
    BYTE aResponse[RFCOMM_MSGHDR_LEN + RFCOMM_MSCMSG_LEN];
    BYTE aRequest[RFCOMM_MSGHDR_LEN + RFCOMM_MSCMSG_LEN];

    ASSERT(NULL != gpsRFCOMMCB);
    /* Check the message length */
    ASSERT(uMsgLen <= RFCOMM_MSCMSG_LEN);

    /* Fill the header */
    aResponse[0] = RFCOMM_MSC_RSP;
    aResponse[1] = (uMsgLen << 1) | 0x01;
    /* Get a copy of the received values */
    for(i = 0; i < uMsgLen; ++i)
    {
        aResponse[2 + i] = pMsgData[i];
    }
    /* Send the frame */
    bRetVal = _RFCOMM_sendUIH(0x00, aResponse, RFCOMM_MSGHDR_LEN + uMsgLen);
    if (!bRetVal)
    {
        return bRetVal;
    }

    /* Send a request after responding the command */
    /* Fill the header */
    aRequest[0] = RFCOMM_MSC_CMD;
    aRequest[1] = (RFCOMM_MSCMSG_LEN << 1) | 0x01;
    /* Fill the command */
    aRequest[2] = pMsgData[0];  /* Set the same DLCi as the initial command */
    aRequest[3] = 0x8D;         /* Set the DV, RTR, RTC and EA bits */
    aRequest[4] = 0x00;         /* No break signal */
    /* Send the frame */
    bRetVal = _RFCOMM_sendUIH(0x00, aRequest,
            RFCOMM_MSGHDR_LEN + RFCOMM_MSCMSG_LEN);
    return bRetVal;
}

BOOL _RFCOMM_handleRPN(const BYTE *pMsgData, UINT8 uMsgLen)
{
    BOOL bRetVal = FALSE;
    UINT8 uDLCI;
    BYTE aResponse[RFCOMM_MSGHDR_LEN + RFCOMM_RPNMSG_LEN];

    /* Check the message length */
    ASSERT(uMsgLen == RFCOMM_RPNMSG_LEN);

    /* Get the DLC to configure */
    uDLCI = pMsgData[0] >> 2;

    /* Fill the header */
    aResponse[0] = RFCOMM_RPN_RSP;
    aResponse[1] = (RFCOMM_RPNMSG_LEN << 1) | 0x01;
    /*
     * Fill the default values
     * DLCI = DLCI to configure
     * B = 0x03 (9600bps)
     * D_S_P_PT = PT=0 (odd parity),
     *            P=0 (No parity),
     *            S=0 (1 stop) and D=0x3 (8 bits)
     * FLC = 0x00 (No flow control)
     * XON = 0x00 (XON character)
     * XOFF = 0x00 (XOFF character)
     * PM = 0xFFFF (All new parameters accepted)
     */
    /* DLCI */
    aResponse[2] = (uDLCI << 2) | (0x01 << 1) | 0x01;
    /* Baudrate (0x03) = 9600 */
    aResponse[3] = 0x03;
    /* Data (0x03), stop (0), parity (0) and parity type (0) bits */
    aResponse[4] = (0 << 4) | (0 << 3) | (0 << 2) | 0x03;
    /* Flow control (0x00) = No FLC */
    aResponse[5] = 0x00;
    /* XON (0x00) and XOFF (0x00) characters */
    aResponse[6] = 0x00;
    aResponse[7] = 0x00;
    /* Parameter mask (0xFFFF) = Accept all the changes */
    aResponse[8] = 0xFF;
    aResponse[9] = 0xFF;

    bRetVal = _RFCOMM_sendUIH(0x00, aResponse,
            RFCOMM_MSGHDR_LEN + RFCOMM_RPNMSG_LEN);

    return bRetVal;
}

BOOL _RFCOMM_handleRLS(const BYTE *pMsgData, UINT8 uMsgLen)
{
    UINT8 uDLCI;
    BYTE bErrFlags, aRLSRsp[RFCOMM_MSGHDR_LEN + RFCOMM_RLSMSG_LEN];
    BOOL bRetVal = FALSE;

    /* Check the message length */
    ASSERT(uMsgLen == RFCOMM_RLSMSG_LEN);
    
    uDLCI = pMsgData[0];

    /* Fill the header */
    aRLSRsp[0] = RFCOMM_RLS_RSP;
    aRLSRsp[1] = (RFCOMM_RLSMSG_LEN << 1) | 0x01;
    /* DLC */
    aRLSRsp[2] = uDLCI;
    /* Line status */
    aRLSRsp[3] = pMsgData[1];
    /* If an error has occurred */
    if (pMsgData[1] & RFCOMM_MASK_RLS_ERROR)
    {
        bErrFlags = pMsgData[1] >> 1;
        if (bErrFlags & RFCOMM_MASK_RLS_OVERRUN)
        {
            /* TODO: Error handling */
        }
        if (bErrFlags & RFCOMM_MASK_RLS_PARITY)
        {
            /* TODO: Error handling */
        }
        if (bErrFlags & RFCOMM_MASK_RLS_FRAMING)
        {
            /* TODO: Error handling */
        }
    }

    bRetVal = _RFCOMM_sendUIH(RFCOMM_CH_MUX,
            aRLSRsp, RFCOMM_MSGHDR_LEN + RFCOMM_RLSMSG_LEN);

    return bRetVal;
}

BOOL _RFCOMM_handleTEST(const BYTE *pMsgData, UINT8 uMsgLen)
{
    BYTE aTESTRsp[RFCOMM_MSGHDR_LEN + uMsgLen];
    UINT i;
    BOOL bRetVal = FALSE;

    /* Fill the header */
    aTESTRsp[0] = RFCOMM_TEST_RSP;
    aTESTRsp[1] = (uMsgLen << 1) | 0x01;
    /* Fill the payload */
    for (i = 0; i < uMsgLen; ++i)
    {
        aTESTRsp[2 + i] = pMsgData[i];
    }

    bRetVal = _RFCOMM_sendUIH(RFCOMM_CH_MUX,
            aTESTRsp, RFCOMM_MSGHDR_LEN + uMsgLen);

    return bRetVal;
}

