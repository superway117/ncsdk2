UsbVsc

Supported Platform
===================
MyriadX - This example works on MyriadX silicon.

Overview
==========
Vendor specific independent read/write API example.

Software description
=======================

This application uses the Leon OS. Basically, these are the few steps made during the application:
    1. Start the USB DataPump on LeonOS
    2. General configurations of the board
    3. Semaphores are created and configured to sync between USB Read/Write threads and internal USB thread
    4. Read/Write threads are created, which call VSC API functions for bulk transfers

Hardware needed
==================
MyriadX - This software should run on MV235 board.

Build
==================
Please type "make help" if you want to learn available targets.

To build the project please type:
"make clean"
"make all"

Setup
==================
MyriadX silicon - To run the application:
    - connect a USB3.0 or USB2.0 cable from MV235 board to the a PC
    - open terminal and type make "start_server"
    - open another terminal and type "make debug"
    - open vscHost application on the host - at this point the transfers should start

Expected output
==================

PIPE:LOS:
PIPE:LOS: RTEMS POSIX Started
PIPE:LOS: +UsbPumpRtems_UsbPumpInit_ServiceTask:
PIPE:LOS: Write thread created
PIPE:LOS: Read thread created
PIPE:LOS: Starting write transfer [ep 1] 1048576 bytes
PIPE:LOS: Starting read transfer [ep 1] 1048576 bytes
...
PIPE:LOS: Starting write transfer [ep 2] 1048576 bytes
PIPE:LOS: Starting read transfer [ep 2] 1048576 bytes
...
PIPE:LOS: Starting write transfer [ep 3] 1048576 bytes
PIPE:LOS: Starting read transfer [ep 3] 1048576 bytes
...

User interaction
==================
The user should send and read data from the MV235 board using a PC as host.
The host application should send and request bulk transfers, corresponding to what is set on MyriadX.

The user has the choice of adding and changing the EndPoints used for transfers, the setup to do this is the following:
1. Define the bulk in/out endpoints in the vsc2app_device.urc file, in the following structure:
    #########################################
    # Vendor specific interface #
    #########################################
    interface 0
      {
      alternate-setting 0
        class  0xFF # vendor specific class
        subclass 0x00 #
        protocol 0x00 #
        name S_BULKDATA # string
        endpoints
          bulk out #first  OUT EndPoint  (0)
          bulk out #second OUT EndPoint  (1)
          bulk in  #first  IN  EndPoint  (0)
        ;
      }

2. Set the corresponding number of bulk IN/OUT endpoints in the usbpump_vsc2app.h file using the following defines:
    #define USBPUMP_VSC2APP_NUM_EP_OUT 2
    #define USBPUMP_VSC2APP_NUM_EP_IN 1

3. Call the following VSC API with the desired EndPoint (first EndPoint is considered 0) :
VOID
UsbVscAppRead(USBPUMP_VSC2APP_CONTEXT * pSelf, __TMS_UINT32 size, __TMS_CHAR * buff, __TMS_UINT32 endPoint);

VOID
UsbVscAppWrite(USBPUMP_VSC2APP_CONTEXT * pSelf, __TMS_UINT32 size, __TMS_CHAR * buff, __TMS_UINT32 endPoint);

