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
	AFIO->MAPR |= AFIO_MAPR_USART1_REMAP;
}

static WORKING_AREA(wa_blinker, 128);
static msg_t blinker(void *data)
{
	chRegSetThreadName("blinker");

	for (;;) {
		palClearPad(GPIOC, GPIOC_LED_STATUS1);
		palSetPad(GPIOC, GPIOC_LED_STATUS2);
		chThdSleepMilliseconds(500);
		palSetPad(GPIOC, GPIOC_LED_STATUS1);
		palClearPad(GPIOC, GPIOC_LED_STATUS2);
		chThdSleepMilliseconds(500);
	}

	return 0;
}

static void hello(void)
{
	int i;

	for (i = 0; i < 8; i++) {
		palTogglePad(GPIOC, GPIOC_LED_STATUS1);
		palTogglePad(GPIOC, GPIOC_LED_STATUS2);
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

	chThdCreateStatic(wa_blinker, sizeof(wa_blinker), NORMALPRIO, blinker, NULL);

	for (;;) {
		console_poll();
		chThdSleepMilliseconds(500);
	}
}
