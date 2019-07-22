/* Nandakumar Edamana
 * Started on 2019-07-22
 */

#include <stdio.h>
#include <stdlib.h>

#define BUFLEN 4096

extern FILE * fpin;
extern FILE * fpout;

extern char         buf[BUFLEN];
extern char       * bufptr;
extern const char * bufendptr;

static inline char nan_getchar()
{
	/* Cyclic buffer */
	if(bufptr == bufendptr)
		bufptr = buf;

	/* Pointer at the beginning of the buffer means we have to read */
	if(bufptr == buf) {
		fread(buf, BUFLEN, 1, fpin);
		if(ferror(fpin)) {
			fprintf(stderr, "Error reading input.\n");
			exit(1);
		}
	}
	
	/* Now return the character */
	return *(bufptr++);
}
