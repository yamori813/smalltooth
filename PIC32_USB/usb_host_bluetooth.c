#include <stdlib.h>
#include <string.h>
#include "GenericTypeDefs.h"
#include "HardwareProfile.h"
#include "USB/usb.h"
#include "usb_host_bluetooth.h"

#ifndef USB_MAX_BLUETOOTH_DEVICES
    #define USB_MAX_BLUETOOTH_DEVICES     1
#endif

#if USB_MAX_BLUETOOTH_DEVICES != 1
    #error The Generic client driver supports only one attached device.
#endif

BLUETOOTH_DEVICE  gc_DevData;

BOOL USBHostBluetoothInit ( BYTE address, DWORD flags, BYTE clientDriverID )
{
    BYTE *pDesc;

    // Initialize state
    gc_DevData.rxEvtLength     = 0;
    gc_DevData.rxAclLength     = 0;
    gc_DevData.flags.val = 0;

    // Save device the address, VID, & PID
    gc_DevData.ID.deviceAddress = address;
    pDesc  = USBHostGetDeviceDescriptor(address);
    pDesc += 8;
    gc_DevData.ID.vid  =  (WORD)*pDesc;       pDesc++;
    gc_DevData.ID.vid |= ((WORD)*pDesc) << 8; pDesc++;
    gc_DevData.ID.pid  =  (WORD)*pDesc;       pDesc++;
    gc_DevData.ID.pid |= ((WORD)*pDesc) << 8; pDesc++;

    // Save the Client Driver ID
    gc_DevData.clientDriverID = clientDriverID;
    
    #ifdef USBHOSTBT_DEBUG
        SIOPrintString( "GEN: USB Generic Client Initalized: flags=0x" );
        SIOPutHex(      flags );
        SIOPrintString( " address=" );
        SIOPutDec( address );
        SIOPrintString( " VID=0x" );
        SIOPutHex(      gc_DevData.ID.vid >> 8   );
        SIOPutHex(      gc_DevData.ID.vid & 0xFF );
        SIOPrintString( " PID=0x"      );
        SIOPutHex(      gc_DevData.ID.pid >> 8   );
        SIOPutHex(      gc_DevData.ID.pid & 0xFF );
        SIOPrintString( "\r\n"         );
    #endif
	
    // Generic Client Driver Init Complete.
    gc_DevData.flags.initialized = 1;

    // Notify that application that we've been attached to a device.
    USB_HOST_APP_EVENT_HANDLER(address, EVENT_BLUETOOTH_ATTACH, &(gc_DevData.ID), sizeof(BLUETOOTH_DEVICE_ID) );

    return TRUE;

} // USBHostBluetoothInit

BOOL USBHostBluetoothEventHandler ( BYTE address, USB_EVENT event, void *data, DWORD size )
{
    // Make sure it was for our device
    if ( address != gc_DevData.ID.deviceAddress)
    {
        return FALSE;
    }

    // Handle specific events.
    switch (event)
    {
        case EVENT_DETACH:
            // Notify that application that the device has been detached.
            USB_HOST_APP_EVENT_HANDLER(gc_DevData.ID.deviceAddress, EVENT_BLUETOOTH_DETACH, &gc_DevData.ID.deviceAddress, sizeof(BYTE) );
            gc_DevData.flags.val        = 0;
            gc_DevData.ID.deviceAddress = 0;
            #ifdef USBHOSTBT_DEBUG
                SIOPrintString( "USB Host Bluetooth Device Detached: address=" );
                SIOPutDec( address );
                SIOPrintString( "\r\n" );
            #endif
            return TRUE;

        case EVENT_TRANSFER:
            if ( (data != NULL) && (size == sizeof(HOST_TRANSFER_DATA)) )
            {
                DWORD dataCount = ((HOST_TRANSFER_DATA *)data)->dataCount;

                if ( ((HOST_TRANSFER_DATA *)data)->bEndpointAddress == (USB_IN_EP|USB_EP1) )	//GVG
                {
                    //SIOPrintString( "E\n" );
                    gc_DevData.flags.rxEvtBusy = 0;
                    gc_DevData.rxEvtLength = dataCount;
                    if(!dataCount) return FALSE;
                    USB_HOST_APP_EVENT_HANDLER(gc_DevData.ID.deviceAddress, EVENT_BLUETOOTH_RX1_DONE, &dataCount, sizeof(DWORD) );
                }
                else if ( ((HOST_TRANSFER_DATA *)data)->bEndpointAddress == (USB_IN_EP|USB_EP2) )	//GVG
                {
                    //if(!dataCount) return FALSE;
                    gc_DevData.flags.rxAclBusy = 0;
                    gc_DevData.rxAclLength = dataCount;
                    USB_HOST_APP_EVENT_HANDLER(gc_DevData.ID.deviceAddress, EVENT_BLUETOOTH_RX2_DONE, &dataCount, sizeof(DWORD) );
                }
                else if ( ((HOST_TRANSFER_DATA *)data)->bEndpointAddress == (USB_OUT_EP|USB_EP2) )	//GVG
                {
                    gc_DevData.flags.txAclBusy = 0;
                    USB_HOST_APP_EVENT_HANDLER(gc_DevData.ID.deviceAddress, EVENT_BLUETOOTH_TX2_DONE, &dataCount, sizeof(DWORD) );
                }
                else
                {
                    return FALSE;
                }

                return TRUE;

            }
            return FALSE;

        case EVENT_SUSPEND:
        case EVENT_RESUME:
        case EVENT_BUS_ERROR:
        default:
            break;
    }

    return FALSE;
} // USBHostBluetoothEventHandler

// *****************************************************************************
// *****************************************************************************
// Section: Application Callable Functions
// *****************************************************************************
// *****************************************************************************

/* GVG: All the USB Generic API has been changed in order to support the USB 
HCI implementation. Basically to add a second EP */


/*
BOOL USBHostBluetoothDeviceDetached( BYTE deviceAddress )
	Implemented as a macro. See usb_host_bluetooth.h
*/

/*
DWORD USBHostBluetoothGetRxLength( BYTE deviceAddress )
	Implemented as a macro. See usb_host_bluetooth.h
*/

BYTE USBHostBluetoothRead_EP1( BYTE deviceAddress, void *buffer, DWORD length )
{
    BYTE bRetVal;

    // Validate the call
    if (!API_VALID( deviceAddress)) return USB_INVALID_STATE;
    if ( gc_DevData.flags.rxEvtBusy)   return USB_BUSY;

    // Set the busy flag, clear the count and start a new IN transfer.
    gc_DevData.flags.rxEvtBusy = 1;
    gc_DevData.rxEvtLength = 0;
    bRetVal = USBHostRead( deviceAddress, USB_IN_EP|USB_EP1, (BYTE *)buffer, length );
    if (bRetVal != USB_SUCCESS)
    {
        gc_DevData.flags.rxEvtBusy = 0;    // Clear flag to allow re-try
    }
    return bRetVal;

} // USBHostBluetoothRead_EP1

BYTE USBHostBluetoothRead_EP2( BYTE deviceAddress, void *buffer, DWORD length )
{
    BYTE RetVal;

    // Validate the call
    if (!API_VALID(deviceAddress)) return USB_INVALID_STATE;
    if ( gc_DevData.flags.rxAclBusy)   return USB_BUSY;

    // Set the busy flag, clear the count and start a new IN transfer.
    gc_DevData.flags.rxAclBusy = 1;
    gc_DevData.rxAclLength = 0;
    RetVal = USBHostRead( deviceAddress, USB_IN_EP|USB_EP2, (BYTE *)buffer, length );
    if (RetVal != USB_SUCCESS)
    {
        gc_DevData.flags.rxAclBusy = 0;    // Clear flag to allow re-try
    }

    return RetVal;

} // USBHostBluetoothRead_EP2


BOOL USBHostBluetoothRx1IsBusy( BYTE deviceAddress )
{
    if (!API_VALID( deviceAddress)) return FALSE;
    if (gc_DevData.flags.rxEvtBusy) return TRUE;
    return FALSE;
}

BOOL USBHostBluetoothRx2IsBusy( BYTE deviceAddress )
{
    if (!API_VALID( deviceAddress)) return FALSE;
    if (gc_DevData.flags.rxAclBusy) return TRUE;
    return FALSE;
}
  
BYTE USBHostBluetoothWrite_EP0( BYTE deviceAddress, void *buffer, DWORD length )
{
    BYTE RetVal;

    // Validate the call
    if (!API_VALID( deviceAddress)) return USB_INVALID_STATE;
    if (gc_DevData.flags.txCtlBusy)   return USB_BUSY;

    // Set the busy flag and start a new OUT transfer.
    gc_DevData.flags.txCtlBusy = 1;
    RetVal = USBHostIssueDeviceRequest( deviceAddress, USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE, 0, 0, 0, length, (BYTE *)buffer, USB_DEVICE_REQUEST_SET,0x00);
    if (RetVal != USB_SUCCESS)
    {
        gc_DevData.flags.txCtlBusy = 0;    // Clear flag to allow re-try
        DelayMs(1);
    }

    return RetVal;

} // USBHostBluetoothWrite
  
/* GVG: Dummy function, the BT Host NEVER writes on EP1  */
BYTE USBHostBluetoothWrite_EP1( BYTE deviceAddress, void *buffer, DWORD length )
{
    BYTE RetVal;

    // Validate the call
    if (!API_VALID(deviceAddress)) return USB_INVALID_STATE;
    if (gc_DevData.flags.txAclBusy)   return USB_BUSY;

    // Set the busy flag and start a new OUT transfer.
    gc_DevData.flags.txAclBusy = 1;
    RetVal = USBHostWrite( deviceAddress, USB_OUT_EP|USB_EP1, (BYTE *)buffer, length );
    if (RetVal != USB_SUCCESS)
    {
        gc_DevData.flags.txAclBusy = 0;    // Clear flag to allow re-try
        DelayMs(1);
    }

    return RetVal;

} // USBHostBluetoothWrite


BYTE USBHostBluetoothWrite_EP2( BYTE deviceAddress, void *buffer, DWORD length )
{
    BYTE RetVal;

    // Validate the call
    if (!API_VALID(deviceAddress)) return USB_INVALID_STATE;
    if (gc_DevData.flags.txAclBusy)   return USB_BUSY;

    // Set the busy flag and start a new OUT transfer.
    gc_DevData.flags.txAclBusy = 1;
    RetVal = USBHostWrite( deviceAddress, USB_OUT_EP|USB_EP2, (BYTE *)buffer, length );
    if (RetVal != USB_SUCCESS)
    {
        gc_DevData.flags.txAclBusy = 0;    // Clear flag to allow re-try
        DelayMs(1);
    }

    return RetVal;

} // USBHostBluetoothWrite

BYTE USBHostBluetoothGetDeviceAddress(){
	return gc_DevData.ID.deviceAddress;
}

