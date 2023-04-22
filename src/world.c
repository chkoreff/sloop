#include <stddef.h>

#include <buffer.h>
#include <sloop.h>
#include <stdio.h>
#include <unistd.h> // alarm
// #include <util.h> // die

// LATER Keep reading until you see CRLF on a line by itself which indicates
// end of headers.
static void read_request(int fd, buffer *buf)
	{
	buf->len = 0;
	buf_read(buf,fd,8192);
	}

// LATER 20230421: Include an example of "Transfer-Encoding: chunked"

static void meta(buffer *buf, const char *key, const char *val)
	{
	bprintf(buf,"<meta http-equiv=\"%s\" content=\"%s\">\n", key, val);
	}

static void show_link(buffer *buf, const char *url, const char *label)
	{
	bprintf(buf,"<a style='margin-right:10px;' href=\"%s\">%s</a>\n",
		url, label);
	}

// LATER Respond differently to the favicon.ico request from Brave browser.
// GET / HTTP/1.1
// GET /favicon.ico HTTP/1.1
static void compose_page(buffer *buf)
	{
	bprintf(buf,"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">\n");
	bprintf(buf,"<html>\n");

	bprintf(buf,"<head>\n");
	meta(buf, "Content-Type", "text/html; charset=utf-8");
	meta(buf, "Content-Language", "en-us");
	bprintf(buf,"<title>%s</title>\n", "Test");
	bprintf(buf,"</head>\n");

	bprintf(buf,"<body>\n");
	bprintf(buf,"<p>\n");
	show_link(buf, "/", "Home");
	show_link(buf, "/a/b", "Path");
	bprintf(buf,"<p>\n");
	bprintf(buf,"This is a test.\n");
	bprintf(buf,"</body>\n");

	bprintf(buf,"</html>\n");
	}

// Send the HTML content in the buffer to the file prefaced with the proper
// headers.
static void send_html(buffer *buf, int fd)
	{
	dprintf(fd,"HTTP/1.1 200 OK\n");
	dprintf(fd,"Content-Type: text/html\n");
	dprintf(fd,"Content-Length: %lu\n",buf->len);
	dprintf(fd,"\n");

	if (0) // LATER elim test code
	{
	size_t remain = buf->size - buf->len;
	fprintf(stderr,"response length = %lu\n",buf->len);
	fprintf(stderr,"remain          = %lu\n",remain);
	printf("length = %lu\n",buf->len); // No effect because stdout is closed.
	}

	bsend(buf,fd);
	}

static void write_response(buffer *buf, int fd)
	{
	compose_page(buf);
	send_html(buf,fd);
	}

// Number of requests to allow per session (inbound socket connection)
static const int max_request = 10;

// Maximum number of seconds to wait for a full request to come in
static const int max_time = 30;

static void do_session(int fd)
	{
	buffer *buf_in = &((buffer){0});
	buffer *buf_out = &((buffer){0});

	for (int i = 0; i < max_request; i++)
		{
		alarm(max_time);
		read_request(fd,buf_in);
		alarm(0);

		if (buf_in->len == 0) // disconnect or timeout
			break;

		if (0) fprintf(stderr,"response %d\n",i); // LATER elim

		write_response(buf_out,fd);
		}

	if (0) fprintf(stderr,"== end of session\n"); // LATER elim

	buf_clear(buf_in);
	buf_clear(buf_out);
	}

int main(int argc, const char *argv[])
	{
	return run_server(argc, argv, "127.0.0.1", 9722, do_session);
	}
