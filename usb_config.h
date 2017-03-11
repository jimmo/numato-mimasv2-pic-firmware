#ifndef USBCFG_H
#define USBCFG_H

#define USB_EP0_BUFF_SIZE		8	// Valid Options: 8, 16, 32, or 64 bytes.

#define USB_MAX_NUM_INT     	4   //Set this number to match the maximum interface number used in the descriptors for this firmware project
#define USB_MAX_EP_NUMBER	    4   //Set this number to match the maximum endpoint number used in the descriptors for this firmware project

#define USB_USER_DEVICE_DESCRIPTOR &device_dsc
#define USB_USER_DEVICE_DESCRIPTOR_INCLUDE extern const USB_DEVICE_DESCRIPTOR device_dsc

#define USB_USER_CONFIG_DESCRIPTOR USB_CD_Ptr
#define USB_USER_CONFIG_DESCRIPTOR_INCLUDE extern const uint8_t *const USB_CD_Ptr[]

#define USB_PING_PONG_MODE USB_PING_PONG__FULL_PING_PONG    //A good all around setting

//#define USB_POLLING
#define USB_INTERRUPT

#define USB_PULLUP_OPTION USB_PULLUP_ENABLE

#define USB_TRANSCEIVER_OPTION USB_INTERNAL_TRANSCEIVER

#define USB_SPEED_OPTION USB_FULL_SPEED

#define USB_ENABLE_STATUS_STAGE_TIMEOUTS
#define USB_STATUS_STAGE_TIMEOUT     (uint8_t)45

#define USB_SUPPORT_DEVICE

#define USB_NUM_STRING_DESCRIPTORS 5  //Set this number to match the total number of string descriptors that are implemented in the usb_descriptors.c file

// Enable all events.
//#define USB_DISABLE_SUSPEND_HANDLER
//#define USB_DISABLE_WAKEUP_FROM_SUSPEND_HANDLER
//#define USB_DISABLE_SOF_HANDLER
//#define USB_DISABLE_TRANSFER_TERMINATED_HANDLER
//#define USB_DISABLE_ERROR_HANDLER
//#define USB_DISABLE_NONSTANDARD_EP0_REQUEST_HANDLER
//#define USB_DISABLE_SET_DESCRIPTOR_HANDLER
//#define USB_DISABLE_SET_CONFIGURATION_HANDLER
//#define USB_DISABLE_TRANSFER_COMPLETE_HANDLER

#define USB_USE_CDC

#define CDC_COMM_IN_EP_SIZE     10

#define CDC_NUM_PORTS           2

// This defines the size of the tx/rx buffers.
#define CDC_DATA_OUT_EP_SIZE    64/CDC_NUM_PORTS
#define CDC_DATA_IN_EP_SIZE     64/CDC_NUM_PORTS

// Support: Send_Break command
//#define USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D2

// Support: Set_Line_Coding, Set_Control_Line_State, Get_Line_Coding, and Serial_State commands
#define USB_CDC_SUPPORT_ABSTRACT_CONTROL_MANAGEMENT_CAPABILITIES_D1

#endif //USBCFG_H
