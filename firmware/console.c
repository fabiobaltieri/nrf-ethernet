#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <shell.h>

#include "console.h"

#define SHELL_WA_SIZE   THD_WA_SIZE(2048)

static Thread *console_th = NULL;

static void cmd_mem(BaseSequentialStream *chp, int argc, char *argv[])
{
	size_t n, size;

	if (argc > 0) {
		chprintf(chp, "Usage: mem\r\n");
		return;
	}
	n = chHeapStatus(NULL, &size);
	chprintf(chp, "core free memory : %u bytes\r\n", chCoreStatus());
	chprintf(chp, "heap fragments   : %u\r\n", n);
	chprintf(chp, "heap free total  : %u bytes\r\n", size);
}

static void cmd_ps(BaseSequentialStream *chp, int argc, char *argv[])
{
	static const char *states[] = {THD_STATE_NAMES};
	Thread *th;

	if (argc > 0) {
		chprintf(chp, "Usage: ps\r\n");
		return;
	}

	chprintf(chp, "    addr    stack prio refs     state     time\r\n");

	th = chRegFirstThread();
	do {
		chprintf(chp, "%8x %8x %4u %4u %9s %8u %s\r\n",
				th,
				th->p_ctx.r13,
				th->p_prio,
				th->p_refs - 1,
				states[th->p_state],
				th->p_time, th->p_name);
		th = chRegNextThread(th);
	} while (th != NULL);
}

static const ShellCommand commands[] = {
	{"mem", cmd_mem},
	{"ps", cmd_ps},
	{}
};

static ShellConfig shell_cfg = {
	NULL, /* set by console_init */
	commands
};

void console_init(BaseSequentialStream *stream)
{
	shell_cfg.sc_channel = stream;
	shellInit();
}

void console_poll(void)
{
	if (!console_th)
		console_th = shellCreate(&shell_cfg, SHELL_WA_SIZE, NORMALPRIO);
	else if (chThdTerminated(console_th)) {
		chThdRelease(console_th);
		console_th = NULL;
	}
}
