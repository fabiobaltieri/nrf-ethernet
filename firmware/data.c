#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <string.h>

#include "board.h"
#include "blink.h"
#include "nrf24l01p.h"
#include "data.h"
#include "nrf_frames.h"
#include "net/rawd.h"

static struct Thread *data_rx_th;
static WORKING_AREA(data_rx_wa, 64);
static struct Thread *data_clean_th;
static WORKING_AREA(data_clean_wa, 16);

#define TABLE_SIZE 32
#define DATA_SIZE 12
#define NEW_TTL 180
#define TTL_DELAY 1000 /* ms */

struct data_entry {
	uint8_t ttl;
	uint8_t board_id;
	uint8_t msg_id;
	uint8_t seq;
	uint8_t data[DATA_SIZE];
};

static struct Mutex data_mutex;
static struct data_entry data_table[TABLE_SIZE];

void data_json(BaseSequentialStream *chp)
{
	struct data_entry *entry;
	int i;
	int count;
	union nrf_msg *msg;

	chMtxLock(&data_mutex);

	chprintf(chp, "{"
			"\"device\": \"nrf-ethernet\","
			"\"timestamp\": \"%d\",", chTimeNow());

	chprintf(chp, "\"sensors\": [");

	count = 0;
	for (i = 0; i < TABLE_SIZE; i++) {
		entry = &data_table[i];
		msg = (union nrf_msg *)entry->data;

		if (entry->ttl == 0)
			continue;

		chprintf(chp, "%s{"
				"\"ttl\": %d,"
				"\"board_id\": %d,"
				"\"msg_id\": %d,"
				"\"seq\": %d,",
				(count++ == 0) ? "" : ",",
				entry->ttl,
				entry->board_id,
				entry->msg_id,
				entry->seq
				);

		switch (entry->msg_id) {
		case NRF_MSG_ID_POWER:
			chprintf(chp, "\"type\": \"power\",");
			chprintf(chp, "\"power1\": %d,"
					"\"power2\": %d,"
					"\"power3\": %d,"
					"\"power4\": %d,"
					"\"battery\": %d",
					msg->power.value[0],
					msg->power.value[1],
					msg->power.value[2],
					msg->power.value[3],
					msg->power.vbatt
					);
			break;
		default:
			chprintf(chp, "\"type\": \"unknown\"");
		}

		chprintf(chp, "}");
	}

	chprintf(chp, "]}");

	chMtxUnlock();
}

void data_dump(BaseSequentialStream *chp)
{
	struct data_entry *entry;
	int i, j;

	chMtxLock(&data_mutex);

	for (i = 0; i < TABLE_SIZE; i++) {
		entry = &data_table[i];

		chprintf(chp, "%2d %3d - ", i, entry->ttl);
		chprintf(chp, "%.2x %.2x %.2x - ",
				entry->board_id, entry->msg_id, entry->seq);
		for (j = 0; j < DATA_SIZE; j++) {
			chprintf(chp, " %.2x", entry->data[j]);
			if (j + 1 == DATA_SIZE / 2)
				chprintf(chp, " ");
		}
		chprintf(chp, "\r\n");
	}

	chMtxUnlock();
}

static void update_entry(struct data_entry *entry, struct nrf_frame *msg)
{
	entry->ttl = NEW_TTL;
	entry->board_id = msg->board_id;
	entry->msg_id = msg->msg_id;
	entry->seq = msg->seq;
	memcpy(entry->data, msg->msg.generic, DATA_SIZE);
}

static void update_table(struct nrf_frame *msg)
{
	struct data_entry *entry;
	int first_free = -1;
	int match = -1;
	int i;

	chMtxLock(&data_mutex);

	for (i = 0; i < TABLE_SIZE; i++) {
		entry = &data_table[i];

		if (first_free < 0 && entry->ttl == 0)
			first_free = i;

		if (match < 0 &&
		    entry->board_id == msg->board_id &&
		    entry->msg_id == msg->msg_id) {
			match = i;
			break;
		}
	}

	if (match >= 0)
		update_entry(&data_table[match], msg);
	else if (first_free >= 0)
		update_entry(&data_table[first_free], msg);

	chMtxUnlock();
}

static msg_t data_rx(void *data)
{
	struct nrf_frame msg;

	chRegSetThreadName("data_rx");

	for (;;) {
		nrf_recv((struct nrf_raw_msg *)&msg);
		update_table(&msg);
		rawd_push(&msg);
	}

	return 0;
}

static msg_t data_clean(void *data)
{
	struct data_entry *entry;
	int i;

	chRegSetThreadName("data_clean");

	for (;;) {
		chThdSleepMilliseconds(TTL_DELAY);

		chMtxLock(&data_mutex);

		for (i = 0; i < TABLE_SIZE; i++) {
			entry = &data_table[i];

			if (entry->ttl == 0)
				continue;

			entry->ttl--;
			if (entry->ttl == 0)
				memset(entry, 0, sizeof(*entry));
		}

		chMtxUnlock();
	}

	return 0;
}

void data_init(void)
{
	int i;

	for (i = 0; i < TABLE_SIZE; i++)
		memset(&data_table[i], 0, sizeof(*data_table));

	chMtxInit(&data_mutex);

	data_rx_th = chThdCreateStatic(data_rx_wa, sizeof(data_rx_wa),
			NORMALPRIO,
			data_rx, NULL);
	data_clean_th = chThdCreateStatic(data_clean_wa, sizeof(data_clean_wa),
			NORMALPRIO,
			data_clean, NULL);
}
