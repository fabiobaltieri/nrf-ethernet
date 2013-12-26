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
#include "../nrf24l01p.h"

#include "httpd.h"

#define HTTPD_PORT 80

extern SerialUSBDriver SDU1;

static struct Thread *httpd_th;
static WORKING_AREA(httpd_wa, 512);

#define HTTP_GET "GET "
#define HTTP_POST "POST "
#define HTTP_ROOT "/"
#define HTTP_VER " HTTP/1.1\r\n"
#define HTTP_BREAK "\r\n\r\n"

/* basic sequential stream to push on tcp */

struct net_seq_stream {
	const struct BaseSequentialStreamVMT *vmt;
	struct netconn *nc;
	char buf[512];
	int count;
};

static msg_t net_put(void *instance, uint8_t c)
{
	struct net_seq_stream *net = instance;

	net->buf[net->count++] = c;

	if (net->count == sizeof(net->buf)) {
		netconn_write(net->nc, net->buf, net->count, NETCONN_COPY);
		net->count = 0;
	}

	return c;
}

static void net_finalize(struct net_seq_stream *net)
{
	if (net->count)
		netconn_write(net->nc, net->buf, net->count, NETCONN_COPY);
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
	void (*handler)(struct net_seq_stream *net,
			char *url,
			char *hdr,
			char *data, int dlen);
};

static struct path paths[];

static void path_root(struct net_seq_stream *net, char *url,
		char *hdr, char *data, int dlen)
{
	struct path *path = paths;
	chprintf((BaseSequentialStream *)net,
			"HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n");
	chprintf((BaseSequentialStream *)net,
			"<html>"
			"<head><title>Available paths</title></head>"
			"<body>");
	for (path = paths; path->path != NULL; path++)
		chprintf((BaseSequentialStream *)net,
				"<a href=\"%s\">%s</a><br />",
				path->path, path->path);
	chprintf((BaseSequentialStream *)net,
			"<body>"
			"</html>");
}

static void path_raw(struct net_seq_stream *net, char *url,
		char *hdr, char *data, int dlen)
{
	chprintf((BaseSequentialStream *)net,
			"HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\n");
	data_dump((BaseSequentialStream *)net);
}

static int nibble_valid(char ch)
{
	if ((ch >= '0' && ch <= '9') ||
	    (ch >= 'a' && ch <= 'f') ||
	    (ch >= 'A' && ch <= 'F'))
		return 1;
	else
		return 0;
}

static char get_nibble(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	else if (ch >= 'a' && ch <= 'z')
		return ch - 'a' + 10;
	else if (ch >= 'A' && ch <= 'Z')
		return ch - 'A' + 10;
	return 0;
}

static void path_sendraw(struct net_seq_stream *net, char *url,
		char *hdr, char *data, int dlen)
{
	int i;
	struct nrf_raw_msg msg;

	chprintf((BaseSequentialStream *)net,
			"HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\n");

	if (dlen != PAYLOADSZ * 2)
		goto out;

	for (i = 0; i < PAYLOADSZ; i++) {
		if (!nibble_valid(data[i * 2]) ||
		    !nibble_valid(data[i * 2 + 1]))
			goto out;
		msg.data[i] = (get_nibble(data[i * 2]) << 4) |
			get_nibble(data[i * 2 + 1]);
	}

	nrf_send(&msg);

	chprintf((BaseSequentialStream *)net,
			"OK\r\n");
	return;

out:
	chprintf((BaseSequentialStream *)net,
			"ERR\r\n");
}

static void path_json(struct net_seq_stream *net, char *url,
		char *hdr, char *data, int dlen)
{
	chprintf((BaseSequentialStream *)net,
			"HTTP/1.1 200 OK\r\nContent-type: application/json\r\n\r\n");
	data_json((BaseSequentialStream *)net);
}

static void path_notfound(struct net_seq_stream *net, char *url,
		char *hdr, char *data, int dlen)
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
	{ "/raw", path_raw },
	{ "/json", path_json },
	{ "/sendraw", path_sendraw },
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
	char *hdr;
	char *data;
	int dlen;
	int i;
	struct path *path;

	ret = netconn_recv(nc, &nb);
	if (ret) {
		goto bailout;
	}

	netbuf_data(nb, (void **)&buf, &len);

	/* cut the initial "GET " */
	if (len > strlen(HTTP_GET) &&
	    strncmp(HTTP_GET, buf, strlen(HTTP_GET)) == 0) {
			url = buf + strlen(HTTP_GET);
			len -= strlen(HTTP_GET);
	} else if (len > strlen(HTTP_POST) &&
	    strncmp(HTTP_POST, buf, strlen(HTTP_POST)) == 0) {
			url = buf + strlen(HTTP_POST);
			len -= strlen(HTTP_POST);
	} else {
		goto bailout;
	}

	/* find the HTTP version and null terminate */
	for (i = 0; i < len - strlen(HTTP_VER); i++) {
		if (url[i] < ' ' || url[i] > '~')
			goto bailout;
		if (strncmp(HTTP_VER, url + i, strlen(HTTP_VER)) == 0) {
			url[i] = '\0';
			break;
		}
	}

	/* find header pointer and length */
	hdr = url + i + strlen(HTTP_VER);
	dlen = len - i - strlen(HTTP_VER);

	/* find end of headers and data length */
	for (i = 0; i < len - strlen(HTTP_BREAK); i++) {
		if (hdr[i] < ' ' || hdr[i] > '~')
			if (hdr[i] != '\r' && hdr[i] != '\n')
				goto bailout;
		if (strncmp(HTTP_BREAK, hdr + i, strlen(HTTP_BREAK)) == 0) {
			hdr[i] = '\0';
			break;
		}
	}

	/* find data pointer and length */
	data = hdr + i + strlen(HTTP_BREAK);
	dlen = dlen - i - strlen(HTTP_BREAK);

	blink(BLINK_RED, false);

	for (path = paths; path->path != NULL; path++)
		if (strcmp(path->path, url) == 0)
			break;

	net.nc = nc;
	net.count = 0;
	if (path->handler)
		path->handler(&net, url, hdr, data, dlen);
	else
		path_notfound(&net, url, hdr, data, dlen);

	netbuf_delete(nb);

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
