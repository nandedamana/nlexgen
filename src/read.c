/* read.c
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020 Nandakumar Edamana
 * Started on 2019-07-22
 */

#include "read.h"
#include "tree.h"

/* NLEX_ITSELF can be set using the gcc option -D */
#ifdef NLEX_ITSELF
FILE * fpout;

/* Includes the escaping of special chars used by the lexgen */
const NlexCharacter escin [] = {'a',  'b',  'f',  'n',  'r',  't',  'v',  '\\', '\'', '"', '\?', '0',  '[', ']', '^', '.', '*', '+', 'd', 'w', 'Z',  NAN_NOMATCH};
const NlexCharacter escout[] = {'\a', '\b', '\f', '\n', '\r', '\t', '\v', '\\', '\'', '"', '\?', '\0', '[', ']', '^', '.', '*', '+', -NLEX_CASE_DIGIT, -NLEX_CASE_WORDCHAR, -NLEX_CASE_EOF, NAN_NOMATCH};
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
	
	nh->buf_alloc_unit = NLEX_DEFT_BUF_ALLOC_UNIT;
	
	nh->on_error       = nlex_onerror;
	nh->on_consume     = NULL;
	nh->userdata       = NULL;
	
	return nh;
}

void nlex_init(NlexHandle * nh, FILE * fpi, const char * buf)
{
	/* These two assignments not put inside some branching because
	 * the one that is not chosen has to be initialized to NULL
	 * and these two will do that automatically.
	 */
	nh->fp          = fpi;
	/* Casting is safe because I know I don't misuse it. */
	nh->buf         = (char *) buf;

	nh->bufptr      = nh->buf - 1;
	nh->bufendptr   = nh->buf; /* Makes nlex_next() read if fp != NULL */
	
	nh->eof_read    = 0;
	
	nh->auxbuf      = NULL;

	nh->tstack      = NULL;
	nh->nstack      = NULL;
}

/* Behaviour:
 * On EOF, sets nh->eof_read to 1, appends the buf with nullchar, and returns EOF.
 * Subsequent calls to nlex_last(nh) will return 0 and nlex_next() will return EOF.
 * on_next callback is not called for EOF (even for the first time).
 */
int nlex_next(NlexHandle * nh)
{
	#ifdef DEBUG
	fprintf(stderr, "nlex_next() called.\n");
	#endif

	_Bool  eof_read   = 0;
	size_t bytes_read;

	if(nh->eof_read)
		return EOF;

	nh->bufptr++;

	/* If fp is NULL, bufptr is assumed to be pointed to a pre-filled buffer.
	 * This helps tokenize strings directly.
	 */
	if(nh->fp && (nh->bufptr == nh->bufendptr)) {
		/* This is the best place to check */
		if(feof(nh->fp)) {
			eof_read   = 1;
			bytes_read = 1; /* To append nullchar */
		}

		/* We have to read more */
		size_t curlen = nh->bufendptr - nh->buf;
		nh->buf       = nlex_realloc(nh, nh->buf, curlen + nh->buf_alloc_unit);

		/* Because realloc() might have relocated the buffer */
		nh->bufptr    = nh->buf + curlen;

		if(!eof_read) {
			bytes_read = fread(nh->bufptr, 1, nh->buf_alloc_unit, nh->fp);
			if(ferror(nh->fp))
				nh->on_error(nh, NLEX_ERR_READING);
		}

		/* Really important. The feof() chech at the top fails to detect the end if the file size is a multiple of buf_alloc_unit and the previous read had consumed the last block, leaving nothing for this call to read. */
		if(bytes_read == 0) {
			eof_read   = 1;
			bytes_read = 1; /* To fill with nullchar */
		}

		/* Really important */
		if(bytes_read < nh->buf_alloc_unit) {
			nh->buf    = nlex_realloc(nh, nh->buf, curlen + bytes_read);
			nh->bufptr = nh->buf + curlen;
		}
		
		/* Points to the memory location next to the last character. */
		nh->bufendptr = nh->bufptr + bytes_read;


		if(eof_read) {
			*(nh->bufptr) = '\0';
			nh->eof_read  = 1;
			return EOF;
		}
	}

	/* Now return the character */
	return *(nh->bufptr);
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
