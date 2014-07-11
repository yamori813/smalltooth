#include <stdlib.h>
#include "GenericTypeDefs.h"
#include "HardwareProfile.h"
#include "PIC32_USB/usb_host_bluetooth.h"
#include "BTApp.h"
#include "xprintf.h"
#include "debug.h"

// *****************************************************************************
// *****************************************************************************
// Configuration Bits
// *****************************************************************************
// *****************************************************************************

#if defined( __PIC32MX__ )

    #pragma config UPLLEN   = ON            // USB PLL Enabled
    #pragma config FPLLMUL  = MUL_15        // PLL Multiplier
    #pragma config UPLLIDIV = DIV_2         // USB PLL Input Divider
    #pragma config FPLLIDIV = DIV_2         // PLL Input Divider
    #pragma config FPLLODIV = DIV_1         // PLL Output Divider
    #pragma config FPBDIV   = DIV_1         // Peripheral Clock divisor
    #pragma config FWDTEN   = OFF           // Watchdog Timer
    #pragma config WDTPS    = PS1           // Watchdog Timer Postscale
    #pragma config FCKSM    = CSDCMD        // Clock Switching & Fail Safe Clock Monitor
    #pragma config OSCIOFNC = OFF           // CLKO Enable
    #pragma config POSCMOD  = HS            // Primary Oscillator
    #pragma config IESO     = OFF           // Internal/External Switch-over
    #pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable (KLO was off)
    #pragma config FNOSC    = PRIPLL        // Oscillator Selection
    #pragma config CP       = OFF           // Code Protect
    #pragma config BWP      = OFF           // Boot Flash Write Protect
    #pragma config PWP      = OFF           // Program Flash Write Protect
    #pragma config ICESEL   = ICS_PGx2      // ICE/ICD Comm Channel Select
    #pragma config DEBUG    = ON            // Background Debugger Enable

#else

    #error Cannot define configuration bits.

#endif

BT_DEVICE *gpsBTAPP = NULL;

DEBUG_PutChar() {}
DEBUG_PutString(char *ptr) {}
DEBUG_PutHexUINT8() {}

SIOPrintString(char *ptr, ...)
{
	UART1PrintString(ptr);
}
exit() {}

/*INITIALIZES THE SYSTEM*/
void SysInit(){
    #if defined(__PIC32MX__)
        {
            int  value;
            value = SYSTEMConfigWaitStatesAndPB( GetSystemClock() );
            // Enable the cache for the best performance
            CheKseg0CacheOn();
            INTEnableSystemMultiVectoredInt();
            value = OSCCON;
            while (!(value & 0x00000020))
            {
                value = OSCCON;    // Wait for PLL lock to stabilize
            }
        }
    #endif //__PIC32MX__

    // Set LED Pins to Outputs
    PORTSetPinsDigitalOut(IOPORT_B, BIT_15);

    mInitAllSwitches();

    // Enable change notice, enable discrete pins and weak pullups
//    mCNOpen(CONFIG, PINS, PULLUPS);
//    mPORTDRead();
    // Turn All Led's Off
//    mSetAllLedsOFF();
    // Init UART
    SIOInit();
}

/*APPLICATION USB EVENT HANDLER*/
BOOL USB_ApplicationEventHandler ( BYTE address, USB_EVENT event, void *data, DWORD size )
{
    BYTE *pBuff = NULL;
    // Handle specific events.
    switch (event)
    {
        case EVENT_BLUETOOTH_ATTACH:
            if (size == sizeof(BLUETOOTH_DEVICE_ID))
            {
                ((BLUETOOTH_DEVICE_ID *)data)->deviceAddress = address;
                DBG_INFO("Generic device connected, deviceAddress=%d\n", address);
                BTAPP_Start(gpsBTAPP);
                return TRUE;
            }
            break;

        case EVENT_BLUETOOTH_DETACH:
            DBG_INFO("USB device disconnected\n");
            *(BYTE *)data   = 0;
            return TRUE;

        case EVENT_BLUETOOTH_TX2_DONE:
            return TRUE;

        case EVENT_BLUETOOTH_RX1_DONE:
            if(NULL != data)
            {
                pBuff = gpsBTAPP->sUSB.getEVTBuff();
                gpsBTAPP->sUSB.readEVT(pBuff, (int)*(DWORD*)data);
            }
            return TRUE;

        case EVENT_BLUETOOTH_RX2_DONE:
            if(NULL != data)
            {
               pBuff = gpsBTAPP->sUSB.getACLBuff();
               gpsBTAPP->sUSB.readACL(pBuff, (int)*(DWORD*)data);
            }
            return TRUE;

        case EVENT_VBUS_REQUEST_POWER:
            // We'll let anything attach.
            return TRUE;

        case EVENT_VBUS_RELEASE_POWER:
            // We aren't keeping track of power.
            return TRUE;

        case EVENT_HUB_ATTACH:
            DBG_ERROR("%s:%d ERROR: HUB devices not supported.\n", __FILE__, __LINE__);
            return TRUE;
            break;

        case EVENT_UNSUPPORTED_DEVICE:
            DBG_ERROR("%s:%d ERROR: Unsupported device.\n", __FILE__, __LINE__);
            return TRUE;
            break;

        case EVENT_CANNOT_ENUMERATE:
            DBG_ERROR("%s:%d ERROR: Cannot enumerate device.\n", __FILE__, __LINE__);
            return TRUE;
            break;

        case EVENT_CLIENT_INIT_ERROR:
            DBG_ERROR("%s:%d ERROR: Client init error.\n", __FILE__, __LINE__);
            return TRUE;
            break;

        case EVENT_OUT_OF_MEMORY:
            DBG_ERROR("%s:%d ERROR: Out of heap memory.\n", __FILE__, __LINE__);
            return TRUE;
            break;

        case EVENT_UNSPECIFIED_ERROR:   // This should never be generated.
            DBG_ERROR("%s:%d ERROR: Unspecified error.\n", __FILE__, __LINE__);
            return TRUE;
            break;

        case EVENT_SUSPEND:
        case EVENT_DETACH:
        case EVENT_RESUME:
        case EVENT_BUS_ERROR:
            return TRUE;
            break;

        default:
            break;
    }

    return FALSE;

} // USB_ApplicationEventHandler

/*Scans the USB Port*/
void USBScan()
{
    BYTE bDevAddr = USBHostBluetoothGetDeviceAddress();
    BYTE *pBuff = NULL;
    if (bDevAddr != 0)
    {
        if(!USBHostBluetoothRx1IsBusy(bDevAddr))
        {
            //Scan the EP1 (Event data)
            pBuff = gpsBTAPP->sUSB.getEVTBuff();
            USBHostBluetoothRead_EP1(bDevAddr,pBuff, EVENT_PACKET_LENGTH );
	}
        else if(!USBHostBluetoothRx2IsBusy(bDevAddr))
        {
            //Scan the EP2 (ACL data)
            pBuff = gpsBTAPP->sUSB.getACLBuff();
            USBHostBluetoothRead_EP2(bDevAddr,pBuff, DATA_PACKET_LENGTH );
	}
    }
}


int main ( void )
{
        unsigned int last_sw_state = 1;
        unsigned int last_sw2_state = 1;
    //Initialise the system
    SysInit();
	
	xfunc_out=UART1PutChar;

    //Initialise the USB Host
    if ( USBHostInit(0) == TRUE )
    {
        DBG_INFO( "USB initialized.\n" );
    }
    else
    {
        DBG_INFO( "Unable to initialise the USB: HALT\n" );
    }

    DelayMs(100);
    BTAPP_Initialise(&gpsBTAPP);
    DBG_INFO( "USB-Bluetooth Dongle Demo v1\n" );
    //Main loop
    while (1)
    {
        if(PORTBbits.RB7 == 0)					// 0 = switch is pressed
        {
            PORTSetBits(IOPORT_B, BIT_15);			// RED LED = on (same as LATDSET = 0x0001)
            if(last_sw_state == 1)					// display a message only when switch changes state
            {
                gpsBTAPP->SPPsendData("SW1 PRESSED ", 12);
                last_sw_state = 0;
            }
        }
        else										// 1 = switch is not pressed
        {
            PORTClearBits(IOPORT_B, BIT_15);			// RED LED = off (same as LATDCLR = 0x0001)
            if(last_sw_state == 0)                 // display a message only when switch changes state
            {
                gpsBTAPP->SPPsendData("SW1 RELEASED ", 13);
                last_sw_state = 1;
            }
        }

#if 0
        if(PORTDbits.RD7 == 0)					// 0 = switch is pressed
        {
            PORTSetBits(IOPORT_D, BIT_1);			// RED LED = on (same as LATDSET = 0x0001)
            if(last_sw2_state == 1)					// display a message only when switch changes state
            {
                DBPRINTF("Switch SW2 has been pressed. \n");
                gpsBTAPP->SPPdisconnect(RFCOMM_CH_DATA);
                gpsBTAPP->SPPdisconnect(RFCOMM_CH_MUX);
                gpsBTAPP->L2CAPdisconnect(L2CAP_RFCOMM_PSM);
                last_sw2_state = 0;
            }
        }
        else										// 1 = switch is not pressed
        {
            PORTClearBits(IOPORT_D, BIT_1);			// RED LED = off (same as LATDCLR = 0x0001)
            if(last_sw2_state == 0)                 // display a message only when switch changes state
            {
                DBPRINTF("Switch SW2 has been released. \n");
                last_sw2_state = 1;
            }
        }
#endif

        //Maintain the USB status
        USBHostTasks();
        //Maintain the application
        USBScan();
    }
}
