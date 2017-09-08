#include "usb.h"
#include "usb_device_cdc.h"

// TODO(jim): Verify that this is ending up in romdata.

const USB_DEVICE_DESCRIPTOR device_dsc = {
    0x12,                          // Size of this descriptor in bytes
    USB_DESCRIPTOR_DEVICE,         // DEVICE descriptor type
    0x0200,                        // USB Spec Release Number in BCD format
    CDC_DEVICE,                    // Class Code
    0x00,                          // Subclass code
    0x00,                          // Protocol code
    USB_EP0_BUFF_SIZE,             // Max packet size for EP0, see usb_config.h
    0x2a19,                        // Vendor ID (Numato)
    0x1002,                        // Product ID (Numato Lab Mimas V2 Spartan 3A Development Board)
    0x0100,                        // Device release number in BCD format
    0x01,                          // Manufacturer string index
    0x02,                          // Product string index
    0x00,                          // Device serial number string index
    0x01                           // Number of possible configurations
};

const uint8_t configDescriptor1[] = {
    // ----- Configuration descriptor ------------------------------------------
    0x09,                          // Size of this descriptor in bytes
    USB_DESCRIPTOR_CONFIGURATION,  // CONFIGURATION descriptor type
    9+58*CDC_NUM_PORTS,0,                      // Total length of data for this cfg
    USB_MAX_NUM_INT,               // Number of interfaces in this cfg
    1,                             // Index value of this configuration
    0,                             // Configuration string index
    _DEFAULT | _SELF,              // Attributes, see usb_device.h
    50,                            // Max power consumption (2X mA)

    // ---- CDC1 - comm interface ----------------------------------------------
    9,                             // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,      // INTERFACE descriptor type
    0,                             // Interface Number                       ***
    0,                             // Alternate Setting Number
    1,                             // Number of endpoints in this intf
    COMM_INTF,                     // Class code
    ABSTRACT_CONTROL_MODEL,        // Subclass code
    V25TER,                        // Protocol code
    0x03,                          // Interface string index                 ***

    // ----- CDC1 - class-specific descriptors ---------------------------------
    sizeof(USB_CDC_HEADER_FN_DSC),
    CS_INTERFACE,
    DSC_FN_HEADER,
    0x10,0x01,

    sizeof(USB_CDC_ACM_FN_DSC),
    CS_INTERFACE,
    DSC_FN_ACM,
    USB_CDC_ACM_FN_DSC_VAL,

    sizeof(USB_CDC_UNION_FN_DSC),
    CS_INTERFACE,
    DSC_FN_UNION,
    0,                             // Interface ID of the comm interface.    ***
    1,                             // Interface ID of the data interface.    ***

    sizeof(USB_CDC_CALL_MGT_FN_DSC),
    CS_INTERFACE,
    DSC_FN_CALL_MGT,
    0x00,
    1,                             // Interface ID of the data interface.    ***

    // ----- CDC1 - comm endpoint ----------------------------------------------
    0x07,                          // Size of this descriptor in bytes
    USB_DESCRIPTOR_ENDPOINT,       // Endpoint Descriptor
    _EP01_IN,                      // EndpointAddress                        ***
    _INTERRUPT,                    // Attributes
    0x08,0x00,                     // Max tx/rx size
    0x02,                          // Interval

    // ----- CDC1 - data interface ---------------------------------------------
    0x09,                          // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,      // INTERFACE descriptor type
    1,                             // Interface Number                       ***
    0,                             // Alternate Setting Number
    2,                             // Number of endpoints in this intf
    DATA_INTF,                     // Class code
    0,                             // Subclass code
    NO_PROTOCOL,                   // Protocol code
    0x03,                          // Interface string index                 ***

    // ----- CDC1 - data out endpoint ------------------------------------------
    0x07,                          // Size of this descriptor in bytes
    USB_DESCRIPTOR_ENDPOINT,       // Endpoint Descriptor
    _EP02_OUT,                     // EndpointAddress                        ***
    _BULK,                         // Attributes
    CDC_DATA_OUT_EP_SIZE,0x00,     // Max tx/rx size
    0x00,                          // Interval

    // ----- CDC1 - data in endpoint ---0---------------------------------------
    0x07,                          // Size of this descriptor in bytes
    USB_DESCRIPTOR_ENDPOINT,       // Endpoint Descriptor
    _EP02_IN,                      // EndpointAddress                        ***
    _BULK,                         // Attributes
    CDC_DATA_IN_EP_SIZE,0x00,      // Max tx/rx size
    0x00,                          // Interval
#if CDC_NUM_PORTS > 1
    // ---- CDC2 - comm interface ----------------------------------------------
    9,                             // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,      // INTERFACE descriptor type
    2,                             // Interface Number                       ***
    0,                             // Alternate Setting Number
    1,                             // Number of endpoints in this intf
    COMM_INTF,                     // Class code
    ABSTRACT_CONTROL_MODEL,        // Subclass code
    V25TER,                        // Protocol code
    0x04,                          // Interface string index                 ***

    // ----- CDC2 - class-specific descriptors ---------------------------------
    sizeof(USB_CDC_HEADER_FN_DSC),
    CS_INTERFACE,
    DSC_FN_HEADER,
    0x10,0x01,

    sizeof(USB_CDC_ACM_FN_DSC),
    CS_INTERFACE,
    DSC_FN_ACM,
    USB_CDC_ACM_FN_DSC_VAL,

    sizeof(USB_CDC_UNION_FN_DSC),
    CS_INTERFACE,
    DSC_FN_UNION,
    2,                             // Interface ID of the comm interface.    ***
    3,                             // Interface ID of the data interface.    ***

    sizeof(USB_CDC_CALL_MGT_FN_DSC),
    CS_INTERFACE,
    DSC_FN_CALL_MGT,
    0x00,
    3,                             // Interface ID of the data interface.

    // ----- CDC2 - comm endpoint ----------------------------------------------
    0x07,                          // Size of this descriptor in bytes
    USB_DESCRIPTOR_ENDPOINT,       // Endpoint Descriptor
    _EP03_IN,                      // EndpointAddress                        ***
    _INTERRUPT,                    // Attributes
    0x08,0x00,                     // Max tx/rx size
    0x02,                          // Interval

    // ----- CDC2 - data interface ---------------------------------------------
    0x09,                          // Size of this descriptor in bytes
    USB_DESCRIPTOR_INTERFACE,      // INTERFACE descriptor type
    3,                             // Interface Number                       ***
    0,                             // Alternate Setting Number
    2,                             // Number of endpoints in this intf
    DATA_INTF,                     // Class code
    0,                             // Subclass code
    NO_PROTOCOL,                   // Protocol code
    0x04,                          // Interface string index                 ***

    // ----- CDC2 - data out endpoint ------------------------------------------
    0x07,                          // Size of this descriptor in bytes
    USB_DESCRIPTOR_ENDPOINT,       // Endpoint Descriptor
    _EP04_OUT,                     // EndpointAddress                        ***
    _BULK,                         // Attributes
    CDC_DATA_OUT_EP_SIZE,0x00,     // Max tx/rx size
    0x00,                          // Interval

    // ----- CDC2 - data in endpoint ---0---------------------------------------
    0x07,                          // Size of this descriptor in bytes
    USB_DESCRIPTOR_ENDPOINT,       // Endpoint Descriptor
    _EP04_IN,                      // EndpointAddress                        ***
    _BULK,                         // Attributes
    CDC_DATA_IN_EP_SIZE,0x00,      // Max tx/rx size
    0x00,                          // Interval
#endif
};

// Language code string descriptor
const struct {
    uint8_t bLength;
    uint8_t bDscType;
    uint16_t string[1];
} sd000 = {
    sizeof(sd000),
    USB_DESCRIPTOR_STRING,
    {
        0x0409
    }
};

// Manufacturer string descriptor
const struct {
    uint8_t bLength;
    uint8_t bDscType;
    uint16_t string[14];
} sd001 = {
    sizeof(sd001),
    USB_DESCRIPTOR_STRING,
    {
        'N','u','m','a','t','o',' ','S','y','s','t','e','m','s'
    }
};

// Product string descriptor
const struct {
    uint8_t bLength;
    uint8_t bDscType;
    uint16_t string[8];
} sd002 = {
    sizeof(sd002),
    USB_DESCRIPTOR_STRING,
    {
        'M','i','m','a','s',' ','V','2'
    }
};

// CDC1 descriptor
const struct {
    uint8_t bLength;
    uint8_t bDscType;
    uint16_t string[9];
} sd003 = {
    sizeof(sd003),
    USB_DESCRIPTOR_STRING,
    {
        'S','P','I',' ','F','l','a','s','h'
    }
};

// CDC2 descriptor
const struct {
    uint8_t bLength;
    uint8_t bDscType;
    uint16_t string[9];
} sd004 = {
    sizeof(sd004),
    USB_DESCRIPTOR_STRING,
    {
        'F','P','G','A',' ','U','A','R','T'
    }
};

// Array of configuration descriptors
const uint8_t *const USB_CD_Ptr[] = {
    (const uint8_t *const)configDescriptor1
};

// Array of string descriptors
const uint8_t *const USB_SD_Ptr[USB_NUM_STRING_DESCRIPTORS] = {
    (const uint8_t *const)&sd000,
    (const uint8_t *const)&sd001,
    (const uint8_t *const)&sd002,
    (const uint8_t *const)&sd003,
    (const uint8_t *const)&sd004,
};
