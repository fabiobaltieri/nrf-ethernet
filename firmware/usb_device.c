#include <ch.h>
#include <hal.h>
#include <serial_usb.h>

#include "board.h"
#include "usb_device.h"

static SerialUSBDriver SDU1;

/*
 * Endpoints to be used for USBD1.
 */
#define USBD1_DATA_REQUEST_EP		1
#define USBD1_DATA_AVAILABLE_EP		1
#define USBD1_INTERRUPT_REQUEST_EP	2

/* Device Descriptor */
static const uint8_t vcom_device_descriptor_data[18] = {
	USB_DESC_DEVICE(0x0110,        /* bcdUSB (1.1).                    */
			0x02,          /* bDeviceClass (CDC).              */
			0x00,          /* bDeviceSubClass.                 */
			0x00,          /* bDeviceProtocol.                 */
			0x40,          /* bMaxPacketSize.                  */
			0x0483,        /* idVendor (ST).                   */
			0x5740,        /* idProduct.                       */
			0x0200,        /* bcdDevice.                       */
			1,             /* iManufacturer.                   */
			2,             /* iProduct.                        */
			0,             /* iSerialNumber.                   */
			1)             /* bNumConfigurations.              */
};

static const USBDescriptor vcom_device_descriptor = {
	sizeof(vcom_device_descriptor_data),
	vcom_device_descriptor_data
};

/* Configuration Descriptor */
static const uint8_t vcom_configuration_descriptor_data[67] = {
	/* Configuration Descriptor */
	USB_DESC_CONFIGURATION(
			67,            /* wTotalLength.                    */
			0x02,          /* bNumInterfaces.                  */
			0x01,          /* bConfigurationValue.             */
			0,             /* iConfiguration.                  */
			0xC0,          /* bmAttributes (self powered).     */
			50),           /* bMaxPower (100mA).               */
	/* Interface Descriptor */
	USB_DESC_INTERFACE(
			0x00,          /* bInterfaceNumber.                */
			0x00,          /* bAlternateSetting.               */
			0x01,          /* bNumEndpoints.                   */
			0x02,          /* bInterfaceClass (Communications
					  Interface Class, CDC section
					  4.2).                            */
			0x02,          /* bInterfaceSubClass (Abstract
					  Control Model, CDC section 4.3). */
			0x01,          /* bInterfaceProtocol (AT commands,
					  CDC section 4.4).                */
			0),            /* iInterface.                      */
	/* Header Functional Descriptor (CDC section 5.2.3).*/
	USB_DESC_BYTE(5),              /* bLength.                         */
	USB_DESC_BYTE(0x24),           /* bDescriptorType (CS_INTERFACE).  */
	USB_DESC_BYTE(0x00),           /* bDescriptorSubtype (Header
					  Functional Descriptor.           */
	USB_DESC_BCD(0x0110),          /* bcdCDC.                          */
	/* Call Management Functional Descriptor. */
	USB_DESC_BYTE(5),              /* bFunctionLength.                 */
	USB_DESC_BYTE(0x24),           /* bDescriptorType (CS_INTERFACE).  */
	USB_DESC_BYTE(0x01),           /* bDescriptorSubtype (Call Management
					  Functional Descriptor).          */
	USB_DESC_BYTE(0x00),           /* bmCapabilities (D0+D1).          */
	USB_DESC_BYTE(0x01),           /* bDataInterface.                  */
	/* ACM Functional Descriptor.*/
	USB_DESC_BYTE(4),              /* bFunctionLength.                 */
	USB_DESC_BYTE(0x24),           /* bDescriptorType (CS_INTERFACE).  */
	USB_DESC_BYTE(0x02),           /* bDescriptorSubtype (Abstract
					  Control Management Descriptor).  */
	USB_DESC_BYTE(0x02),           /* bmCapabilities.                  */
	/* Union Functional Descriptor.*/
	USB_DESC_BYTE(5),              /* bFunctionLength.                 */
	USB_DESC_BYTE(0x24),           /* bDescriptorType (CS_INTERFACE).  */
	USB_DESC_BYTE(0x06),           /* bDescriptorSubtype (Union
					  Functional Descriptor).          */
	USB_DESC_BYTE(0x00),           /* bMasterInterface (Communication
					  Class Interface).                */
	USB_DESC_BYTE(0x01),           /* bSlaveInterface0 (Data Class
					  Interface).                      */
	/* Endpoint 2 Descriptor.*/
	USB_DESC_ENDPOINT(
			USBD1_INTERRUPT_REQUEST_EP | 0x80,
			0x03,          /* bmAttributes (Interrupt).        */
			0x0008,        /* wMaxPacketSize.                  */
			0xFF),         /* bInterval.                       */
	/* Interface Descriptor.*/
	USB_DESC_INTERFACE(
			0x01,          /* bInterfaceNumber.                */
			0x00,          /* bAlternateSetting.               */
			0x02,          /* bNumEndpoints.                   */
			0x0A,          /* bInterfaceClass (Data Class
					  Interface, CDC section 4.5).     */
			0x00,          /* bInterfaceSubClass (CDC section
					  4.6).                            */
			0x00,          /* bInterfaceProtocol (CDC section
					  4.7).                            */
			0x00),         /* iInterface.                      */
	/* Endpoint 3 Descriptor.*/
	USB_DESC_ENDPOINT(
			USBD1_DATA_AVAILABLE_EP,
			0x02,          /* bmAttributes (Bulk).             */
			0x0040,        /* wMaxPacketSize.                  */
			0x00),         /* bInterval.                       */
	/* Endpoint 1 Descriptor.*/
	USB_DESC_ENDPOINT(
			USBD1_DATA_REQUEST_EP | 0x80,
			0x02,          /* bmAttributes (Bulk).             */
			0x0040,        /* wMaxPacketSize.                  */
			0x00)          /* bInterval.                       */
};

static const USBDescriptor vcom_configuration_descriptor = {
	sizeof(vcom_configuration_descriptor_data),
	vcom_configuration_descriptor_data
};

/* Language ID */
static const uint8_t vcom_string0[] = {
	USB_DESC_BYTE(4),
	USB_DESC_BYTE(USB_DESCRIPTOR_STRING),
	USB_DESC_WORD(0x0409)
};

/* Vendor */
static const uint8_t vcom_string1[] = {
	USB_DESC_BYTE(44),
	USB_DESC_BYTE(USB_DESCRIPTOR_STRING),
	'w', 0,
	'w', 0,
	'w', 0,
	'.', 0,
	'f', 0,
	'a', 0,
	'b', 0,
	'i', 0,
	'o', 0,
	'b', 0,
	'a', 0,
	'l', 0,
	't', 0,
	'i', 0,
	'e', 0,
	'r', 0,
	'i', 0,
	'.', 0,
	'c', 0,
	'o', 0,
	'm', 0
};

/* Product */
static const uint8_t vcom_string2[] = {
	USB_DESC_BYTE(26),
	USB_DESC_BYTE(USB_DESCRIPTOR_STRING),
	'n', 0,
	'r', 0,
	'f', 0,
	'-', 0,
	'e', 0,
	't', 0,
	'h', 0,
	'e', 0,
	'r', 0,
	'n', 0,
	'e', 0,
	't', 0
};

/* Strings */
static const USBDescriptor vcom_strings[] = {
	{ sizeof(vcom_string0), vcom_string0 },
	{ sizeof(vcom_string1), vcom_string1 },
	{ sizeof(vcom_string2), vcom_string2 },
};


/* GET_DESCRIPTOR callback */
static const USBDescriptor *get_descriptor(USBDriver *usbp, uint8_t dtype,
		uint8_t dindex, uint16_t lang)
{
	switch (dtype) {
	case USB_DESCRIPTOR_DEVICE:
		return &vcom_device_descriptor;
	case USB_DESCRIPTOR_CONFIGURATION:
		return &vcom_configuration_descriptor;
	case USB_DESCRIPTOR_STRING:
		if (dindex < 3)
			return &vcom_strings[dindex];
	}

	return NULL;
}

/* EP1 (bulk IN and OUT) */
static USBInEndpointState ep1_in_state;
static USBOutEndpointState ep1_out_state;
static const USBEndpointConfig ep1_config = {
	USB_EP_MODE_TYPE_BULK,
	NULL,
	sduDataTransmitted,
	sduDataReceived,
	0x0040,
	0x0040,
	&ep1_in_state,
	&ep1_out_state,
	2,
	NULL
};

/* EP2 (intr IN) */
static USBInEndpointState ep2_in_state;
static const USBEndpointConfig ep2_config = {
	USB_EP_MODE_TYPE_INTR,
	NULL,
	sduInterruptTransmitted,
	NULL,
	0x0010,
	0x0000,
	&ep2_in_state,
	NULL,
	1,
	NULL
};


/* Handles the USB driver global events */
static void usb_event(USBDriver *usbp, usbevent_t event)
{
	switch (event) {
	case USB_EVENT_RESET:
		return;
	case USB_EVENT_ADDRESS:
		return;
	case USB_EVENT_CONFIGURED:
		chSysLockFromIsr();

		/*
		 * Enables the endpoints specified into the configuration.
		 * Note, this callback is invoked from an ISR so I-Class functions
		 * must be used.
		 */
		usbInitEndpointI(usbp, USBD1_DATA_REQUEST_EP, &ep1_config);
		usbInitEndpointI(usbp, USBD1_INTERRUPT_REQUEST_EP, &ep2_config);

		/* Resetting the state of the CDC subsystem. */
		sduConfigureHookI(&SDU1);

		chSysUnlockFromIsr();
		return;
	case USB_EVENT_SUSPEND:
		return;
	case USB_EVENT_WAKEUP:
		return;
	case USB_EVENT_STALLED:
		return;
	}
	return;
}

static bool request_hook(USBDriver *usbp)
{
	struct usb_request *rq;

	rq = (struct usb_request *)usbp->setup;

	if ((rq->bmRequestType & USB_RTYPE_TYPE_MASK) != USB_RTYPE_TYPE_VENDOR)
		return sduRequestsHook(usbp);

	switch (rq->wIndex) {
	case 0:
		SCB->AIRCR = (0x5FA << SCB_AIRCR_VECTKEY_Pos) |
			SCB_AIRCR_SYSRESETREQ;
		return true;
	default:
		return false;
	}

	return false;
}

/* USB driver configuration */
static const USBConfig usbcfg = {
	usb_event,
	get_descriptor,
	request_hook,
	NULL
};

/* CDC Configuration */
static const SerialUSBConfig serusbcfg = {
	&USBD1,
	USBD1_DATA_REQUEST_EP,
	USBD1_DATA_AVAILABLE_EP,
	USBD1_INTERRUPT_REQUEST_EP
};

SerialUSBDriver *usb_get_serial_driver(void)
{
	return &SDU1;
}

void usb_init(void)
{
	/* CDC init */
	sduObjectInit(&SDU1);
	sduStart(&SDU1, &serusbcfg);

	/* USB driver init */
	usbDisconnectBus(serusbcfg.usbp);
	chThdSleepMilliseconds(100);
	usbStart(serusbcfg.usbp, &usbcfg);
	usbConnectBus(serusbcfg.usbp);
}
