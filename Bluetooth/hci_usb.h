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

#ifndef __HCI_USB_H__
#define __HCI_USB_H__

#include "GenericTypeDefs.h"
#include "bt_common.h"

#define HCI_USB_SUCCESS 0x10
#define HCI_USB_BUSY 0x11
#define HCI_USB_ERROR 0x12

typedef struct _HCIUSB_CONTROL_BLOCK
{
    BOOL isInitialised;

    BYTE *pREvtData;
    BYTE *pRAclData;

    HCIUSB_API sAPI;
} HCIUSB_CONTROL_BLOCK;

/*
 * HCIUSB layer private function prototypes
 */

BYTE* _getACLBuffer();
BYTE* _getEVTBuffer();

INT _w_ACL(const BYTE *pData, UINT uLength);
INT _w_CTRL(const BYTE *pData, UINT uLength);

#endif /*HCI_USB*/
