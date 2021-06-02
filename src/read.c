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
const NlexCharacter escin [] = {'a',  'b',  'f',  'n',  'r',  't',  'v',  '\\', '\'', '"', '\?', '0',  '(', ')', '|', '[', ']', '^', '.', '*', '+', 'd', 'l', 'w', 'Z',  NAN_NOMATCH};
const NlexCharacter escout[] = {'\a', '\b', '\f', '\n', '\r', '\t', '\v', '\\', '\'', '"', '\?', '\0', '(', ')', '|', '[', ']', '^', '.', '*', '+', -NLEX_CASE_DIGIT, -NLEX_CASE_LETTER, -NLEX_CASE_WORDCHAR, -NLEX_CASE_EOF, NAN_NOMATCH};
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
