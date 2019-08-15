/* read.c used by lexgen as well as the offsprings
 * Nandakumar Edamana
 * Started on 2019-07-22
 */

#include "read.h"

/* NLEX_ITSELF can be set using the gcc option -D */
#ifdef NLEX_ITSELF
FILE * fpout;

/* Includes the escaping of special chars used by the lexgen */
const NlexCharacter escin [] = {'a',  'b',  'f',  'n',  'r',  't',  'v',  '\\', '\'', '"', '\?', '0',  '#', '[', ']', 'Z',  NAN_NOMATCH};
const NlexCharacter escout[] = {'\a', '\b', '\f', '\n', '\r', '\t', '\v', '\\', '\'', '"', '\?', '\0', '#', '[', ']',  EOF, NAN_NOMATCH};
#endif

/* For C output */
const NlexCharacter escin_c [] = {'a',  'b',  'f',  'n',  'r',  't',  'v',  '\\', '\'', '"', '\?', '0',  NAN_NOMATCH};
const NlexCharacter escout_c[] = {'\a', '\b', '\f', '\n', '\r', '\t', '\v', '\\', '\'', '"', '\?', '\0', NAN_NOMATCH}; /* Not terminating with 0 because it is in the list */

NlexHandle * nlex_handle_new()
{
	NlexHandle * nh;
	
	nh = malloc(sizeof(NlexHandle));
	if(!nh)
		return NULL;
	
	nh->buflen      = NLEX_DEFT_BUFSIZE;
	nh->tokbuf_unit = NLEX_DEFT_TBUF_UNIT;
	
	nh->tokbuf      = NULL;
	
	nh->on_error    = nlex_onerror;
	nh->on_next     = NULL;
	
	return nh;
}

void nlex_init(NlexHandle * nh, FILE * fpi, const char * buf)
{
	nh->fp          = fpi;

	/* Casting is safe because I know I don't misuse it. */
	nh->buf         = (char *) buf;

	if(fpi) {
		nh->buf       = nlex_malloc(nh, sizeof(NlexHandle));

		/* Precalculate for efficient later comparisons.
		 * (buf + BUFLEN) actually points to the first byte next to the end of the buffer.
		*/
		nh->bufendptr = nh->buf + nh->buflen;	
	}

	nh->bufptr      = nh->buf;
}

int nlex_next(NlexHandle * nh)
{
	/* If fp is NULL, bufptr is assumed to be pointed to a pre-filled buffer.
	 * This helps tokenize strings directly.
	 */
	if(nh->fp) {
		/* Cyclic buffer
		 * (bufendptr is out of bound, so no memory wastage due to the following 
		 * comparison)
		 */
		if(nh->bufptr == nh->bufendptr) {
			/* This is the best place to check */
			if(feof(nh->fp))
				return EOF;

			nh->bufptr = nh->buf;
		}

		/* Pointer at the beginning of the buffer means we have to read */
		if(nh->bufptr == nh->buf) {
			size_t bytes_read = fread(nh->buf, 1, nh->buflen, nh->fp);
			if(ferror(nh->fp))
				nh->on_error(nh, NLEX_ERR_READING);
			
			/* Again, points to the memory location next to the last character. */
			nh->bufendptr = nh->buf + bytes_read;
		}
	}

	/* Useful for line counting, col counting, etc. */
	if(nh->on_next)
		nh->on_next(nh);

	/* Now return the character */
	return *(nh->bufptr++);
}

void nlex_onerror(NlexHandle * nh, int errno)
{
	switch(errno) {
	case NLEX_ERR_MALLOC:
	case NLEX_ERR_REALLOC:
		fprintf(stderr, "nlex: malloc()/realloc() error.\n");
		break;
	case NLEX_ERR_READING:
		fprintf(stderr, "nlex: error reading input.\n");
		break;
	default:
		fprintf(stderr, "nlex: unknown error.\n");
		break;
	}
	
	exit(1);
}

void nlex_tokbuf_append(NlexHandle * nh, const char c) {
	/* bufendptr is out of bound, so no memory wastage */
	if(nh->tokbufptr == nh->tokbufendptr) {
		nh->tokbuflen += nh->tokbuf_unit;
		nh->tokbuf     = nlex_realloc(nh, nh->tokbuf, nh->tokbuflen);
		
		nh->tokbufendptr = nh->tokbuf + nh->tokbuflen;
		
		/* Resetting nh->tokbufptr is a must after realloc() since the buffer might
		 * have been relocated.
		 */
		nh->tokbufptr  = nh->tokbufendptr - nh->tokbuf_unit;
	}

	*(nh->tokbufptr++) = c;
}

void nlex_tokrec_init(NlexHandle * nh)
{
	/* calloc() may ensure automatic null-termination,
	 * but later realloc() won't. Hence malloc().
	 */
	nh->tokbuf = nlex_malloc(nh, nh->tokbuf_unit);

	nh->tokbufptr = nh->tokbuf;
	nh->tokbuflen = nh->tokbuf_unit;

	/* Precalculate for efficient comparison */
	nh->tokbufendptr = nh->tokbuf + nh->tokbuf_unit;
}
