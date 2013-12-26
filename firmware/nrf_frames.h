#include <stdint.h>

#define NRF_MSG_ID_GENERIC	0x00
#define NRF_MSG_ID_POWER	0x01

#define NRF_POWER_VBATT_CHARGING	(1 << 15)
#define NRF_POWER_VBATT_MASK		0x7fff
struct nrf_power {
	uint16_t value[4];
	uint16_t vbatt;
	uint8_t _spare[2];
};

union nrf_msg {
	uint8_t generic[12];
	struct nrf_power power;
};

struct nrf_frame {
	uint8_t board_id;
	uint8_t msg_id;
	uint8_t len;
	uint8_t seq;
	union nrf_msg msg;
};
