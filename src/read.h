/* read.h
 * Nandakumar Edamana
 * Started on 2019-07-22
 */

#ifndef _N96E_LEX_READ_H
#define _N96E_LEX_READ_H

#include <limits.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"

#define NLEX_DEFT_BUF_ALLOC_UNIT 4096

/* Because EOF can be any value and writing down a constant here can
 * cause confusion with EOF.
 * -1 because EOF is already -ve and +N may make it some ASCII character.
 */
#define NAN_NOMATCH (EOF - 1)

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

/* NLEX_ITSELF can be set using the gcc option -D */
#ifdef NLEX_ITSELF
extern FILE * fpout;

extern const NlexCharacter escin [];
extern const NlexCharacter escout[];
#endif

/* Escape sequence mapping for C output */
extern const NlexCharacter escin_c [];
extern const NlexCharacter escout_c[];

/* Sets errno and calls the error handling function on error */
static inline void * nlex_malloc(NlexHandle * nh, size_t size)
{
	void * newptr;
	
	newptr = malloc(size);
	if(!newptr) {
		if(nh)
			nh->on_error(nh, NLEX_ERR_MALLOC);
		else
			nlex_die("malloc() error.");
	}
	
	return newptr;
}

/* Sets errno and calls the error handling function on error */
static inline void * nlex_realloc(NlexHandle * nh, void * ptr, size_t size)
{
	void * newptr;

	newptr = realloc(ptr, size);
	if(!newptr) {
		if(nh)
			nh->on_error(nh, NLEX_ERR_REALLOC);
		else
			nlex_die("realloc() error.");
	}	

	return newptr;
}

static inline void nlex_auxbuf_append(NlexHandle * nh, const char c)
{
	if(nh->auxbufptr == nh->auxbufend) {
		size_t curlen = nh->auxbufend - nh->auxbuf;

		nh->auxbuf    = nlex_realloc(nh, nh->auxbuf, curlen + nh->buf_alloc_unit);

		/* Because realloc() may relocate the buffer */
		nh->auxbufptr = nh->auxbuf + curlen;
		
		nh->auxbufend = nh->auxbufptr + nh->buf_alloc_unit;
	}

	*(nh->auxbufptr++) = c;
}

static inline void nlex_auxbuf_init(NlexHandle * nh)
{
	if(nh->auxbuf == NULL)
		nh->auxbuf = nlex_malloc(nh, nh->buf_alloc_unit);
	
	nh->auxbufptr = nh->auxbuf;
	nh->auxbufend = nh->auxbuf + nh->buf_alloc_unit; /* Next-to-the-last-byte */
}

static inline void nlex_auxbuf_terminate(NlexHandle * nh)
{
	nlex_auxbuf_append(nh, '\0');
}

/* Makes a copy of the buffer upto the last-matched character
 * appended with a nullchar.
 */
static inline char * nlex_bufdup(NlexHandle * nh, size_t offset)
{
	size_t len = nh->bufptr - nh->buf + 2 - offset;

	char * newbuf = nlex_malloc(nh, len);
	memcpy(newbuf, nh->buf + offset, len - 1);
	newbuf[len - 1] = '\0';
	
	return newbuf;
}


/**
 * Call free() on buffers
 * @param free_tokbuf Usually false because you might have copied tokbuf without strcpy() or strdup()
 */
static inline void nlex_destroy(NlexHandle * nh)
{
	/* nh->fp is NULL means buf was given by the user and should not be freed. */
	if(nh->fp)
		free(nh->buf);
}

/* Call nlex_destroy() and then set the pointer to NULL */
static inline void nlex_destroy_and_null(NlexHandle ** nhp)
{
	nlex_destroy(*nhp);
	*nhp = NULL;
}

/* Find the counterpart of c of list1 in list2 */
static inline int
	nlex_get_counterpart
		(const NlexCharacter c, const NlexCharacter * list1, const NlexCharacter * list2)
{
	const signed int *ptr;

	for(ptr = list1; *ptr != NAN_NOMATCH; ptr++)
		if(c == *ptr)
			return list2[(ptr - list1)];

	return NAN_NOMATCH;
}

NlexHandle * nlex_handle_new();

/* Only one of fpi or buf is required, and the other can be NULL. */
void nlex_init(NlexHandle * nh, FILE * fpi, const char * buf);

/* Look at the last-scanned character without moving the pointer */
static inline char nlex_last(NlexHandle * nh)
{
	return *(nh->bufptr);
}

static inline _Bool nlex_nstack_is_empty(NlexHandle * nh)
{
	return (nh->nstack_top == 0);
}

static inline void nlex_nstack_dump(NlexHandle * nh)
{
	int i;

	fprintf(stderr, "nlex nstack [bottom ");

	for(i = 1; i <= nh->nstack_top; i++)
		fprintf(stderr, "%d ", nh->nstack[i]);

	fprintf(stderr, "top]\n");
}

/* Remove low-priority actions and push the remaining one action node to the
 * stack's bottom so that it'll be considered only after all other
 * possibilities.
 */
static inline void nlex_nstack_fix_actions(NlexHandle * nh)
{
	int i;
	int lowpos = 0;
	unsigned long lowid;

	/* Find the action node with the lowest id (higher priority) */
	for(i = 1; i <= nh->nstack_top; i++)
		if(nh->nstack[i] & 1 && nh->nstack[i] < nh->nstack[lowpos])
			lowpos = i;

	/* Mark other action nodes to be avoided */
	if(nh->nstack[lowpos] & 1) { /* This means at least one action was found. */
		for(i = 1; i <= nh->nstack_top; i++)
			if(nh->nstack[i] & 1 && i != lowpos)
				nh->nstack[i] = 0;

		/* Swap the low-prio action with the stack bottom */
		if(lowpos != 0) {
			lowid              = nh->nstack[lowpos];
			nh->nstack[lowpos] = nh->nstack[0];
			nh->nstack[0]      = lowid;
		}
	}
}

static inline void nlex_nstack_push(NlexHandle * nh, NanTreeNodeId id)
{
	nh->nstack_top++;
	nh->nstack =
		nlex_realloc(nh, nh->nstack, sizeof(NanTreeNodeId) * (nh->nstack_top + 1));
	nh->nstack[nh->nstack_top] = id;
}

static inline void nlex_nstack_remove(NlexHandle * nh, NanTreeNodeId id)
{
	int i;

	for(i = 1; i <= nh->nstack_top; i++)
		if(nh->nstack[i] == id)
			/* Make inactive */
			nh->nstack[i] = 0; // TODO better if I can remove
}

int nlex_next(NlexHandle * nh);

void nlex_onerror(NlexHandle * nh, int errno);

static inline void nlex_reset_states(NlexHandle * nh)
{
	nh->tstack_top = 0;
	nh->nstack_top = 0;
	nh->done       = 0; // TODO needed?
	nh->last_accepted_state = 0;
}

/* Flush the left part of the buffer so that nh->buf starts with the same
 * character currently pointed by nh->bufptr.
 */
static inline void nlex_shift(NlexHandle * nh)
{
	size_t chars_remaining    = nh->bufendptr - nh->bufptr;
	size_t flushed_char_count = nh->bufptr - nh->buf;
	
	memmove(nh->buf, nh->bufptr, chars_remaining);
	nh->buf = nlex_realloc(nh, nh->buf, chars_remaining);
	
	nh->bufptr    = nh->buf;
	nh->bufendptr = nh->buf + chars_remaining; /* Yes, just out of bound. */

	nh->lastmatchat -= flushed_char_count;
}

static inline void nlex_swap_t_n_stacks(NlexHandle * nh)
{
	NanTreeNodeId * ptmp;
	size_t          stmp;

	ptmp = nh->tstack;
	stmp = nh->tstack_top;
	
	nh->tstack     = nh->nstack;
	nh->tstack_top = nh->nstack_top;
	
	nh->nstack     = ptmp;
	nh->nstack_top = stmp;
}

static inline void nlex_tstack_dump(NlexHandle * nh)
{
	int i;

	fprintf(stderr, "nlex tstack [bottom ");

	for(i = 1; i <= nh->tstack_top; i++)
		fprintf(stderr, "%d ", nh->tstack[i]);

	fprintf(stderr, "top]\n");
}

static inline _Bool nlex_tstack_has_non_action_nodes(NlexHandle * nh)
{
	int i;

	for(i = 1; i <= nh->tstack_top; i++)
		if((nh->tstack[i] & 1) == 0)
			return 1;
	
	return 0;
}

static inline _Bool nlex_tstack_is_empty(NlexHandle * nh)
{
	return (nh->tstack_top == 0);
}

static inline size_t nlex_tstack_pop(NlexHandle * nh)
{
	size_t id = nh->tstack[nh->tstack_top--];

	if(nh->tstack_top != 0) {
		nh->tstack =
			nlex_realloc(nh, nh->tstack, sizeof(size_t) * (nh->tstack_top + 1));
	}
	else {
		free(nh->tstack);
		nh->tstack = NULL;
	}

	return id;
}

static inline void nlex_debug_print_bufptr(NlexHandle * nh, FILE * stream, size_t maxlen)
{
	size_t len = 0;

	for(char *ptr = nh->bufptr; ptr < nh->bufendptr && *ptr && len <= maxlen; ptr++, len++)
		fputc(*ptr, stream);
}

#endif
