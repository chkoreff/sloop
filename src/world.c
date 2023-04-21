#include <sloop.h>
#include <stdarg.h>
#include <stdio.h>
// #include <stdlib.h> // malloc realloc LATER dynamic buffers
#include <unistd.h> // read alarm
#include <util.h> // die

typedef struct buffer
	{
	size_t len;
	size_t size;
	char *data;
	} buffer;

// LATER: Always need room for a decent size write, so make sure there's at
// least 1024 available before calling vsnprintf.
void bprintf(buffer *buf, const char *format, ...)
	{
	va_list args;
	va_start(args,format);

	{
	char *str = buf->data + buf->len;
	size_t size = buf->size - buf->len;
	int nb = vsnprintf(str, size, format, args);

	if (nb < 0 || nb >= size)
		{
		fprintf(stderr,"Truncated output: size = %lu, nb = %d\n", size, nb);
		die(0);
		}
	buf->len += nb;
	}

	va_end(args);
	}

void do_write(int fd, const char *buf, size_t count)
	{
	ssize_t nb = write(fd, buf, count);
	if (nb != count)
		die("ERROR on write"); // LATER iterate until write is complete
	}

// LATER keep reading until you see the CRLF on a line by itself which
// indicates end of headers.
static void read_request(int fd, buffer *buf)
	{
	ssize_t nb = read(fd, buf->data, buf->size);
	if (nb > 0)
		{
		buf->len += nb;

		if (0) // LATER elim
		{
		fprintf(stderr, "read %ld bytes\n",buf->len);
		do_write(2,buf->data,buf->len);
		}
		}
	}

static void bsend(buffer *buf, int fd)
	{
	do_write(fd, buf->data, buf->len);
	buf->len = 0;
	}

// LATER 20230421: Include an example of "Transfer-Encoding: chunked"

static void meta(buffer *buf, const char *key, const char *val)
	{
	bprintf(buf,"<meta http-equiv=\"%s\" content=\"%s\">\n", key, val);
	}

static void write_response(int fd)
	{
	char data[8192]; // LATER dynamic buffers
	buffer s_buf = { 0, sizeof(data), data };
	buffer *buf = &s_buf;

	bprintf(buf,"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">\n");
	bprintf(buf,"<html>\n");

	bprintf(buf,"<head>\n");
	meta(buf, "Content-Type", "text/html; charset=utf-8");
	meta(buf, "Content-Language", "en-us");
	bprintf(buf,"<title>%s</title>\n", "Test");
	bprintf(buf,"</head>\n");

	bprintf(buf,"<body>\n");
	bprintf(buf,"<p>\n");
	bprintf(buf,"This is a test.\n");
	bprintf(buf,"</body>\n");
	bprintf(buf,"</html>\n");

	dprintf(fd,"HTTP/1.1 200 OK\n");
	dprintf(fd,"Content-Type: text/html\n");
	dprintf(fd,"Content-Length: %lu\n",buf->len);
	dprintf(fd,"\n");

	if (0) // LATER elim test code
	{
	fprintf(stderr,"length = %lu\n",buf->len);
	printf("length = %lu\n",buf->len); // No effect because stdout is closed.
	}

	bsend(buf,fd);
	}

// Number of requests to allow per session (inbound socket connection)
const int max_request = 10;

// Maximum number of seconds to wait for a full request to come in
const int max_time = 30;

static void do_session(int fd)
	{
	char buf_in[8192]; // LATER dynamic buffers
	buffer b_in = { 0, sizeof(buf_in), buf_in };

	buffer *buf = &b_in;

	for (int i = 0; i < max_request; i++)
		{
		alarm(max_time);

		buf->len = 0;
		read_request(fd,buf);

		alarm(0);

		if (buf->len == 0)
			break;

		write_response(fd);
		}

	if (0)
		fprintf(stderr,"== end of session\n"); // LATER elim
	}

int main(int argc, const char *argv[])
	{
	return run_server(argc, argv, "127.0.0.1", 9722, do_session);
	}
