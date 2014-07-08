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

#ifndef __L2CAP_H__
#define __L2CAP_H__

#include "bt_common.h"

/*
 * L2CAP Definitions
 */

/*Packet header lengths*/
#define L2CAP_HDR_LEN 4
#define L2CAP_SIGHDR_LEN 4

/*Signal sizes*/
#define L2CAP_CONN_REQ_SIZE 4
#define L2CAP_CONN_RSP_SIZE 8
#define L2CAP_CFG_REQ_SIZE 4
#define L2CAP_CFG_RSP_SIZE 6
#define L2CAP_DISCONN_REQ_SIZE 4
#define L2CAP_DISCONN_RSP_SIZE 4
#define L2CAP_INFO_REQ_SIZE 2
#define L2CAP_INFO_RSP_SIZE 4

/*Signal codes*/
#define L2CAP_CMD_REJ 0x01
#define L2CAP_CONN_REQ 0x02
#define L2CAP_CONN_RSP 0x03
#define L2CAP_CFG_REQ 0x04
#define L2CAP_CFG_RSP 0x05
#define L2CAP_DISCONN_REQ 0x06
#define L2CAP_DISCONN_RSP 0x07
#define L2CAP_INFO_REQ 0x0A
#define L2CAP_INFO_RSP 0x0B

/*Channel identifiers*/
#define L2CAP_NULL_CID 0x0000
#define L2CAP_SIG_CID 0x0001
#define L2CAP_MIN_CID 0x0040
#define L2CAP_MAX_CID 0xFFFF

/*Configuration types*/
#define L2CAP_CFG_MTU 0x01
#define L2CAP_CFG_FLUSHTO 0x02

/*Configuration types length*/
#define L2CAP_CFG_MTU_LEN 2
#define L2CAP_CFG_FLUSHTO_LEN 2

/*Information request InfoType*/
#define L2CAP_INFO_CONNECTIONLESS_MTU 0x0001
#define L2CAP_INFO_EXTENDED_FEATURES 0x0002
/*Information response results*/
#define L2CAP_INFO_SUCCESS 0x0000
#define L2CAP_INFO_NOT_SUPPORTED 0x0001

/*Configuration response types*/
#define L2CAP_CFG_SUCCESS 0x0000

/*Connection response results*/
#define L2CAP_CONN_SUCCESS 0x0000

/*Protocol and service multiplexor*/
#define L2CAP_SDP_PSM 0x0001
#define L2CAP_RFCOMM_PSM 0x0003

/*
 * L2CAP default parameters
 * MTU = Payload carried by two baseband DH5 packets (2*341=682) 
 *       minus the Baseband ACL headers (2*2=4) minus L2CAP header (6)
 * NOTICE: We don't use the default one.
 */
#define L2CAP_DEFAULT_MTU 672

/* MTU = DATA PACKET LENGTH (256) minus the L2CAP and the HCI headers (4 + 4)*/
//#define L2CAP_MTU 248
#define L2CAP_MTU 672

//#define L2CAP_MAX_CHANNELS 2
#define L2CAP_MAX_CHANNELS 6

/*
 * L2CAP structure definitions
 */

typedef enum
{
	L2CAP_STATE_CLOSED = 0,
	L2CAP_STATE_WAIT_CONNECT,
        L2CAP_STATE_WAIT_CONNECT_RSP,
	L2CAP_STATE_WAIT_CONFIG,
	L2CAP_STATE_WAIT_SEND_CONFIG,
	L2CAP_STATE_WAIT_CONFIG_REQ_RSP,
	L2CAP_STATE_WAIT_CONFIG_RSP,
	L2CAP_STATE_WAIT_CONFIG_REQ,
	L2CAP_STATE_OPEN,
	L2CAP_STATE_WAIT_DISCONNECT
} L2CAP_STATE;

/* Channel definition */
typedef struct _L2CAP_CHANNEL
{
    UINT8 uIndex;
    BOOL isLinked;
    L2CAP_STATE uState;

    UINT16 uLocalCID;
    UINT16 uRemoteCID;

    UINT16 uPSMultiplexor;
} L2CAP_CHANNEL;

/* Control Block */
typedef struct _L2CAP_CONTROL_BLOCK
{
    BOOL isInitialised;
    UINT8 bSigID;
    L2CAP_CHANNEL *pasChannel[L2CAP_MAX_CHANNELS];

    /* HCI API */
    BOOL (*HCIsendData)(const BYTE*, UINT);
    /* RFCOMM API */
    BOOL (*RFCOMMputData)(const BYTE*, UINT);
    /* SDP API */
    BOOL (*SDPputData)(const BYTE*, UINT);
} L2CAP_CONTROL_BLOCK;

/*
 * L2CAP layer private function prototypes
 */

/* Private API */
BOOL L2CAP_API_putData(const BYTE *pData, UINT16 uLen, BOOL bContinuation);
BOOL L2CAP_API_sendData(UINT16 uPSM, BYTE const *pData, UINT16 uLen);
BOOL L2CAP_API_disconnect(UINT16 uPSM);

/* Private functions */
L2CAP_CHANNEL* _L2CAP_createChannel();
BOOL _L2CAP_destroyChannel(L2CAP_CHANNEL* pChannel);
L2CAP_CHANNEL* _L2CAP_getChannelByLCID(UINT16 uLocalCID);
L2CAP_CHANNEL* _L2CAP_getChannelByPSM(UINT16 uPSM);

BOOL _L2CAP_dataHandler(UINT16 uCID, UINT16 uLen, const BYTE *pData);
BOOL _L2CAP_cmdHandler(UINT8 bCode, UINT8 bId, UINT16 uLen, const BYTE *pData);

BOOL _L2CAP_acceptConnetion(UINT8 bId, L2CAP_CHANNEL* pChannel);
BOOL _L2CAP_configResponse(UINT8 bId, UINT uMTU, L2CAP_CHANNEL* pChannel);
BOOL _L2CAP_configRequest(UINT8 bId, UINT uMTU, L2CAP_CHANNEL* pChannel);
BOOL _L2CAP_disconnResponse(UINT8 bId, L2CAP_CHANNEL* pChannel);
BOOL _L2CAP_infoResponse(UINT8 bId, UINT16 uInfoType);

#endif /*L2CAP*/
