#include <ch.h>
#include <hal.h>

#include "blink.h"

#define BLINK_DELTA MS2ST(50)

enum {
	BLINK_EV_POS = (1 << 0),
	BLINK_EV_NEG = (1 << 1),
};

struct blink_entry {
	enum led_id id;
	char *name;
	struct Thread *th;
	WORKING_AREA(wa, 16);
};

static struct blink_entry blinks[BLINK_COUNT] = {
	{
		.id = BLINK_RED,
		.name = "blink_red",
	}, {
		.id = BLINK_GREEN,
		.name = "blink_green",
	}, {
		.id = BLINK_RF,
		.name = "blink_rf",
	}
};

static void led_set(enum led_id id, bool on)
{
	switch (id) {
		case BLINK_RED:
			if (on)
				led_red_on();
			else
				led_red_off();
			break;
		case BLINK_GREEN:
			if (on)
				led_green_on();
			else
				led_green_off();
			break;
		case BLINK_RF:
			if (on)
				led_rf_on();
			else
				led_rf_off();
			break;
	}
}

static msg_t blinker(void *data)
{
	struct blink_entry *blink = data;
	int evt = 1;

	chRegSetThreadName(blink->name);

	for (;;) {
		led_set(blink->id,(evt == BLINK_EV_POS) ? true : false);
		chThdSleepMilliseconds(BLINK_DELTA);
		led_set(blink->id, (evt == BLINK_EV_POS) ? false : true);
		chThdSleepMilliseconds(BLINK_DELTA);
		evt = chEvtWaitAny(ALL_EVENTS);
	}

	return 0;
}


void blink(enum led_id id, bool invert)
{
	if (id >= BLINK_COUNT)
		return;

	chEvtSignal(blinks[id].th,
			(invert) ? BLINK_EV_NEG : BLINK_EV_POS);
}

void blink_init(void)
{
	int i;

	for (i = 0; i < BLINK_COUNT; i++)
		blinks[i].th = chThdCreateStatic(
				blinks[i].wa, sizeof(blinks[i].wa),
				LOWPRIO,
				blinker, &blinks[i]);
}
