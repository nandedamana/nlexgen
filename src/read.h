/* read.h
 * Nandakumar Edamana
 * Started on 2019-07-22
 */

#ifndef _N96E_LEX_READ_H
#define _N96E_LEX_READ_H

#include <stdio.h>
#include <stdlib.h>

#define BUFLEN 4096

extern FILE * fpin;
extern FILE * fpout;

extern char         buf[BUFLEN];
extern char       * bufptr;
extern const char * bufendptr;

#ifndef NLEX_ITSELF
/* Features not used by the lexgen itself */

/* Unit size of the token buffer (must be greater than 1;
 * see nlex_start_tokrec())
 */
#define TOKBUF_UNIT 16
// TODO test 1 and 2

extern _Bool    rectok;       /* Flag to enable token recording */
extern char   * tokbuf;
extern char   * tokbufptr;
extern char   * tokbufendptr;
extern size_t   tokbuflen;    /* Actual current buffer length   */
#endif

// TODO return int for err handling?
static inline char nlex_getchar()
{
	/* Cyclic buffer */
	if(bufptr == bufendptr) /* bufendptr is out of bound, so no memory wastage */
		bufptr = buf;

	/* Pointer at the beginning of the buffer means we have to read */
	if(bufptr == buf) {
		fread(buf, BUFLEN, 1, fpin);
		if(ferror(fpin)) { // TODO enable custom error
			fprintf(stderr, "Error reading input.\n");
			exit(1);
		}
	}
	
	#ifndef NLEX_ITSELF
	if(rectok) {
		/* Unlike tokbufptr, tokbufendptr is inside the bound, so that space for
		 * nullchar is always ensured.
		 */
		if(tokbufptr == tokbufendptr) {
			tokbuflen += TOKBUF_UNIT;
			puts("Re");
			tokbuf     = realloc(tokbuf, tokbuflen);
			if(!tokbuf) { // TODO enable custom error
				fprintf(stderr, "realloc() error.\n");
				exit(1);
			}
			
			tokbufendptr = tokbuf + tokbuflen;
			
			/* Resetting tokbufptr is a must after realloc() since the buffer might
			 * have been relocated.
			 */
			tokbufptr    = tokbufendptr - TOKBUF_UNIT;
		}

		*tokbufptr++ = *bufptr;
	}
	#endif
	
	/* Now return the character */
	return *bufptr++;
}

static inline void nlex_init()
{
	bufptr = buf;

	/* Precalculate for efficient later comparisons.
	 * (buf + BUFLEN) actually points to the first byte next to the end of the buffer.
	*/
	bufendptr = buf + BUFLEN;	
	
	#ifndef NLEX_ITSELF
	rectok = 0;
	#endif
}

#ifndef NLEX_ITSELF
/* Start recording the input */
// TODO option to handle err here itself
static inline int nlex_start_tokrec()
{
printf("nlex_start_tokrec() while *bufptr = %c\n", *bufptr);
	rectok = 1;
	tokbuf = calloc(TOKBUF_UNIT, 1); /* calloc() ensures null-termination */
	if(!tokbuf)
		return 1;

	/* Can't wait until the next call to nlex_getchar() or the current char
	 * will be missed.
	 */
	tokbuf[0] = *(bufptr - 1);

	tokbufptr = tokbuf + 1;
	tokbuflen = TOKBUF_UNIT;
	
	/* Precalculate for efficient comparison */
	tokbufendptr = tokbuf + TOKBUF_UNIT - 1;
	
	return 0;
}
#endif

#endif
