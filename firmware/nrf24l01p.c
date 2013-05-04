/*
 * Copyright 2013 Fabio Baltieri <fabio.baltieri@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <ch.h>
#include <hal.h>
#include <string.h>

#include "board.h"
#include "blink.h"
#include "nrf24l01p.h"

#define SPI SPID3

#define MBSZ 10

static struct nrf_entry {
	struct Thread *th;
	WORKING_AREA(wa, 128);

	Mailbox rx;
	msg_t rx_msgs[MBSZ];
	struct nrf_raw_msg rx_bufs[MBSZ];
	int rx_idx;

	Mailbox tx;
	msg_t tx_msgs[MBSZ];
	struct nrf_raw_msg tx_bufs[MBSZ];
	int tx_idx;
} nrf;

#define ADDR_AW AW_3_BYTES
static uint8_t broadcast[] = { 0x10, 0xab, 0x0f };

#define CHANNEL 24

#define NRF_CONFIG (EN_CRC | CRCO)

/* Commands */

static uint8_t nrf_read_reg(uint8_t addr)
{
	uint8_t ret;

	addr = CMD_R_REGISTER | addr;

	spiSelect(&SPI);
	spiSend(&SPI, 1, &addr);
	spiReceive(&SPI, 1, &ret);
	spiUnselect(&SPI);

	return ret;
}

static void nrf_write_reg(uint8_t addr, uint8_t data)
{
	addr = CMD_W_REGISTER | addr;

	spiSelect(&SPI);
	spiSend(&SPI, 1, &addr);
	spiSend(&SPI, 1, &data);
	spiUnselect(&SPI);
}

static __attribute__((unused)) void nrf_read_addr(uint8_t addr, uint8_t *data, uint8_t size)
{
	addr = CMD_R_REGISTER | addr;

	spiSelect(&SPI);
	spiSend(&SPI, 1, &addr);
	spiReceive(&SPI, size, data);
	spiUnselect(&SPI);
}

static void nrf_write_addr(uint8_t addr, uint8_t *data, uint8_t size)
{
	addr = CMD_W_REGISTER | addr;

	spiSelect(&SPI);
	spiSend(&SPI, 1, &addr);
	spiSend(&SPI, size, data);
	spiUnselect(&SPI);
}

static void nrf_read_payload(uint8_t *buf, uint8_t size)
{
	uint8_t cmd;

	cmd = CMD_R_RX_PAYLOAD;

	spiSelect(&SPI);
	spiSend(&SPI, 1, &cmd);
	spiReceive(&SPI, size, buf);
	spiUnselect(&SPI);
}

static void nrf_write_payload(uint8_t *buf, uint8_t size)
{
	uint8_t cmd;

	cmd = CMD_W_TX_PAYLOAD;

	spiSelect(&SPI);
	spiSend(&SPI, 1, &cmd);
	spiSend(&SPI, size, buf);
	spiUnselect(&SPI);
}

static void nrf_flush_tx(void)
{
	uint8_t cmd;

	cmd = CMD_FLUSH_TX;

	spiSelect(&SPI);
	spiSend(&SPI, 1, &cmd);
	spiUnselect(&SPI);
}

static void nrf_flush_rx(void)
{
	uint8_t cmd;

	cmd = CMD_FLUSH_RX;

	spiSelect(&SPI);
	spiSend(&SPI, 1, &cmd);
	spiUnselect(&SPI);
}

static uint8_t nrf_get_status(void)
{
	uint8_t cmd;
	uint8_t ret;

	cmd = CMD_NOP;

	spiSelect(&SPI);
	spiExchange(&SPI, 1, &cmd, &ret);
	spiUnselect(&SPI);

	return ret;
}

/* Normal functions */

static void nrf_standby(void)
{
	nrf_ce_l();

	nrf_write_reg(CONFIG, NRF_CONFIG | PWR_UP);

	chThdSleepMilliseconds(5);
}

static void nrf_rx_mode(void)
{
	nrf_write_reg(CONFIG, NRF_CONFIG | PWR_UP | PRIM_RX);

	nrf_ce_h();
}

static void nrf_powerdown(void)
{
	nrf_ce_l();

	nrf_write_reg(CONFIG, 0x00);
}

static __attribute__((unused)) uint8_t nrf_has_data(void)
{
	if (nrf_get_status() & RX_DR) {
		nrf_write_reg(STATUS, RX_DR);
		return 1;
	} else {
		return 0;
	}
}

static uint8_t nrf_rx(uint8_t *data, uint8_t size)
{
	uint8_t pipe;
	uint8_t rx_size;

	pipe = (nrf_get_status() & RX_P_NO_MASK) >> RX_P_NO_SHIFT;

	if (pipe > 5)
		return 0;

	rx_size = nrf_read_reg(RX_PW_P0 + pipe);

	if (rx_size == size) {
		nrf_read_payload(data, rx_size);
		return rx_size;
	} else {
		nrf_flush_rx();
		return 0;
	}
}

static __attribute__((unused)) void nrf_tx(uint8_t *data, uint8_t size)
{
	nrf_write_reg(CONFIG, NRF_CONFIG | PWR_UP | PRIM_TX);

	nrf_write_payload(data, size);

	/* pulse ce to start transmit */
	nrf_ce_h();
}

void nrf_recv(struct nrf_raw_msg *msg)
{
	struct nrf_raw_msg *in_msg;

	chSysLock();
	chMBFetchS(&nrf.rx, (msg_t *)&in_msg, TIME_INFINITE);
	memcpy(msg, in_msg, sizeof(*msg));
	chSysUnlock();
}

void nrf_send(struct nrf_raw_msg *msg)
{
	struct nrf_raw_msg *out_msg;

	chSysLock();
	out_msg = &nrf.tx_bufs[nrf.tx_idx];
	chMBPostS(&nrf.tx, (msg_t)out_msg, TIME_INFINITE);
	memcpy(out_msg, msg, sizeof(*msg));
	nrf.tx_idx = (nrf.tx_idx + 1) % MBSZ;
	chSysUnlock();

	nrf_kick_loop();
}

static void nrf_irq(void)
{
	msg_t dummy;
	uint8_t status;

	status = nrf_get_status();

	/* RX data ready */
	if (status & RX_DR) {
		chSysLock();
		/* if full, drop oldest */
		if (chMBGetFreeCountI(&nrf.rx) == 0)
			chMBFetchI(&nrf.rx, &dummy);
		chSysUnlock();

		/* fill a new element */
		nrf_rx(nrf.rx_bufs[nrf.rx_idx].data, PAYLOADSZ);
		chMBPost(&nrf.rx, (msg_t)nrf.rx_bufs[nrf.rx_idx].data, TIME_IMMEDIATE);
		nrf.rx_idx = (nrf.rx_idx + 1) % MBSZ;
	}

	/* TX data sent */
	if (status & TX_DS) {
		chSysLock();
		/* go back to rx mode if it was the last packet */
		if (chMBGetUsedCountI(&nrf.tx) == 0) {
			nrf_standby();
			nrf_rx_mode();
		}
		chSysUnlock();
	}

	/* TX max retry */
	if (status & MAX_RT) {
	}

	nrf_write_reg(STATUS, status);
}

static void nrf_check_tx(void)
{
	struct nrf_raw_msg *msg;

	chSysLock();
	if (chMBGetUsedCountI(&nrf.tx)) {
		nrf_standby();

		chMBFetchS(&nrf.tx, (msg_t *)&msg, TIME_IMMEDIATE);
		nrf_tx((uint8_t *)msg, PAYLOADSZ);
	}
	chSysUnlock();
}

static msg_t nrf_radio(void *data)
{
	chRegSetThreadName("nrf_radio");

	nrf_powerdown();

	/* set TX addr and addr size */
	nrf_write_reg(SETUP_AW, ADDR_AW);
	nrf_write_addr(TX_ADDR, broadcast, sizeof(broadcast));
	nrf_write_reg(SETUP_RETR, (0x0 << ARD_SHIFT) | (0x0 << ARC_SHIFT));

	/* set RX pipe 0 */
	nrf_write_addr(RX_ADDR_P0, broadcast, sizeof(broadcast));
	nrf_write_reg(EN_RXADDR, ERX_P0);
	nrf_write_reg(EN_AA, 0);
	nrf_write_reg(DYNPD, 0);
	nrf_write_reg(RX_PW_P0, PAYLOADSZ); /* pipe 0 size */

	/* set channel */
	nrf_write_reg(RF_CH, CHANNEL);
	nrf_write_reg(RF_SETUP, RF_DR_1M | RF_PWR_3);

	nrf_flush_tx();
	nrf_flush_rx();

	/* clear interrupts */
	nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT);

	nrf_standby();
	nrf_rx_mode();

	for (;;) {
		chEvtWaitAny(ALL_EVENTS);
		blink(BLINK_RF, false);
		nrf_irq();
		nrf_check_tx();
	}

	return 0;
}

void nrf_kick_loop(void)
{
	chEvtSignal(nrf.th, 1);
}

/* Initialization */

static const SPIConfig spicfg = {
	NULL,
	/* HW dependent part.*/
	RF_GPIO,
	RF_CSN,
	SPI_CR1_BR_0, /* 36/4 = 9MHz */
};

void nrf_init(void)
{
	nrf.rx_idx = 0;
	chMBInit(&nrf.rx, (msg_t *)nrf.rx_msgs, MBSZ);
	nrf.tx_idx = 0;
	chMBInit(&nrf.tx, (msg_t *)nrf.tx_msgs, MBSZ);

	spiStart(&SPI, &spicfg);

	nrf.th = chThdCreateStatic(nrf.wa, sizeof(nrf.wa),
			NORMALPRIO + 1,
			nrf_radio, &nrf);
}
