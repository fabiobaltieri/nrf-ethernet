#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <string.h>

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"

#include "../board.h"
#include "../blink.h"
#include "../data.h"

#define HTTPD_PORT 80

extern SerialUSBDriver SDU1;

static struct Thread *httpd_th;
static WORKING_AREA(httpd_wa, 512);

#define GET_GET "GET "
#define GET_ROOT "/"
#define GET_VER " HTTP/1.1"

/* basic sequential stream to push on tcp */

struct net_seq_stream {
	const struct BaseSequentialStreamVMT *vmt;
	struct netconn *nc;
};

static msg_t net_put(void *instance, uint8_t c)
{
	struct net_seq_stream *seq = instance;

	netconn_write(seq->nc, (char *)&c, 1, NETCONN_COPY);

	return c;
}

const struct BaseSequentialStreamVMT net_vmt = {
	.write = NULL,
	.read = NULL,
	.put = net_put,
	.get = NULL,
};

struct net_seq_stream net = {
	.vmt = &net_vmt,
};

/* httpd code */

static void httpd_process(struct netconn *nc)
{
	struct netbuf *nb;
	char *buf;
	int ret;
	uint16_t len;
	char *url;
	int i;

	ret = netconn_recv(nc, &nb);
	if (ret) {
		goto bailout;
	}

	netbuf_data(nb, (void **)&buf, &len);

	/* cut the initial "GET " */
	if (len > strlen(GET_GET) &&
	    strncmp(GET_GET, buf, strlen(GET_GET)) == 0) {
			url = buf + strlen(GET_GET);
			len -= strlen(GET_GET);
	} else {
		goto bailout;
	}

	/* find the HTTP version and null terminate */
	for (i = 0; i < len - strlen(GET_VER); i++) {
		if (url[i] < ' ' || url[i] > '~')
			goto bailout;
		if (strncmp(GET_VER, url + i, strlen(GET_VER)) == 0) {
			url[i] = '\0';
			break;
		}
	}

	netbuf_delete(nb);

	blink(BLINK_RED, false);

	net.nc = nc;
	data_dump((BaseSequentialStream *)&net);

	netconn_close(nc);
	return;

bailout:
	netconn_close(nc);
	netbuf_delete(nb);
}

static msg_t httpd_loop(void *data)
{
	struct netconn *nc, *ncnew;
	int ret;

        chRegSetThreadName("net/httpd");

	nc = netconn_new(NETCONN_TCP);
	if (!nc)
		return 1;

	netconn_bind(nc, NULL, HTTPD_PORT);
	netconn_listen(nc);

	for (;;) {
		ret = netconn_accept(nc, &ncnew);
		if (ret)
			continue;

		httpd_process(ncnew);

		netconn_delete(ncnew);
	}

	return 0;
}

void httpd_init(void)
{
	httpd_th = chThdCreateStatic(httpd_wa, sizeof(httpd_wa),
			LOWPRIO + 2,
			httpd_loop, NULL);

}
