/* Nandakumar Edamana
 * Started on 2019-07-22
 */

#include <stdio.h>
#include <stdlib.h>

#define BUFLEN 4096

char         buf[BUFLEN];
char       * bufptr;
const char * bufendptr;

FILE *fpin;
FILE *fpout;

static inline char nan_getchar() {
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

int main() {
	fpin  = stdin;
	fpout = stdout;

	bufptr = buf;

	/* Precalculate for efficient later comparisons.
	 * (buf + BUFLEN) actually points to the first byte next to the end of the buffer.
	*/
	bufendptr = buf + BUFLEN;

	char ch;
	while( (ch = nan_getchar()) != 0 && ch != EOF)
		putchar(ch);

	return 0;
}
