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
	char buf[512];
	int count;
};

static msg_t net_put(void *instance, uint8_t c)
{
	struct net_seq_stream *seq = instance;

	seq->buf[seq->count++] = c;

	if (seq->count == sizeof(seq->buf)) {
		netconn_write(seq->nc, seq->buf, seq->count, NETCONN_COPY);
		seq->count = 0;
	}

	return c;
}

static void net_finalize(struct net_seq_stream *seq)
{
	if (seq->count)
		netconn_write(seq->nc, seq->buf, seq->count, NETCONN_COPY);
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

/* paths */

struct path {
	char *path;
	void (*handler)(struct net_seq_stream *net);
};

static void path_root(struct net_seq_stream *net)
{
	chprintf((BaseSequentialStream *)net,
			"HTTP/1.1 200 OK\r\nContent-type: application/json\r\n\r\n");
	data_dump((BaseSequentialStream *)net);
}

static void path_notfound(struct net_seq_stream *net)
{
	chprintf((BaseSequentialStream *)net,
			"HTTP/1.1 404 Not Found\r\n\r\n");
	chprintf((BaseSequentialStream *)net,
			"<html>"
			"<head><title>404 - Not Found</title></head>"
			"<body><h1>404 - Not Found</h1></body>"
			"</html>");
}

static struct path paths[] = {
	{ "/", path_root },
	{ NULL, NULL },
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
	struct path *path;

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

	for (path = paths; path->path != NULL; path++)
		if (strcmp(path->path, url) == 0)
			break;

	net.nc = nc;
	net.count = 0;
	if (path->handler)
		path->handler(&net);
	else
		path_notfound(&net);

	net_finalize(&net);

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
