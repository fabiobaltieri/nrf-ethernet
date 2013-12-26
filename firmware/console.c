#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <shell.h>
#include <string.h>

#include <lwip/netif.h>
#include <lwip/dhcp.h>
#include <lwip/stats.h>

#include "console.h"
#include "data.h"

#define SHELL_WA_SIZE   THD_WA_SIZE(2048)

static struct Thread *console_th = NULL;

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

static void cmd_data_dump(BaseSequentialStream *chp, int argc, char *argv[])
{
	data_dump(chp);
}

#define pflag(c, f, b, s) do {if (f & b) chprintf(c, " " s);} while (0)
static void cmd_net(BaseSequentialStream *chp, int argc, char *argv[])
{
	struct netif *nf = netif_default;
	struct dhcp *dhcp = nf->dhcp;
	int i;

	chprintf(chp, "Interface %d: %c%c\r\n",
			nf->num, nf->name[0], nf->name[1]);
	chprintf(chp, "  ip_addr: %d.%d.%d.%d\r\n",
			ip4_addr1(&nf->ip_addr.addr),
			ip4_addr2(&nf->ip_addr.addr),
			ip4_addr3(&nf->ip_addr.addr),
			ip4_addr4(&nf->ip_addr.addr));
	chprintf(chp, "  netmask: %d.%d.%d.%d\r\n",
			ip4_addr1(&nf->netmask.addr),
			ip4_addr2(&nf->netmask.addr),
			ip4_addr3(&nf->netmask.addr),
			ip4_addr4(&nf->netmask.addr));
	chprintf(chp, "  gw: %d.%d.%d.%d\r\n",
			ip4_addr1(&nf->gw.addr),
			ip4_addr2(&nf->gw.addr),
			ip4_addr3(&nf->gw.addr),
			ip4_addr4(&nf->gw.addr));
	chprintf(chp, "  hostname: %s\r\n", nf->hostname);
	chprintf(chp, "  mtu: %d\r\n", nf->mtu);
	chprintf(chp, "  hwaddr: ");
	for (i = 0; i < nf->hwaddr_len; i++)
		chprintf(chp, "%02x", nf->hwaddr[i]);
	chprintf(chp, "\r\n");
	chprintf(chp, "  flags (%x):", nf->flags);
	pflag(chp, nf->flags, NETIF_FLAG_UP, "up");
	pflag(chp, nf->flags, NETIF_FLAG_BROADCAST, "broadcast");
	pflag(chp, nf->flags, NETIF_FLAG_POINTTOPOINT, "pointtopoint");
	pflag(chp, nf->flags, NETIF_FLAG_DHCP, "dhcp");
	pflag(chp, nf->flags, NETIF_FLAG_LINK_UP, "link_up");
	pflag(chp, nf->flags, NETIF_FLAG_ETHARP, "etharp");
	pflag(chp, nf->flags, NETIF_FLAG_ETHERNET, "ethernet");
	pflag(chp, nf->flags, NETIF_FLAG_IGMP, "igmp");
	chprintf(chp, "\r\n");

	chprintf(chp, "DHCP:\r\n");
	chprintf(chp, "  state: %d\r\n", dhcp->state);
	chprintf(chp, "  tries: %d\r\n", dhcp->tries);

	nf = nf->next;
}

static const ShellCommand commands[] = {
	{"mem", cmd_mem},
	{"ps", cmd_ps},
	{"dd", cmd_data_dump},
	{"net", cmd_net},
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
