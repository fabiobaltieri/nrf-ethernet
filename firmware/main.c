#include <ch.h>
#include <hal.h>
#include <test.h>

#include "board.h"
#include "console.h"

/* GPIO initialization values */
const PALConfig pal_default_config = {
	{ VAL_GPIOAODR, VAL_GPIOACRL, VAL_GPIOACRH },
	{ VAL_GPIOBODR, VAL_GPIOBCRL, VAL_GPIOBCRH },
	{ VAL_GPIOCODR, VAL_GPIOCCRL, VAL_GPIOCCRH },
	{ VAL_GPIODODR, VAL_GPIODCRL, VAL_GPIODCRH },
	{ VAL_GPIOEODR, VAL_GPIOECRL, VAL_GPIOECRH },
};

void __early_init(void)
{
	stm32_clock_init();
}

void boardInit(void)
{
	/* alternate function init */
	AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_DISABLE |
		AFIO_MAPR_USART1_REMAP |
		AFIO_MAPR_SPI3_REMAP |
		AFIO_MAPR_CAN_REMAP_REMAP2;
}

static WORKING_AREA(blinker_wa, 128);
static msg_t blinker(void *data)
{
	chRegSetThreadName("blinker");

	for (;;) {
		palTogglePad(LED_STATUS_GPIO, LED_STATUS_RED);
		palTogglePad(LED_STATUS_GPIO, LED_STATUS_GREEN);
		palTogglePad(LED_RF_GPIO, LED_RF);
		chThdSleepMilliseconds(500);
	}

	return 0;
}

static void hello(void)
{
	int i;

	for (i = 0; i < 8; i++) {
		palTogglePad(LED_STATUS_GPIO, LED_STATUS_RED);
		palTogglePad(LED_STATUS_GPIO, LED_STATUS_GREEN);
		chThdSleepMilliseconds(50);
	}

}

int main(void)
{
	/* system init */
	halInit();
	chSysInit();

	/* local init */
	console_init();

	hello();

	sdStart(&SD1, NULL);

	chThdCreateStatic(blinker_wa, sizeof(blinker_wa),
			NORMALPRIO,
			blinker, NULL);

	for (;;) {
		console_poll();
		chThdSleepMilliseconds(500);
	}
}
