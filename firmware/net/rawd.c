#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <string.h>

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/tcp.h"

#include "../board.h"
#include "../blink.h"
#include "../data.h"
#include "../nrf_frames.h"

#include "rawd.h"

#define RAW_PORT 8888

static struct Thread *rawd_th;
static WORKING_AREA(rawd_wa, 128);
static struct Thread *rawd_push_th;
static WORKING_AREA(rawd_push_wa, 512);

extern SerialUSBDriver SDU1;

#define NC_COUNT 3
static struct netconn *ncs[NC_COUNT];
static struct nrf_frame buf_frame;

static struct Mutex rawd_mutex;

#define RAWD_READY	"\x00\x00\x00\x00# ready ####"
#define RAWD_FULL	"\x00\x00\x00\x00# full #####"

static msg_t rawd_push_loop(void *data)
{
	int i;

	chRegSetThreadName("net/rawd_push");

	for (;;) {
		chEvtWaitAny(ALL_EVENTS);

		chMtxLock(&rawd_mutex);
		for (i = 0; i < NC_COUNT; i++) {
			if (!ncs[i])
				continue;
			if (ncs[i]->pcb.tcp->state != ESTABLISHED)
				continue;

			netconn_write(ncs[i], &buf_frame, sizeof(buf_frame),
					NETCONN_COPY);
		}
		chMtxUnlock();
	}

	return 0;
}

void rawd_push(struct nrf_frame *msg)
{
	chMtxLock(&rawd_mutex);
	memcpy(&buf_frame, msg, sizeof(*msg));
	chMtxUnlock();

	chEvtSignal(rawd_push_th, 1);
}

static void rawd_new(struct netconn *nc)
{
	int i;
	int free = -1;

	chMtxLock(&rawd_mutex);

	/* check for existing connections */
	for (i = 0; i < NC_COUNT; i++) {
		if (ncs[i] == NULL) {
			free = i;
		} else if (ncs[i]->pcb.tcp->state != ESTABLISHED) {
			/* kill stale connections */
			netconn_close(ncs[i]);
			netconn_delete(ncs[i]);
			ncs[i] = NULL;
			free = i;
		}
	}

	if (free < 0) {
		netconn_write(nc, RAWD_FULL, sizeof(RAWD_FULL) - 1, NETCONN_COPY);
		netconn_close(nc);
		netconn_delete(nc);
		chMtxUnlock();
		return;
	}

	ncs[free] = nc;

	chMtxUnlock();

	netconn_write(nc, RAWD_READY, sizeof(RAWD_READY) - 1, NETCONN_COPY);
}

static msg_t rawd_loop(void *data)
{
	struct netconn *nc, *ncnew;
	int ret;

	chRegSetThreadName("net/rawd");

	nc = netconn_new(NETCONN_TCP);
	if (!nc)
		return 1;

	netconn_bind(nc, NULL, RAW_PORT);
	netconn_listen(nc);

	for (;;) {
		ret = netconn_accept(nc, &ncnew);
		if (ret)
			continue;

		rawd_new(ncnew);
	}

	return 0;
}

void rawd_init(void)
{
	chMtxInit(&rawd_mutex);

	memset(ncs, sizeof(ncs), 0);

	rawd_th = chThdCreateStatic(rawd_wa, sizeof(rawd_wa),
			LOWPRIO + 2,
			rawd_loop, NULL);

	rawd_push_th = chThdCreateStatic(rawd_push_wa, sizeof(rawd_push_wa),
			LOWPRIO + 2,
			rawd_push_loop, NULL);
}
