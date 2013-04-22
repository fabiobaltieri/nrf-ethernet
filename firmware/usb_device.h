struct usb_request {
	uint8_t  bmRequestType;
	uint8_t  bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
};

SerialUSBDriver *usb_get_serial_driver(void);
void usb_init(void);
