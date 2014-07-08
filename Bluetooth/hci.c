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
#include "hci.h"
#include "hci_usb.h"
#include "bt_utils.h"
#include "debug.h"

/*
 * HCI definitions
 */

#if DEBUG_HCI
#undef DBG_CLASS
#define DBG_CLASS DBG_CLASS_HCI
#endif

#if DEBUG_PHY
#undef DBG_CLASS
#define DBG_CLASS DBG_CLASS_PHY
#endif

#define PC_CLASS
//#define CELL_CLASS 

/*
 * HCI variables
 */

static HCI_CONTROL_BLOCK *gpsHCICB = NULL;

/*
 * HCI public functions implementation
 */

BOOL HCI_create()
{
    HCI_CONFIGURATION_DATA *psConfData;
    HCI_CONNECTION_DATA *psConnData;
    CHAR *pcDefaultName = "PIC32_BTv1";
    CHAR *pcDefaultPIN = "0000";
    UINT i;
    UINT uDefNameLen = 10;
    UINT uDefPINLen = 4;
    HCIUSB_API sHCIUSB;

    /*Allocate the control block memory*/
    gpsHCICB = BT_malloc(sizeof(HCI_CONTROL_BLOCK));
    ASSERT(NULL != gpsHCICB);

    /*Allocate and initialise the configuration data*/
    gpsHCICB->psHCIConfData = BT_malloc(sizeof(HCI_CONFIGURATION_DATA));
    psConfData = gpsHCICB->psHCIConfData;
    ASSERT(NULL != psConfData);

    psConfData->isConfigured = FALSE;

    psConfData->sLocalName = (CHAR*) BT_malloc(uDefNameLen * sizeof(CHAR));
    psConfData->uLocalNameLen = uDefNameLen;
    for(i = 0; i < uDefNameLen; ++i)
    {
        psConfData->sLocalName[i] = pcDefaultName[i];
    }

    psConfData->sPinCode = (CHAR*) BT_malloc(uDefPINLen * sizeof(CHAR));
    psConfData->uPinCodeLen = uDefPINLen;
    for(i = 0; i < uDefPINLen; ++i)
    {
        psConfData->sPinCode[i] = pcDefaultPIN[i];
    }
    
    psConfData->uHostAclBufferSize = DATA_PACKET_LENGTH - HCI_ACL_HDR_LEN;
    psConfData->uHostNumAclBuffers = 0; /*Infinite*/

    /*Allocate and initialise the connection data*/
    gpsHCICB->psHCIConnData = BT_malloc(sizeof(HCI_CONNECTION_DATA));
    psConnData = gpsHCICB->psHCIConnData;
    ASSERT(NULL != psConnData);
    
    psConnData->isConnected = FALSE;
    psConnData->uPacketsToAck = 0;
    psConnData->uConnHandler = 0;

    /*Initialise the internal API*/
    HCIUSB_getAPI(&sHCIUSB);

    gpsHCICB->PHY_w_ACL = sHCIUSB.USBwriteACL;
    gpsHCICB->PHY_w_CTL = sHCIUSB.USBwriteCTL;

    /*Raise the initialised flag*/
    gpsHCICB->isInitialised = TRUE;
    return TRUE;
}

BOOL HCI_destroy()
{
    if(NULL != gpsHCICB)
    {
        if(NULL != gpsHCICB->psHCIConfData)
        {
            BT_free(gpsHCICB->psHCIConfData->sLocalName);
            BT_free(gpsHCICB->psHCIConfData->sPinCode);
            BT_free(gpsHCICB->psHCIConfData);
        }
        if(NULL != gpsHCICB->psHCIConnData)
        {
            BT_free(gpsHCICB->psHCIConnData);
        }
        BT_free(gpsHCICB);
        gpsHCICB = NULL;
    }
    return TRUE;
}

BOOL HCI_getAPI(HCI_API *psAPI)
{
    ASSERT(NULL != gpsHCICB);
    psAPI->cmdReset = &HCI_API_cmdReset;
    psAPI->putData = &HCI_API_putData;
    psAPI->putEvent = &HCI_API_putEvent;
    psAPI->sendData = &HCI_API_sendData;
    psAPI->setLocalName = &HCI_API_setLocalName;
    psAPI->setPINCode = &HCI_API_setPINCode;
    return TRUE;
}

BOOL HCI_installL2CAP(L2CAP_API *psAPI)
{
    ASSERT(NULL != psAPI);
    gpsHCICB->L2CAPputData = psAPI->putData;
    return TRUE;
}

BOOL HCI_installDevCB(DEVICE_API *psAPI)
{
    ASSERT(NULL != psAPI);
    gpsHCICB->configurationComplete = psAPI->confComplete;
    return TRUE;
}

/*
 * HCI private functions implementation
 */

BOOL _HCI_isInitialized()
{
    ASSERT(NULL != gpsHCICB);
    return gpsHCICB->isInitialised;
}

BOOL _HCI_isConfigured()
{
    ASSERT(NULL != gpsHCICB);
    ASSERT(NULL != gpsHCICB->psHCIConfData);
    return gpsHCICB->psHCIConfData->isConfigured;
}

BOOL _HCI_isConnected()
{
    ASSERT(NULL != gpsHCICB);
    ASSERT(NULL != gpsHCICB->psHCIConnData);
    return gpsHCICB->psHCIConnData->isConnected;
}

int _HCI_getMaxAclFrameSize()
{
    return DATA_PACKET_LENGTH;
}

int _HCI_cmd(const BYTE *pData, BYTE bOCF, BYTE bOGF, unsigned uLen)
{
    int i;
    BYTE aCmdBuffer[uLen];
    UINT16 uCmdCode;

    /*Get the command code*/
    uCmdCode = bOCF | (bOGF<<10);

    /*Validate the input arguments*/
    ASSERT(uLen >= HCI_CMD_HDR_LEN);

    /*Store the command header*/
    BT_storeLE16(uCmdCode, aCmdBuffer, 0);
    BT_storeLE16(uLen-HCI_CMD_HDR_LEN, aCmdBuffer, 2);

    /*Copy the data into the command buffer*/
    for(i=HCI_CMD_HDR_LEN; i<uLen; ++i)
    {
        aCmdBuffer[i] = pData[i-HCI_CMD_HDR_LEN];
    }

	DelayMs(10);
    DBG_INFO( "HCI w CMD: ");
    DBG_DUMP(aCmdBuffer,uLen);

    /*Write the command*/
    gpsHCICB->PHY_w_CTL(aCmdBuffer, uLen);
	return uLen;
}

void _HCI_commandEnd(const BYTE *pEventData)
{
    BYTE aData[CONTROL_PACKET_LENGTH - HCI_CMD_HDR_LEN];
    HCI_CONFIGURATION_DATA *psConfData;
    INT i;
    UINT16 uCmdCode;

    ASSERT(NULL != gpsHCICB);
    psConfData = gpsHCICB->psHCIConfData;

    ASSERT(NULL != psConfData);

    /*Identify the completed command*/
    uCmdCode = BT_readLE16(pEventData, 3);
    switch (uCmdCode)
    {
        /*RESET complete*/
        case HCI_RESET_OCF|(HCI_HC_BB_OGF<<10):
            DBG_INFO( "HCI_RESET DONE\n");

            /*Issue the next command (READ_BUFFER_SIZE)*/
            _HCI_cmd(NULL, HCI_R_BUF_SIZE_OCF, HCI_INFO_PARAM_OGF,
                    HCI_R_BD_ADDR_PLEN);
            break;

        /*READ_BUFFER_SIZE complete*/
        case HCI_R_BUF_SIZE_OCF|(HCI_INFO_PARAM_OGF<<10):

            DBG_INFO( "HCI_R_BUF_SIZE DONE\n");
            i = BT_readLE16(pEventData, 6);
            DBG_INFO( "HC_ACL_Data_Packet_Length: %d\n", i);
            i = BT_readLE16(pEventData, 9);
            DBG_INFO( "HC_Total_Num_ACL_Data_Packets: %d\n", i);

            /*Issue the next command (READ_BD_ADDRESS)*/
            _HCI_cmd(NULL, HCI_R_BD_ADDR_OCF, HCI_INFO_PARAM_OGF,
                    HCI_R_BD_ADDR_PLEN);
            break;

        /*READ_BD_ADDRESS complete*/
        case HCI_R_BD_ADDR_OCF|(HCI_INFO_PARAM_OGF<<10):
            DBG_INFO( "HCI_R_BD_ADDR DONE\n");

            /*Store the Local Name and the BDADDR that we've just read*/
            for(i=0; i<6; ++i)
            {
                psConfData->aLocalADDR[5-i] = pEventData[6+i];
            }
			DelayMs(10);
            DBG_INFO( "Local DB_ADDR: ");
            DBG_DUMP(psConfData->aLocalADDR, 6);
            DBG_INFO( "Local Name: ");
            DBG_DUMP(psConfData->sLocalName,psConfData->uLocalNameLen);

            /*Fill the next command parameters*/

            /*ACL data packet length*/
            BT_storeLE16(psConfData->uHostAclBufferSize,aData,0);
            /*SYNC data packet length*/
            aData[2] = 0;
            /*Host ACL data packet buffer (0 = Infinite)*/
            BT_storeLE16(psConfData->uHostNumAclBuffers,aData,3);
            /*Host SYNC packet buffer*/
            BT_storeLE16(0,aData,5);

            /*Issue the next command (WRITE_HOST_BUFFER_SIZE)*/
            _HCI_cmd(aData,HCI_H_BUF_SIZE_OCF, HCI_HC_BB_OGF,
                    HCI_H_BUF_SIZE_PLEN);
            break;

        /*WRITE_HOST_BUFFER_SIZE complete*/
        case HCI_H_BUF_SIZE_OCF|(HCI_HC_BB_OGF<<10):
            DBG_INFO( "HCI_CONF_WRITE_HOST_BUFFER_SIZE DONE\n");

            /*Issue the next command (WRITE_LOCAL_NAME)*/
            _HCI_cmd(psConfData->sLocalName,HCI_W_LOCAL_NAME_OCF,HCI_HC_BB_OGF,
            HCI_CHANGE_LOCAL_NAME_PLEN + psConfData->uLocalNameLen);
            break;

        /*WRITE_LOCAL_NAME completed*/
        case HCI_W_LOCAL_NAME_OCF|(HCI_HC_BB_OGF<<10):
            DBG_INFO( "HCI_CONF_WRITE_LOCAL_NAME DONE\n");

            /*Issue the next command (READ_CLASS_OF_DEVICE)*/
            _HCI_cmd(NULL, HCI_R_COD_OCF, HCI_HC_BB_OGF, HCI_R_COD_PLEN);
            break;

        /*READ_CLASS_OF_DEVICE completed*/
        case HCI_R_COD_OCF|(HCI_HC_BB_OGF<<10):
            DBG_INFO("HCI_CONF_READ_CLASS_DEVICE DONE\n");

            /*Fill the next command parameters*/
            #ifdef PC_CLASS
                /*
                 * PC class:
                 * Major Class = Computer
                 * Minor Class = Desktop
                 */
                aData[0]=0x04;
                aData[1]=0x01;
                aData[2]=0x18;
            #elif CELL_CLASS
                /*
                 * Mobile phone class:
                 * Major Class = Mobile device
                 * Minor Class = Cell phone
                 * Services = Information
                 */
                aData[0]=0x04;
                aData[1]=0x02;
                aData[2]=0x80;
            #else
                #error
            #endif

            /*Issue the next command (WRITE_CLASS_OF_DEVICE)*/
            _HCI_cmd(aData, HCI_W_COD_OCF, HCI_HC_BB_OGF, HCI_W_COD_PLEN);
            break;

        /*WRITE_CLASS_OF_DEVICE completed*/
        case HCI_W_COD_OCF|(HCI_HC_BB_OGF<<10):
            DBG_INFO( "HCI_CONF_WRITE_CLASS_DEVICE DONE\n");

            /*
             * Fill the next command parameters
             * Inquiry Scan - Activate
             * Page Scan - Activate
             */
            aData[0]=0x03;

            /*Issue the next command (WRITE_SCAN_ENABLE)*/
            _HCI_cmd(aData, HCI_W_SCAN_EN_OCF, HCI_HC_BB_OGF,
                    HCI_W_SCAN_EN_PLEN);
            break;

         /*WRITE_SCAN_ENABLE complete*/
         case HCI_W_SCAN_EN_OCF|(HCI_HC_BB_OGF<<10):
            DBG_INFO( "HCI_CONF_WRITE_SCAN_ENABLE DONE\n");

            psConfData->isConfigured = TRUE;
            gpsHCICB->configurationComplete();
            break;

         /* WRITE STORED LINK KEY complete*/
        case HCI_W_STORED_LINK_KEY_OCF|(HCI_HC_BB_OGF<<10):
            DBG_INFO( "HCI_W_STORED_LINK_KEY DONE\n");
           /* Store the BD_ADDR */
            for (i = 0; i < 6; ++i)
            {
                aData[i] = gpsHCICB->psHCIConnData->aRemoteADDR[i];
            }
            aData[6] = 0x01;
            /*Issue the command (READ STORED LINK KEY)*/
            _HCI_cmd(aData, HCI_R_STORED_LINK_KEY_OCF, HCI_HC_BB_OGF,
                    HCI_R_STORED_LINK_KEY_PLEN);
            break;

        case HCI_R_STORED_LINK_KEY_OCF|(HCI_HC_BB_OGF<<10):
            DBG_INFO("HCI_R_STORED_LINK_KEY DONE\n");
            i = pEventData[5];
            if (i == 0x00)
            {
                DBG_INFO("Status: Command succeed\n");
                i = BT_readLE16(pEventData, 6);
                DBG_INFO("Max num keys: %d\n", i);
                i = BT_readLE16(pEventData, 8);
                DBG_INFO("Keys read: %d\n", i);
            }
            break;

        default:
            break;
    }
}

void _HCI_eventHandler(const BYTE *pEventData)
{
    BYTE aBDAddr[6];
    BYTE aData[EVENT_PACKET_LENGTH];
    UINT i, j, k;
    HCI_CONNECTION_DATA *psConnData;
    HCI_CONFIGURATION_DATA *psConfData;

    ASSERT(NULL != gpsHCICB);

    psConnData = gpsHCICB->psHCIConnData;
    psConfData = gpsHCICB->psHCIConfData;

    /*Switch cases according to the event code*/
    switch(pEventData[0])
    {
        /*NUMBER_OF_COMPLETED_PACKETS event*/
        case HCI_NBR_OF_COMPLETED_PACKETS:
            /*Verify the connection*/
            if(!_HCI_isConnected())
            {
                DBG_ERROR("HCI not connected.\n");
                return;
            }
            /*Check the connection handler*/
            if(BT_readLE16(pEventData, 3) == psConnData->uConnHandler)
            {
              /*Keep track of the packets to acknowledge by the device*/
              psConnData->uPacketsToAck -= pEventData[5];
            }
            break;

        /*PIN_CODE_REQUEST even*/
        case HCI_PIN_CODE_REQUEST:
            DBG_INFO( "HCI_PIN_CODE_REQUEST (Will return: 00)\n");

            /*Verify the configuration*/
            if(!_HCI_isConfigured())
            {
                DBG_ERROR("HCI not configured.\n");
                return;
            }

            /*Respond with a PIN Code Request Reply command*/

            /*Get the remote device BD ADDRESS*/
            for(i=0; i<6; ++i)
            {
                aBDAddr[i] = pEventData[2+i];
            }
            /*Issue the command using the external HCI API*/
            _HCI_cmdPinCodeRequestReply(aBDAddr, psConfData->sPinCode,
                    psConfData->uPinCodeLen);
            break;

        /*CONNECTION_REQUEST event*/
        case HCI_CONNECTION_REQUEST:
            /*Verify the configuration*/
            if(!_HCI_isConfigured())
            {
                DBG_ERROR("HCI not configured.\n");
                return;
            }

            /*Save the remote device BD ADDRESS*/
            for(i=0;i<6;++i)
            {
                psConnData->aRemoteADDR[i]=pEventData[HCI_EVENT_HDR_LEN+i];
            }

            /*Accept the connection*/

            /*Fill the command parameters*/
            /*The remote address*/
            for(i=0;i<6;++i)
            {
                aData[i] = psConnData->aRemoteADDR[i];
            }
            /*Continue as a slave*/
            aData[6] = 0x01;

            /*Issue the command (ACCEPT_CONNECTION_REQUEST)*/
            _HCI_cmd(aData, HCI_ACCEPT_CONN_REQ_OCF, HCI_LINK_CTRL_OGF,
                    HCI_ACCEPT_CONN_REQ_PLEN);
            break;

        /*CONNECTION_COMPLETE event*/
        case HCI_CONNECTION_COMPLETE:
            /*Verify the configuration*/
            if(!_HCI_isConfigured())
            {
                DBG_ERROR("HCI not configured.\n");
                return;
            }

            /*If the connection was sucessful*/
            if(pEventData[2] == HCI_SUCCESS)
            {
                    /*Save the connection handler*/
                    psConnData->uConnHandler = BT_readLE16(pEventData,3);
                    DBG_INFO( "HCI_CONNECTION_COMPLETE\n");

                    /*Raise the connection flag*/
                    psConnData->isConnected = TRUE;
            }
            else
            {
                    DBG_INFO( "HCI_CONNECTION_ERROR\n");
                    psConnData->isConnected = FALSE;
            }
            break;

        /*COMAND COMPLETE event*/
        case HCI_COMMAND_COMPLETE:
            /*Let the command handler take care of it...*/
            _HCI_commandEnd(pEventData);
            break;

        /*DISCONNECTION COMPLETE event*/
        case HCI_DISCONNECTION_COMPLETE:
            /*Verify the connection*/
            if(!_HCI_isConnected())
            {
                DBG_ERROR("HCI not connected.\n");
                return;
            }

            /*Lower the connection flag*/
            psConnData->isConnected = FALSE;
            DBG_INFO( "HCI_DISCONNECTION_COMPLETE\n");

            break;

        /* LINK KEY REQUEST event */
       case HCI_LINK_KEY_REQUEST:
           /* Store the BD_ADDR */
            for (i = 0; i < 6; ++i)
            {
                aData[i] = pEventData[2+i];
            }
            aData[6] = 0x01;
            /*Issue the command (READ STORED LINK KEY)*/
            _HCI_cmd(aData, HCI_R_STORED_LINK_KEY_OCF, HCI_HC_BB_OGF,
                    HCI_R_STORED_LINK_KEY_PLEN);
           break;

        /* LINK KEY NOTIFICATION event */
        case HCI_LINK_KEY_NOTIFICATION:
            /* Number of keys to store */
            aData[0] = 0x01;
            /* Store the BD_ADDR */
            for (i = 0; i < 6; ++i)
            {
                aData[1 + i] = pEventData[2+i];
            }
            /* Store the key itself */
            for (i = 0; i < 16; ++i)
            {
                aData[1 + 6 + i] = pEventData[8+i];
            }
            /*Issue the command (ACCEPT_CONNECTION_REQUEST)*/
            _HCI_cmd(aData, HCI_W_STORED_LINK_KEY_OCF, HCI_HC_BB_OGF,
                    HCI_W_STORED_LINK_KEY_PLEN);
            break;

        /* RETURN LINK KEY event */
        case HCI_RETURN_LINK_KEYS:
            DBG_INFO("Return Link Key event returned %d keys\n", i);
            /*
             * Check for each of the returned BD_ADDR if the value is the same
             * as the remote device. In case it is send the KEY REQUEST REPLY
             * command with the pair BDADDR/KEY.
             */
            i = pEventData[2];
            for (j = 0; j < i; ++j)
            {
                if (BT_isEqualBD_ADDR(psConnData->aRemoteADDR, 
                        &pEventData[1 + j*6]))
                {
                    for(k = 0; k < 6; ++k)
                    {
                        aData[k] = pEventData[1 + j*6 + k];
                    }
                    for(k = 0; k < 16; ++k)
                    {
                        aData[6 + k] = pEventData[1 + i*6 + k];
                    }
                    _HCI_cmd(aData, HCI_LINK_KEY_REQ_REP_OCF, HCI_LINK_CTRL_OGF,
                            HCI_LINK_KEY_REQ_REP_PLEN);
                }
            }
            break;

        default:
            break;
    }
}

/*
 * HCI API functions implementation
 */

void HCI_API_cmdReset()
{
    /*Send the RESET command to the device*/
    _HCI_cmd(NULL,HCI_RESET_OCF,HCI_HC_BB_OGF,HCI_RESET_PLEN);
}

BOOL HCI_API_setLocalName(const CHAR *pName, UINT uLen)
{
    UINT i;

    ASSERT(NULL != gpsHCICB);
    ASSERT(NULL != gpsHCICB->psHCIConfData);

    /* Free the old name and allocate enough space for the new one */
    BT_free(gpsHCICB->psHCIConfData->sLocalName);
    gpsHCICB->psHCIConfData->sLocalName =
            (CHAR*) BT_malloc(uLen * sizeof(CHAR));

    /* Set the name and the length of it */
    gpsHCICB->psHCIConfData->uLocalNameLen = uLen;
    for(i = 0; i < uLen; ++i)
    {
        gpsHCICB->psHCIConfData->sLocalName[i] = pName[i];
    }
    
    return TRUE;
}

BOOL HCI_API_setPINCode(const CHAR *pCode, UINT uLen)
{
    UINT i;

    ASSERT(NULL != gpsHCICB);
    ASSERT(NULL != gpsHCICB->psHCIConfData);
    
    /* Free the old code and allocate enough space for the new one */
    BT_free(gpsHCICB->psHCIConfData->sPinCode);
    gpsHCICB->psHCIConfData->sPinCode =
            (CHAR*) BT_malloc(uLen * sizeof(CHAR));

    /* Set the code and the length of it */
    gpsHCICB->psHCIConfData->uPinCodeLen = uLen;
    for(i = 0; i < uLen; ++i)
    {
        gpsHCICB->psHCIConfData->sPinCode[i] = pCode[i];
    }

    return TRUE;
}

void _HCI_cmdDisconnect()
{
    BYTE aData[HCI_DISCONN_PLEN - HCI_CMD_HDR_LEN];
    HCI_CONNECTION_DATA *psConnData;

    if(!_HCI_isConnected())
    {
        DBG_INFO("HCI not connected.\n");
        return;
    }
    psConnData = gpsHCICB->psHCIConnData;

    /*Fill the command parameters*/

    /*Connection handler*/
    BT_storeLE16(psConnData->uConnHandler,aData,0);
    /*Terminated by user*/
    aData[2]=0x13;

    /*Send the DISCONNECT command to the device*/
    _HCI_cmd(aData, HCI_DISCONN_OCF,HCI_LINK_CTRL_OGF, HCI_DISCONN_PLEN);
}

int _HCI_cmdPinCodeRequestReply(BYTE aBDAddr[6], const char *sPIN, unsigned uPINLen)
{
    INT i;
    INT16 cmd_code = HCI_PIN_CODE_REQ_REP_OCF|(HCI_LINK_CTRL_OGF<<10);
    BYTE aData[HCI_PIN_CODE_REQ_REP_PLEN-HCI_CMD_HDR_LEN];

    /*Validate the input arguments*/
    if((NULL == sPIN) || (uPINLen > 16))
    {
        DBG_ERROR("Invalid PIN code.\n");
        return 0;
    }

    /*Store the command parameters in aData*/

    /*Local device BD_ADDR*/
    for(i=0; i<6; ++i)
    {
        aData[i] = aBDAddr[i];
    }
    /*PIN length*/
    aData[6] = uPINLen;
    /*PIN code*/
    for(i=0;i<uPINLen;++i)
    {
        aData[7+i] = sPIN[i];
    }
    for(i=uPINLen;i<16;++i)
    {
        aData[7+i] = 0x00;
    }

    /*Send the PIN_CODE_REQUEST_REPLY command to the device*/
    _HCI_cmd(aData, cmd_code,HCI_LINK_CTRL_OGF, HCI_PIN_CODE_REQ_REP_PLEN);
    return HCI_PIN_CODE_REQ_REP_PLEN;
}

/*Send data to the remote device*/
/*NOTICE: Does not support fragmentation*/
BOOL HCI_API_sendData(const BYTE *pData, unsigned uLen)
{
    int i, uLength;
    BYTE PB_flag, BC_flag;
    HCI_CONNECTION_DATA *psConnData;
    UINT16 ConnHandler;
    BYTE aUSBData[uLen + HCI_ACL_HDR_LEN];

    /*Verify the connection*/
    ASSERT(_HCI_isConnected());
    psConnData = gpsHCICB->psHCIConnData;

    /*Verify the data*/
    if(NULL == pData || !uLen)
    {
        DBG_ERROR("Empty packet.\n");
        return FALSE;
    }

    /*Packet boundary flag = 10 (First packet of higher layer)*/
    PB_flag = 0b0010;
    /*Broadcast flag = 00 (No broadcast, point-to-point)*/
    BC_flag = 0b0000;
    ConnHandler = psConnData->uConnHandler|((PB_flag|BC_flag)<<12);

    /*Check if the data fits in a single HCI frame*/
    uLength = uLen + HCI_ACL_HDR_LEN;
    ASSERT(uLength <= DATA_PACKET_LENGTH);/* Packet frag. not implemented */

    /*Fill the header with the connection handler and the data lenght*/
    BT_storeLE16(ConnHandler, aUSBData, 0);
    BT_storeLE16(uLen, aUSBData, 2);
    /*Fill the rest of the packet*/
    for(i=0; i<uLen; ++i)
    {
        aUSBData[i+HCI_ACL_HDR_LEN] = pData[i];
    }
    /*Write the packet (using the hci_usb API)*/
    if(gpsHCICB->PHY_w_ACL(aUSBData, uLength)!=HCI_USB_BUSY)
    {
		DelayMs(10);
        DBG_INFO( "HCI w ACL: ");
        DBG_DUMP(aUSBData, uLen + HCI_ACL_HDR_LEN);
        ++psConnData->uPacketsToAck;
        return TRUE;
    }
    DBG_INFO( "HCI w ACL: BUSY\n");
    return FALSE;
}

BOOL HCI_API_putData(const BYTE *pData, unsigned uLen)
{
    BOOL bContinuation = FALSE;
    UINT16 uConnHandle;

    /*Verify the connection*/
    ASSERT(_HCI_isConnected());

    /*Verify the data*/
    if(NULL == pData || (uLen<HCI_ACL_HDR_LEN))
    {
        DBG_ERROR("Packet size error.\n");
        return FALSE;
    }

	DelayMs(10);
    DBG_INFO( "HCI r ACL: ");
    DBG_DUMP(pData,uLen);

    /*
     * Check if the Packet Boundary flag is set (bContinuation):
     * Is used to indicate if there are more fragments of the packet.
     * bContinuation FALSE: This is the last fragment of a given packet.
     * bContinuation TRUE: More fragments follow this one.
     */
    uConnHandle = BT_readLE16(pData,0);
    if(uConnHandle & 0x1000)
    {
        bContinuation = TRUE;
        DBG_ERROR("Packet fragmentation not implemented in the HCI level.\n");
    }
    /*Put the data into the L2CAP layer*/
    gpsHCICB->L2CAPputData(&pData[HCI_ACL_HDR_LEN], uLen-HCI_ACL_HDR_LEN,
            bContinuation);
    return TRUE;
}

BOOL HCI_API_putEvent(const BYTE *pData, unsigned uLen)
{
    /*Check the data*/
    if(!pData || (uLen < HCI_EVENT_HDR_LEN))
    {
        DBG_ERROR("Not a correct event.");
        return FALSE;
    }
	DelayMs(10);
    DBG_INFO( "HCI r EVT: ");
    DBG_DUMP(pData,uLen);

    /*Forward the event to the event handler*/
    _HCI_eventHandler(pData);
    return TRUE;
}
