#include <stddef.h>

#include <stdio.h>
#include <stdlib.h> // exit
#include <string.h> // memcpy
#include <util.h>

void die(const char *msg)
	{
	if (msg)
		fprintf(stderr,"%s\n",msg);
	exit(1);
	}

/*
Copy len chars from src into dest at the given pos, followed by a NUL byte.
The size is the total size of the dest array.  The routine dies if the copy
attempts to go out of bounds.
*/
void safe_copy(char *dest, size_t size, size_t pos,
	const char *src, size_t len)
	{
	if (size > len && pos < size - len)
		{
		char *place = dest + pos;
		memcpy(place, src, len);
		place[len] = 0;
		}
	else
		die("safe_copy out of bounds");
	}
