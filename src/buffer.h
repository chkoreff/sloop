typedef struct buffer
	{
	size_t len;
	size_t size;
	char *data;
	} buffer;

extern void buf_grow(buffer *buf, size_t delta);
extern void buf_need(buffer *buf, const size_t min_size);
extern void buf_clear(buffer *buf);
extern void bprintf(buffer *buf, const char *format, ...);
extern void buf_read(buffer *buf, int fd, size_t count);
extern void bsend(buffer *buf, int fd);
