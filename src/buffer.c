#include <stddef.h>

#include <buffer.h>
#include <stdarg.h>
#include <stdlib.h> // realloc
#include <stdio.h>
#include <unistd.h> // read write
#include <util.h> // die

// Grow buffer by delta bytes.
void buf_grow(buffer *buf, size_t delta)
	{
	size_t size = buf->size + delta;
	char *data = realloc(buf->data, size);

	if (0) // LATER elim
	{
	fprintf(stderr, "grow by %lu from %lu to %lu\n", delta, buf->size, size);
	}

	if (data == NULL) die("Out of memory");
	buf->size = size;
	buf->data = data;
	}

// Ensure that the buffer has at least min_size capacity.  If not, grow the
// buffer by min_size.
void buf_need(buffer *buf, const size_t min_size)
	{
	size_t cur_size = buf->size - buf->len; // cur capacity

	if (cur_size < min_size)
		buf_grow(buf,min_size);
	}

// Free buffer contents and reset to empty.
void buf_clear(buffer *buf)
	{
	free(buf->data);

	buf->len = 0;
	buf->size = 0;
	buf->data = 0;
	}

// Print formatted output to buffer.
void bprintf(buffer *buf, const char *format, ...)
	{
	va_list args;
	va_start(args,format);

	{
	const size_t size = 1024; // min capacity to allow for vsnprintf
	buf_need(buf,size);

	{
	char *str = buf->data + buf->len;
	int nb = vsnprintf(str, size, format, args);

	if (nb < 0 || nb >= size)
		{
		fprintf(stderr,"Truncated: size = %lu, nb = %d\n", size, nb);
		die(0);
		}
	buf->len += nb;
	}
	}

	va_end(args);
	}

static void do_write(int fd, const char *buf, size_t count)
	{
	ssize_t nb = write(fd, buf, count);
	if (nb != count)
		die("ERROR on write"); // LATER iterate until write is complete
	}

// Read up to count bytes from the file into the buffer.
void buf_read(buffer *buf, int fd, size_t count)
	{
	buf_need(buf,count);
	{
	char *str = buf->data + buf->len;
	ssize_t nb = read(fd, str, count);
	if (nb > 0)
		{
		buf->len += nb;

		if (0) // LATER elim
		{
		fprintf(stderr, "\nread %ld bytes\n",buf->len);
		if (0) do_write(2,buf->data,buf->len);
		}
		}
	}
	}

// Send all bytes from the buffer to the file and set the length to zero.
void bsend(buffer *buf, int fd)
	{
	do_write(fd, buf->data, buf->len);
	buf->len = 0;
	}
