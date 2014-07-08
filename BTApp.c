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


#include "debug.h"
#include "BTApp.h"

/*
 * BT APP definitions
 */

#if DEBUG_APP
#undef DBG_CLASS
#define DBG_CLASS DBG_CLASS_APP
#endif

/*
 * Bluetooth application private function prototypes
 */

BOOL BTAPP_API_putRFCOMMData(const BYTE *pData, UINT uLen);
BOOL BTAPP_API_confComplete();

/*
 * Bluetooth application implementation
 */

BOOL BTAPP_Initialise(BT_DEVICE **ppsBTDevice)
{
    BT_DEVICE *psBTDev = NULL;

    ASSERT(NULL != ppsBTDevice);
    
    *ppsBTDevice = (BT_DEVICE *) BT_malloc(sizeof(BT_DEVICE));
    psBTDev = *ppsBTDevice;

    ASSERT(NULL != psBTDev);

    /* Initialise all the BT stack layers */
    HCIUSB_create();
    HCI_create();
    L2CAP_create();
    SDP_create();
    RFCOMM_create();
    
    DBG_INFO("BTAPP: Bluetooth stack initialized\n\r");

    return TRUE;
}

BOOL BTAPP_Start(BT_DEVICE *psBTDevice)
{
    HCIUSB_API sPHY;
    HCI_API sHCI;
    L2CAP_API sL2CAP;
    DEVICE_API sAPI;
    RFCOMM_API sRFCOMM;

    ASSERT(NULL != psBTDevice);

    /* Get all the APIs needed */
    HCIUSB_getAPI(&sPHY);
    HCI_getAPI(&sHCI);
    L2CAP_getAPI(&sL2CAP);
    RFCOMM_getAPI(&sRFCOMM);
    /* Initialise the USB Device API directly related with the physical bus */
    psBTDevice->sUSB.getACLBuff = sPHY.getACLBuff;
    psBTDevice->sUSB.getEVTBuff = sPHY.getEVTBuff;
    psBTDevice->sUSB.writeACL = sPHY.USBwriteACL;
    psBTDevice->sUSB.writeCTL = sPHY.USBwriteCTL;
    /* Initialise the last part of the USB Device API */
    psBTDevice->sUSB.readACL = sHCI.putData;
    psBTDevice->sUSB.readEVT = sHCI.putEvent;
    /* L2CAP API */
    psBTDevice->L2CAPdisconnect = sL2CAP.disconnect;
    /* RFCOMM API */
    psBTDevice->SPPsendData = sRFCOMM.sendData;
    psBTDevice->SPPdisconnect = sRFCOMM.disconnect;

    /* Install the Device call-backs in the RFCOMM and HCI layer */
    sAPI.putRFCOMMData = &BTAPP_API_putRFCOMMData;
    sAPI.confComplete = &BTAPP_API_confComplete;
    RFCOMM_installDevCB(&sAPI);
    HCI_installDevCB(&sAPI);

    /* Issue the RESET command (which will trigger the HCI configuration) */
    sHCI.setLocalName("PIC_BT", 6);
//    sHCI.setPINCode("1234", 4);
    sHCI.cmdReset();

    DBG_INFO("BTAPP: Bluetooth stack start\n\r");

    return TRUE;
}

BOOL BTAPP_Deinitialise()
{
    HCIUSB_destroy();
    HCI_destroy();
    L2CAP_destroy();
    SDP_destroy();
    RFCOMM_destroy();
    return TRUE;
}

/*
 * Device call-back functions to be installed to the BT Stack
 */

/* Called by the RFCOMM after the reception of data from the remote Device */
BOOL BTAPP_API_putRFCOMMData(const BYTE *pData, UINT uLen)
{
    INT i = 0;
    for (i = 0; i < uLen; ++i)
    {
        SIOPutChar(pData[i]);
    }
    SIOPutChar('\n');
    RFCOMM_API_sendData("ACK ", 4);
    return TRUE;
}

/* Called by the HCI after completing the configuration of the local Device */
BOOL BTAPP_API_confComplete()
{
    DBG_INFO("BTAPP: HCI Configured\n");
    return TRUE;
}
