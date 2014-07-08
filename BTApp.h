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

#ifndef __BTApp_H__
#define __BTApp_H__

#include "GenericTypeDefs.h"
#include "bt_common.h"

/*
 * Bluetooth application structure definitions
 */

/* Physical bus generic call-back interface */
typedef struct _PHY_BUS
{
    BYTE* (*getACLBuff)(void);
    BYTE* (*getEVTBuff)(void);

    INT (*writeACL)(const BYTE*, UINT);
    INT (*writeCTL)(const BYTE*, UINT);

    BOOL (*readACL)(const BYTE*, UINT);
    BOOL (*readEVT)(const BYTE*, UINT);
} PHY_BUS;

/* Device call-back interface */
typedef struct _BT_DEVICE
{
    /* PHY_BUS API */
    PHY_BUS sUSB;
    /* L2CAP_API */
    BOOL (*L2CAPdisconnect)(UINT16);
    /* RFCOMM API */
    BOOL (*SPPsendData)(const BYTE*, UINT);
    BOOL (*SPPdisconnect)(UINT8);
} BT_DEVICE;

/*
 * Bluetooth application public function prototypes
 */

BOOL BTAPP_Initialise(BT_DEVICE **ppsBTDevice);
BOOL BTAPP_Start(BT_DEVICE *psBTDevice);
BOOL BTAPP_Deinitialise();

#endif /*BTApp*/
