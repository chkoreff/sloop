#include <sloop.h>
#include <stdio.h>
#include <stdlib.h> // exit
#include <unistd.h> // read alarm

// Number of requests to allow per session (inbound socket connection)
const int max_request = 10;

// Maximum number of seconds to wait for a full request to come in
const int max_time = 30;

static char g_data[4096];
static ssize_t g_len;

static void read_request(void)
	{
	g_len = read(0,g_data,sizeof(g_data));
	if (g_len <= 0)
		exit(0);
	// fprintf(stderr, "read %ld bytes\n",g_len);
	}

/*
Here is a sample page that requires knowing the content length up front.

LATER 20230419
I'll work on library functions that support both protocols:

P1: Content-Length encoding
P2: Transfer-Encoding: chunked

To support P1 I would need to run the response function in a separate process
and buffer up its entire output, which would not include the Content-Length
header.  Then I would assess the length of the content after the headers and
insert the Content-Length as I send it back to the client.

To support P2 I would also run the response function in a separate process, but
in this case I would buffer the output in somewhat large chunks and send them
out with the proper Transfer-Encoding chunk delimiters when each chunk fills up
and at end of transmission.
*/
static void write_response(void)
	{
	int length = 257;
	printf("HTTP/1.1 200 OK\n");
	printf("Content-Type: text/html\n");
	printf("Content-Length: %d\n",length);
	printf("\n");
	printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">\n");
	printf("<html>\n");
	printf("<head>\n");
	printf("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n");
	printf("<meta http-equiv=\"Content-Language\" content=\"en-us\">\n");
	printf("<title>Test</title>\n");
	printf("</head>\n");
	printf("<body>\n");
	printf("<p>\n");
	printf("This is a test.\n");
	printf("</body>\n");
	printf("</html>\n");
	}

static void do_session(void)
	{
	for (int i = 0; i < max_request; i++)
		{
		alarm(max_time);
		read_request();
		alarm(0);

		write_response();
		}
	}

int main(int argc, const char *argv[])
	{
	return run_server(argc, argv, "127.0.0.1", 9722, do_session);
	}
