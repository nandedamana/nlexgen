/* types.h
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020 Nandakumar Edamana
 * File started on 2020-11-28, contains old code.
 */

#ifndef _N96E_LEX_TYPES_H
#define _N96E_LEX_TYPES_H

typedef signed int NlexCharacter; /* Negative values are for special cases */

typedef enum _NlexError {
	NLEX_ERR_NONE = 0,
	NLEX_ERR_MALLOC,
	NLEX_ERR_REALLOC,
	NLEX_ERR_READING
} NlexError;

// TODO avoid redefinition
typedef unsigned int NanTreeNodeId;

typedef struct _NlexHandle {
	/* Parameters that can only be set before calling nlex_init()
	 * (if unset, will be initialized by nlex_init())
	 */
	size_t buf_alloc_unit;
	
	/* Parameters that can be set anytime */
	void (*on_error)(struct _NlexHandle * nh, int errno);
	void (*on_consume)(struct _NlexHandle * nh);
	void *userdata;
	
	/* Set by nlex_init() */
	FILE * fp;
	char * buf;
	
	/* Set/modified by the lexer several times */
	
	char * bufptr;    /* Points to the character in consideration */
	char * bufendptr; /* Where the next block of the input can be appended */
	size_t lastmatchat; /* I originally used `char * lastmatchptr`, but it caused a bug that cost me hours -- because it was set from bufptr and when the entire buf gets relocated, this pointer remained the same (not easy to adjust it either), while in a later stage it had to be assigned to bufptr. */

	_Bool eof_read; /* Set to true if EOF has reached and the buffer was appended with 0 */
	
	/* Auxilliary buffer, user-defined purpose TODO needed actually? */
	char * auxbuf;
	char * auxbufptr; /* Next byte to write */
	char * auxbufend; /* Precalculated for efficient comparison */
	
	NanTreeNodeId curstate;
	NanTreeNodeId last_accepted_state;
	
	/* 'this stack' and 'next stack' (stacks holding the states for
	 * this iteration and the next.
	 * size_t chosen because currently states are identified using compile-time
	 * addresses.
	 */
	NanTreeNodeId * tstack;
	size_t          tstack_top; /* 0 => empty, 1 is the bottom */
	NanTreeNodeId * nstack;
	size_t          nstack_top;

	_Bool  done; // TODO rem?
	
	/* For custom data (will not be initialized to NULL by nlex_*()) */
	void * data;
} NlexHandle;

#endif
