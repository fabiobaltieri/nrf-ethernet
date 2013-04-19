#define BOARD_NAME "nrf-ethernet"

/* MCU configuration */
#define STM32_HSECLK 25000000
#define STM32F10X_CL /* Connectivity Line */

/* Ethernet configuration */
#define BOARD_PHY_ID            MII_STE101P_ID
#define BOARD_PHY_RMII

/*
 * I/O default setup mappings:
 *   0 - Analog input.
 *   1 - Push Pull output 10MHz.
 *   2 - Push Pull output 2MHz.
 *   3 - Push Pull output 50MHz.
 *   4 - Digital input.
 *   5 - Open Drain output 10MHz.
 *   6 - Open Drain output 2MHz.
 *   7 - Open Drain output 50MHz.
 *   8 - Digital input with PullUp or PullDown resistor depending on ODR.
 *   9 - Alternate Push Pull output 10MHz.
 *   A - Alternate Push Pull output 2MHz.
 *   B - Alternate Push Pull output 50MHz.
 *   C - Reserved.
 *   D - Alternate Open Drain output 10MHz.
 *   E - Alternate Open Drain output 2MHz.
 *   F - Alternate Open Drain output 50MHz.
 */

#define VAL_GPIOACRL 0x48818B48
#define VAL_GPIOACRH 0x84444441
#define VAL_GPIOAODR 0xFFFFFFFF

#define VAL_GPIOBCRL 0x4A111488
#define VAL_GPIOBCRH 0x88BBB814
#define VAL_GPIOBODR 0xFFFFFFFF

#define VAL_GPIOCCRL 0x884488B8
#define VAL_GPIOCCRH 0x224B8B88
#define VAL_GPIOCODR 0xFFFFFFFF

#define VAL_GPIODCRL 0x88888888
#define VAL_GPIODCRH 0x88888888
#define VAL_GPIODODR 0xFFFFFFFF

#define VAL_GPIOECRL 0x88888888
#define VAL_GPIOECRH 0x88888888
#define VAL_GPIOEODR 0xFFFFFFFF

/* I/O */
#define GPIOC_LED_STATUS1       14
#define GPIOC_LED_STATUS2       15

void boardInit(void);
