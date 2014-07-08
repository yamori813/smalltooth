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

#ifndef __HCI_H__
#define __HCI_H__

#include "bt_common.h"

/*
 * HCI definitions
 */

/*Headers length*/
#define HCI_EVENT_HDR_LEN 2
#define HCI_ACL_HDR_LEN 4
#define HCI_SCO_HDR_LEN 3
#define HCI_CMD_HDR_LEN 3

/*Possible event codes*/
#define HCI_CONNECTION_COMPLETE 0x03
#define HCI_CONNECTION_REQUEST 0x04
#define HCI_DISCONNECTION_COMPLETE 0x05
#define HCI_COMMAND_COMPLETE 0x0E
#define HCI_COMMAND_STATUS 0x0F
#define HCI_NBR_OF_COMPLETED_PACKETS 0x13
#define HCI_RETURN_LINK_KEYS 0x15
#define HCI_PIN_CODE_REQUEST 0x16
#define HCI_LINK_KEY_REQUEST 0x17
#define HCI_LINK_KEY_NOTIFICATION 0x18

/*Success code*/
#define HCI_SUCCESS 0x00

/*Specification specific parameters*/
#define HCI_BD_ADDR_LEN 6

/*Command OGF*/
#define HCI_LINK_CTRL_OGF 0x01
#define HCI_LINK_POLICY_OGF 0x02
#define HCI_HC_BB_OGF 0x03
#define HCI_INFO_PARAM_OGF 0x04

/*Command OCF*/
#define HCI_DISCONN_OCF 0x06
#define HCI_ACCEPT_CONN_REQ_OCF 0x09
#define HCI_RESET_OCF 0x03
#define HCI_W_SCAN_EN_OCF 0x1A
#define HCI_R_COD_OCF 0x23
#define HCI_W_COD_OCF 0x24
#define HCI_H_BUF_SIZE_OCF 0x33
#define HCI_H_NUM_COMPL_OCF 0x35
#define HCI_R_BUF_SIZE_OCF 0x05
#define HCI_R_BD_ADDR_OCF 0x09
#define HCI_W_LOCAL_NAME_OCF 0x13
#define HCI_PIN_CODE_REQ_REP_OCF 0x0D
#define HCI_R_STORED_LINK_KEY_OCF 0x0D
#define HCI_W_STORED_LINK_KEY_OCF 0x11
#define HCI_LINK_KEY_REQ_REP_OCF 0x0B

/*Command packet length (including ACL header)*/
#define HCI_DISCONN_PLEN 6
#define HCI_ACCEPT_CONN_REQ_PLEN 10
#define HCI_PIN_CODE_REQ_REP_PLEN 26
#define HCI_CHANGE_LOCAL_NAME_PLEN 4
#define HCI_RESET_PLEN 3
#define HCI_W_SCAN_EN_PLEN 4
#define HCI_R_COD_PLEN 3
#define HCI_W_COD_PLEN 6
#define HCI_H_BUF_SIZE_PLEN 10
#define HCI_H_NUM_COMPL_PLEN 7
#define HCI_R_BUF_SIZE_PLEN 4
#define HCI_R_BD_ADDR_PLEN 3
#define HCI_R_STORED_LINK_KEY_PLEN 10
#define HCI_W_STORED_LINK_KEY_PLEN 26
#define HCI_LINK_KEY_REQ_REP_PLEN 27

/*
 * HCI structure definitions
 */

/*Connection data structure*/
typedef struct _HCI_CONNECTION_DATA
{
	BOOL isConnected;
	BYTE aRemoteADDR[6];
	UINT16 uConnHandler;
        unsigned uPacketsToAck;
} HCI_CONNECTION_DATA;

/*Configuration data structure*/
typedef struct _HCI_CONFIGURATION_DATA
{
	BOOL isConfigured;
	char *sLocalName;
	unsigned uLocalNameLen;
	char *sPinCode;
	unsigned uPinCodeLen;
	BYTE aLocalADDR[6];
        UINT16 uHostAclBufferSize;
        UINT16 uHostNumAclBuffers;
        BOOL bCHFlowControl;
} HCI_CONFIGURATION_DATA;

/* Control block */
typedef struct _HCI_CONTROL_BLOCK
{
    BOOL isInitialised;
    HCI_CONFIGURATION_DATA *psHCIConfData;
    HCI_CONNECTION_DATA *psHCIConnData;

    INT (*PHY_w_ACL)(const BYTE*,UINT);
    INT (*PHY_w_CTL)(const BYTE*,UINT);

    BOOL (*L2CAPputData)(const BYTE*, UINT16, BOOL);

    BOOL (*configurationComplete)(void);
} HCI_CONTROL_BLOCK;

/*
 * HCI layer private function prototypes
 */

/* Private API */
void HCI_API_cmdReset();
BOOL HCI_API_setLocalName(const CHAR *pName, UINT uLen);
BOOL HCI_API_setPINCode(const CHAR *pCode, UINT uLen);
BOOL HCI_API_sendData(const BYTE *pData, unsigned uLen);
BOOL HCI_API_putData(const BYTE *pData, unsigned uLen);
BOOL HCI_API_putEvent(const BYTE *pData, unsigned uLen);

/* Private functions */
BOOL _HCI_isInitialized();
BOOL _HCI_isConfigured();
BOOL _HCI_isConnected();
int _HCI_getMaxAclFrameSize();

int _HCI_cmd(const BYTE *pData, BYTE bOCF, BYTE bOGF, unsigned uLen);
void _HCI_cmdDisconnect();
int _HCI_cmdPinCodeRequestReply(BYTE aBDAddr[6], const char *sPIN, unsigned uPINLen);

void _HCI_commandEnd(const BYTE *pEventData);
void _HCI_eventHandler(const BYTE *pEventData);

#endif //HCI.H
