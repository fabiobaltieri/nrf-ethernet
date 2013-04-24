#define BLINK_COUNT 3
enum led_id {
	BLINK_RED = 0,
	BLINK_GREEN,
	BLINK_RF
};

void blink(enum led_id id, bool invert);
void blink_init(void);
