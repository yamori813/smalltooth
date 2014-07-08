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

#ifndef __RFCOMM_H__
#define __RFCOMM_H__

#include "bt_common.h"

/*
 * RFCOMM definitions
 */

/* Control field values */
#define RFCOMM_SABM_FRAME 0x2F
#define RFCOMM_UA_FRAME 0x63
#define RFCOMM_DM_FRAME 0x0F
#define RFCOMM_DISC_FRAME 0x43
#define RFCOMM_UIH_FRAME 0xEF
#define RFCOMM_PF_BIT 0x10

/* Length of the frame header */
#define RFCOMM_HDR_LEN_1B 3
#define RFCOMM_HDR_LEN_2B 4

/* Length of a frame */
#define RFCOMM_SABM_LEN 4
#define RFCOMM_UA_LEN 4
#define RFCOMM_UIH_LEN 4
#define RFCOMM_UIH_CR_LEN 5
#define RFCOMM_DM_LEN 4
#define RFCOMM_DISC_LEN 4

/* Length of a multiplexer message */
#define RFCOMM_MSGHDR_LEN 2
/* Header not included */
#define RFCOMM_PNMSG_LEN 8
#define RFCOMM_MSCMSG_LEN 3
#define RFCOMM_RLSMSG_LEN 2
#define RFCOMM_RPNMSG_LEN 8
#define RFCOMM_NCMSG_LEN 1

/* Multiplexer message types */
#define RFCOMM_PN_CMD 0x83
#define RFCOMM_PN_RSP 0x81
#define RFCOMM_TEST_CMD 0x23
#define RFCOMM_TEST_RSP 0x21
#define RFCOMM_FCON_CMD 0xA3
#define RFCOMM_FCON_RSP 0xA1
#define RFCOMM_FCOFF_CMD 0x63
#define RFCOMM_FCOFF_RSP 0x61
#define RFCOMM_MSC_CMD 0xE3
#define RFCOMM_MSC_RSP 0xE1
#define RFCOMM_RPN_CMD 0x93
#define RFCOMM_RPN_RSP 0x91
#define RFCOMM_RLS_CMD 0x53
#define RFCOMM_RLS_RSP 0x51
#define RFCOMM_NSC_RSP 0x11

/* Masks */
#define RFCOMM_MASK_LI_1B 0x01
#define RFCOMM_MASK_RLS_ERROR 0x01
#define RFCOMM_MASK_RLS_OVERRUN 0x04
#define RFCOMM_MASK_RLS_PARITY 0x02
#define RFCOMM_MASK_RLS_FRAMING 0x01

/* Role configuration */
#define RFCOMM_CMD 0x01
#define RFCOMM_RSP 0x02
#define RFCOMM_DATA 0x03
#define RFCOMM_ROLE_RESPONDER 0x00
#define RFCOMM_ROLE_INITIATIOR 0x01

#define RFCOMM_MTU 242

/*
 * RFCOMM structure definition
 */

typedef struct _RFCOMM_CHANNEL
{
    /* After receiving the SABM the channel will be established */
    BOOL bEstablished;
    /* Only relevant to the data channel, after doing the PN and the MSC */
    BOOL bDataEnabled;
    UINT8 bDLC;
    UINT8 bLocalCr;
    UINT8 bRemoteCr;
} RFCOMM_CHANNEL;

typedef struct _RFCOMM_CONTROL_BLOCK
{
    BOOL isInitialised;

    /* RFCOMM_NUM_CHANNELS, multiplexer control channel and data channels */
    RFCOMM_CHANNEL asChannel[RFCOMM_NUM_CHANNELS];

    /* The role can be either initiator (0x01) or responder (0x00) */
    UINT8 bRole;

    BOOL (*L2CAPsendData)(UINT16, const BYTE*, UINT16);
    BOOL (*putRFCOMMData)(const BYTE*, UINT);
    BOOL (*disconnComplete)(UINT8);

} RFCOMM_CONTROL_BLOCK;

/*
 * RFCOMM layer private function prototypes
 */

BYTE _RFCOMM_getAddress(UINT8 bChNumber, BYTE bType);
RFCOMM_CHANNEL* _RFCOMM_getChannel(UINT8 uChNumber);

BOOL _RFCOMM_sendUA(UINT8 bChNum);
BOOL _RFCOMM_sendUIH(UINT8 bChNum, const BYTE *pData, UINT uLen);
BOOL _RFCOMM_sendUIHCr(UINT8 bChNum, UINT8 uNumCr);

BOOL _RFCOMM_handlePN(const BYTE *pMsgData, UINT8 uMsgLen);
BOOL _RFCOMM_handleRPN(const BYTE *pMsgData, UINT8 uMsgLen);
BOOL _RFCOMM_handleRLS(const BYTE *pMsgData, UINT8 uMsgLen);
BOOL _RFCOMM_handleMSC(const BYTE *pMsgData, UINT8 uMsgLen);
BOOL _RFCOMM_handleTEST(const BYTE *pMsgData, UINT8 uMsgLen);

BOOL RFCOMM_API_putData(const BYTE *pData, UINT uLen);
BOOL RFCOMM_API_sendData(const BYTE *pData, UINT uLen);
BOOL RFCOMM_API_disconnect(UINT8 bChannel);

#endif /*__RFCOMM_H__*/
