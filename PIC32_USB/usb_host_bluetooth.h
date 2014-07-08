#ifndef __USBHostBluetooth_H__
#define __USBHostBluetooth_H__
#include "usb_config.h"
#include "usb_common.h"
#include "usb_host.h"

// *****************************************************************************
// *****************************************************************************
// Section: USB Generic Client Events
// *****************************************************************************
// *****************************************************************************

        // This is an optional offset for the values of the generated events.
        // If necessary, the application can use a non-zero offset for the
        // generic events to resolve conflicts in event number.
#ifndef EVENT_BLUETOOTH_OFFSET
#define EVENT_BLUETOOTH_OFFSET 0
#endif

        // This event indicates that a Generic device has been attached.
        // When USB_HOST_APP_EVENT_HANDLER is called with this event, *data
        // points to a BLUETOOTH_DEVICE_ID structure, and size is the size of the
        // BLUETOOTH_DEVICE_ID structure.
#define EVENT_BLUETOOTH_ATTACH  (EVENT_GENERIC_BASE+EVENT_BLUETOOTH_OFFSET+0)

        // This event indicates that the specified device has been detached
        // from the USB.  When USB_HOST_APP_EVENT_HANDLER is called with this
        // event, *data points to a BYTE that contains the device address, and
        // size is the size of a BYTE.
#define EVENT_BLUETOOTH_DETACH  (EVENT_GENERIC_BASE+EVENT_BLUETOOTH_OFFSET+1)

        // This event indicates that a previous write request has completed.
        // These events are enabled if USB Embedded Host transfer events are
        // enabled (USB_ENABLE_TRANSFER_EVENT is defined).  When
        // USB_HOST_APP_EVENT_HANDLER is called with this event, *data points
        // to the buffer that completed transmission, and size is the actual
        // number of bytes that were written to the device.
#define EVENT_BLUETOOTH_TX2_DONE (EVENT_GENERIC_BASE+EVENT_BLUETOOTH_OFFSET+2)

        // This event indicates that a previous read request has completed.
        // These events are enabled if USB Embedded Host transfer events are
        // enabled (USB_ENABLE_TRANSFER_EVENT is defined).  When
        // USB_HOST_APP_EVENT_HANDLER is called with this event, *data points
        // to the receive buffer, and size is the actual number of bytes read
        // from the device.
#define EVENT_BLUETOOTH_RX1_DONE (EVENT_GENERIC_BASE+EVENT_BLUETOOTH_OFFSET+3)	
#define EVENT_BLUETOOTH_RX2_DONE (EVENT_GENERIC_BASE+EVENT_BLUETOOTH_OFFSET+4)

// *****************************************************************************
/* Generic Device ID Information

This structure contains identification information about an attached device.
*/
typedef struct _BLUETOOTH_DEVICE_ID
{
    WORD        vid;                    // Vendor ID of the device
    WORD        pid;                    // Product ID of the device
    BYTE        deviceAddress;          // Address of the device on the USB
} BLUETOOTH_DEVICE_ID;


// *****************************************************************************
/* Generic Device Information

This structure contains information about an attached device, including
status flags and device identification.
*/
typedef struct _BLUETOOTH_DEVICE
{
    BLUETOOTH_DEVICE_ID   ID;             // Identification information about the device
    DWORD               rxEvtLength;		// GVG: Number of bytes received in the last IN transfer
    DWORD				rxAclLength;		// GVG: Number of bytes received in the last IN transfer
    BYTE                clientDriverID; // ID to send when issuing a Device Request

    union
    {
        BYTE val;                       // BYTE representation of device status flags
        struct
        {
            BYTE initialized            : 1;    // Driver has been initialized
            BYTE txCtlBusy		: 1;
            BYTE txAclBusy		: 1;    // Driver busy transmitting data
            BYTE rxEvtBusy		: 1;    // GVG: Driver busy receiving data
            BYTE rxAclBusy		: 1;    // GVG: Driver busy receiving data
        };
    } flags;                            // Generic client driver status flags

} BLUETOOTH_DEVICE;

BOOL USBHostBluetoothInit ( BYTE address, DWORD flags, BYTE clientDriverID );
BOOL USBHostBluetoothEventHandler ( BYTE address, USB_EVENT event, void *data, DWORD size );

//BOOL USB_ApplicationEventHandler ( BYTE address, USB_EVENT event, void *data, DWORD size );

#define API_VALID(a) ( (((a)==gc_DevData.ID.deviceAddress) && gc_DevData.flags.initialized == 1) ? TRUE : FALSE )

#define USBHostBluetoothDeviceDetached(a) ( (((a)==gc_DevData.ID.deviceAddress) && gc_DevData.flags.initialized == 1) ? FALSE : TRUE )
//BOOL USBHostBluetoothDeviceDetached( BYTE deviceAddress );

#define USBHostBluetoothGetrxEvtLength(a) ( (API_VALID(a)) ? gc_DevData.rxEvtLength : 0 )
//DWORD USBHostBluetoothGetrxEvtLength( BYTE deviceAddress );
#define USBHostBluetoothGetrxAclLength(a) ( (API_VALID(a)) ? gc_DevData.rxAclLength : 0 )
//DWORD USBHostBluetoothGetRxLength( BYTE deviceAddress );

BYTE USBHostBluetoothRead_EP1( BYTE deviceAddress, void *buffer, DWORD length);
/* Macro Implementation:
#define USBHostBluetoothRead_EP1((a,b,l)                                                 \
        ( API_VALID(gc_DevData.ID.deviceAddress) ? USBHostRead((a),USB_IN_EP|USB_EP1,(BYTE *)(b),(l)) : \
                              USB_INVALID_STATE )
*/
BYTE USBHostBluetoothRead_EP2( BYTE deviceAddress, void *buffer, DWORD length);
/* Macro Implementation:
#define USBHostBluetoothRead_EP2((a,b,l)                                                 \
        ( API_VALID(gc_DevData.ID.deviceAddress) ? USBHostRead((a),USB_IN_EP|USB_EP2,(BYTE *)(b),(l)) : \
                              USB_INVALID_STATE )
*/

//#define USBHostBluetoothRx1IsBusy(a) ( (API_VALID(a)) ? ((gc_DevData.flags.rxEvtBusy == 1) ? TRUE : FALSE) : TRUE )
BOOL USBHostBluetoothRx1IsBusy( BYTE deviceAddress );
//#define USBHostBluetoothRx2IsBusy(a) ( (API_VALID(a)) ? ((gc_DevData.flags.rxAclBusy == 1) ? TRUE : FALSE) : TRUE )
BOOL USBHostBluetoothRx2IsBusy( BYTE deviceAddress );

BOOL USBHostBluetoothRxIsComplete( BYTE deviceAddress,
                                    BYTE *errorCode, DWORD *byteCount );

#define USBHostBluetoothTxIsBusy(a) ( (API_VALID(a)) ? ((gc_DevData.flags.txAclBusy == 1) ? TRUE : FALSE) : TRUE )
//BOOL USBHostBluetoothTxIsBusy( BYTE deviceAddress );

BOOL USBHostBluetoothTxIsComplete( BYTE deviceAddress, BYTE *errorCode );

BYTE USBHostBluetoothWrite_EP0( BYTE deviceAddress, void *buffer, DWORD length);
/* Macro Implementation:
#define USBHostBluetoothWrite_EP0(a,b,l)                                                 \
        ( API_VALID(deviceAddress) ? USBHostWrite((a),USB_EP0,(BYTE *)(b),(l)) : \
                              USB_INVALID_STATE )
*/

/* GVG: Dummy function, the BT Host NEVER writes on EP1  */
BYTE USBHostBluetoothWrite_EP1( BYTE deviceAddress, void *buffer, DWORD length);
/* Macro Implementation:
#define USBHostBluetoothWrite_EP1(a,b,l)                                                 \
        ( API_VALID(deviceAddress) ? USBHostWrite((a),USB_EP1,(BYTE *)(b),(l)) : \
                              USB_INVALID_STATE )
*/

BYTE USBHostBluetoothWrite_EP2( BYTE deviceAddress, void *buffer, DWORD length);
/* Macro Implementation:
#define USBHostBluetoothWrite_EP2(a,b,l)                                                 \
        ( API_VALID(deviceAddress) ? USBHostWrite((a),USB_EP2,(BYTE *)(b),(l)) : \
                              USB_INVALID_STATE )
*/

BYTE USBHostBluetoothGetDeviceAddress();

#endif //#ifndef __USBHostBluetooth_H__
