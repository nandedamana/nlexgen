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

extern char         ch;

#define TOKBUF_UNIT 32

extern char   * tokbuf;
extern char   * tokbufptr;
extern char   * tokbufendptr;
extern size_t   tokbuflen;    /* Actual current buffer length   */

extern const char escin [];
extern const char escout[];

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

	/* Useful for line counting, col counting, etc. */
	#ifdef nlex_while_getchar
		nlex_while_getchar
	#endif

	/* Now return the character */
	return *bufptr++;
}

static inline int nlex_get_escin(char ch)
{
	const char *ptr;

	for(ptr = escout; *ptr; ptr++)
		if(ch == *ptr)
			return escin[(ptr - escout)];

	return -1;
}

static inline int nlex_get_escout(char ch)
{
	const char *ptr;

	for(ptr = escin; *ptr; ptr++)
		if(ch == *ptr)
			return escout[(ptr - escin)];

	return -1;
}

static inline void nlex_init()
{
	bufptr = buf;

	/* Precalculate for efficient later comparisons.
	 * (buf + BUFLEN) actually points to the first byte next to the end of the buffer.
	*/
	bufendptr = buf + BUFLEN;	
}

static inline void nlex_tokbuf_append(const char ch) {
	/* bufendptr is out of bound, so no memory wastage */
	if(tokbufptr == tokbufendptr) {
		tokbuflen += TOKBUF_UNIT;
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

	*tokbufptr++ = ch;
}

// TODO enable custom error when nlex_tokbuf_append allows
static inline void nlex_tokrec()
{
	nlex_tokbuf_append(ch);
}

/* Initialize the token buffer */
// TODO option to handle err here itself
static inline int nlex_tokrec_init()
{
	/* calloc() may ensure automatic null-termination,
	 * but later realloc() won't. Hence malloc().
	 */
	tokbuf = malloc(TOKBUF_UNIT);
	if(!tokbuf)
		return 1;

	tokbufptr = tokbuf;
	tokbuflen = TOKBUF_UNIT;

	/* Precalculate for efficient comparison */
	tokbufendptr = tokbuf + TOKBUF_UNIT;

	return 0;
}

/* Just null-termination; no memory cleanup. */
static inline int nlex_tokrec_finish()
{
	*tokbufptr = '\0';
}
#endif
