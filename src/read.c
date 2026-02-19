/* read.c
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020, 2021, 2026 Nandakumar Edamana
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
	NlexHandle *nh = malloc(sizeof(NlexHandle));
	if(!nh)
		return NULL;
	return nh;
}

void nlex_init(NlexHandle * nh, FILE * fpi, const char * buf)
{
	nlex_handle_construct(nh);
	nh->on_error = nlex_onerror;
	nh->buf_alloc_unit = NLEX_DEFT_BUF_ALLOC_UNIT;

	/* These two assignments not put inside some branching because
	 * the one that is not chosen has to be initialized to NULL
	 * and these two will do that automatically.
	 */
	nh->fp          = fpi;
	/* Casting is safe because I know I don't misuse it. */
	nh->buf         = (char *) buf;

	nh->bufptr      = nh->buf - 1;
	nh->bufendptr   = nh->buf; /* Makes nlex_next() read if fp != NULL */
	nh->curtokpos   = -1;
}

void nlex_onerror(NlexHandle * nh, NlexErr errno)
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
