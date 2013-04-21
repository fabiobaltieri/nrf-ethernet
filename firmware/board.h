#define BOARD_NAME "nrf-ethernet"

/* MCU configuration */
#define STM32_HSECLK 25000000
#define STM32F10X_CL /* Connectivity Line */

/* Ethernet configuration */
/* TODO: PHY configuration */
#define BOARD_PHY_RESET()
#define BOARD_PHY_ADDRESS 0
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

/*                     Eeee.EE. */
#define VAL_GPIOACRL 0x44828B48
/*                     r..UUUUc */
#define VAL_GPIOACRH 0x84444442
#define VAL_GPIOAODR 0xFFFFFFEF

/*                     UUlrrX.. */
#define VAL_GPIOBCRL 0x4A211488
/*                     ..EEE.CC */
#define VAL_GPIOBCRH 0x88BBB894
#define VAL_GPIOBODR 0xFFFFFFCF

/*                     ..EE..E. */
#define VAL_GPIOCCRL 0x884488B8
/*                     ll.RRR.. */
#define VAL_GPIOCCRH 0x228B8B88
#define VAL_GPIOCODR 0xFFFF3FFF

#define VAL_GPIODCRL 0x88888888
#define VAL_GPIODCRH 0x88888888
#define VAL_GPIODODR 0xFFFFFFFF

#define VAL_GPIOECRL 0x88888888
#define VAL_GPIOECRH 0x88888888
#define VAL_GPIOEODR 0xFFFFFFFF

/* I/O */
#define LED_STATUS_GPIO GPIOC
#define LED_STATUS_RED 15
#define LED_STATUS_GREEN 14

#define led_red_on() palSetPad(LED_STATUS_GPIO, LED_STATUS_RED)
#define led_red_off() palClearPad(LED_STATUS_GPIO, LED_STATUS_RED)
#define led_red_toggle() palTogglePad(LED_STATUS_GPIO, LED_STATUS_RED)
#define led_green_on() palSetPad(LED_STATUS_GPIO, LED_STATUS_GREEN)
#define led_green_off() palClearPad(LED_STATUS_GPIO, LED_STATUS_GREEN)
#define led_green_toggle() palTogglePad(LED_STATUS_GPIO, LED_STATUS_GREEN)

#define LED_RF_GPIO GPIOB
#define LED_RF 5

#define led_rf_on() palSetPad(LED_RF_GPIO, LED_RF)
#define led_rf_off() palClearPad(LED_RF_GPIO, LED_RF)
#define led_rf_toggle() palTogglePad(LED_RF_GPIO, LED_RF)

#define CAN_SHDN_GPIO GPIOA
#define CAN_SHDN 8

#define can_xcvr_enable() palClearPad(CAN_SHDN_GPIO, CAN_SHDN)
#define can_xcvr_disable() palSetPad(CAN_SHDN_GPIO, CAN_SHDN)

#define ETH_RST_GPIO GPIOA
#define ETH_RST 4

#define eth_reset_l() palClearPad(ETH_RST_GPIO, ETH_RST)
#define eth_reset_h() palSetPad(ETH_RST_GPIO, ETH_RST)

#define RF_GPIO GPIOB
#define RF_CSN 3
#define RF_CE 4

#define nrf_cs_l() palClearPad(RF_GPIO, RF_CSN)
#define nrf_cs_h() palSetPad(RF_GPIO, RF_CSN)
#define nrf_ce_l() palClearPad(RF_GPIO, RF_CE)
#define nrf_ce_h() palSetPad(RF_GPIO, RF_CE)

void boardInit(void);

#define pr_info(fmt, args...) chprintf((BaseSequentialStream *)&SD1, fmt, ## args)
#define pr_debug(fmt, args...) chprintf((BaseSequentialStream *)&SD1, "%s: " fmt, __func__, ## args)
