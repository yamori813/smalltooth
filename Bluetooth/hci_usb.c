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

#include "bt_utils.h"
#include "hci_usb.h"
#include "debug.h"
#include "usb_config.h"
#include "USB/usb.h"
#include "PIC32_USB/usb_host_bluetooth.h"
#include "HardwareProfile.h"

static HCIUSB_CONTROL_BLOCK *gpsHCIUSBCB = NULL;

/*
 * HCIUSB public functions implementation
 */

BOOL HCIUSB_create()
{
    ASSERT(NULL == gpsHCIUSBCB);

    gpsHCIUSBCB =
            (HCIUSB_CONTROL_BLOCK *) BT_malloc(sizeof(HCIUSB_CONTROL_BLOCK));

    gpsHCIUSBCB->isInitialised = TRUE;
    gpsHCIUSBCB->pRAclData = (BYTE *) BT_malloc(DATA_PACKET_LENGTH);
    gpsHCIUSBCB->pREvtData = (BYTE *) BT_malloc(EVENT_PACKET_LENGTH);

    return TRUE;
}

BOOL HCIUSB_destroy()
{
    if(NULL != gpsHCIUSBCB)
    {
        if(NULL != gpsHCIUSBCB->pRAclData)
        {
            BT_free(gpsHCIUSBCB->pRAclData);
        }
        if(NULL != gpsHCIUSBCB->pREvtData)
        {
            BT_free(gpsHCIUSBCB->pREvtData);
        }
        BT_free(gpsHCIUSBCB);
        gpsHCIUSBCB = NULL;
    }
    return TRUE;
}

BOOL HCIUSB_getAPI(HCIUSB_API* psAPI)
{
    ASSERT(NULL != gpsHCIUSBCB);
    /* Initialise the API */
    psAPI->getACLBuff = &_getACLBuffer;
    psAPI->getEVTBuff = &_getEVTBuffer;
    psAPI->USBwriteACL = &_w_ACL;
    psAPI->USBwriteCTL = &_w_CTRL;
    return TRUE;
}

/*
 * HCIUSB private functions implementation
 */

BYTE* _getACLBuffer()
{
    ASSERT(NULL != gpsHCIUSBCB);
    return gpsHCIUSBCB->pRAclData;
}

BYTE* _getEVTBuffer()
{
    ASSERT(NULL != gpsHCIUSBCB);
    return gpsHCIUSBCB->pREvtData;
}

/*Write the data in the EP02 (bulk)*/
INT _w_ACL(const BYTE *pData, UINT uLength)
{
    BYTE bDevAddr = USBHostBluetoothGetDeviceAddress();
    BOOL bRetVal = USBHostWrite(bDevAddr,
            _EP02_OUT, (BYTE* ) pData, uLength);

    if(bRetVal == USB_SUCCESS)
    {
        DelayMs(1);
        return HCI_USB_SUCCESS;
    }
    return HCI_USB_BUSY;
}

/*Write the data in the EP0 (control)*/
INT _w_CTRL(const BYTE *pData, UINT uLength)
{
    BYTE bDevAddr = USBHostBluetoothGetDeviceAddress();
    BYTE bType = USB_SETUP_HOST_TO_DEVICE |
                 USB_SETUP_TYPE_CLASS |
                 USB_SETUP_RECIPIENT_DEVICE;
    BOOL bRetVal = USBHostIssueDeviceRequest(bDevAddr, bType, 0, 0, 0, uLength,
            (BYTE *)pData, USB_DEVICE_REQUEST_SET, 0x00);

    if (bRetVal == USB_SUCCESS )
    {
        DelayMs(25);
        return HCI_USB_SUCCESS;
    }
    return HCI_USB_BUSY;
}
