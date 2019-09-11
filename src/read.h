/* read.h
 * Nandakumar Edamana
 * Started on 2019-07-22
 */

#ifndef _N96E_LEX_READ_H
#define _N96E_LEX_READ_H

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"

#define NLEX_DEFT_BUFSIZE   4096
#define NLEX_DEFT_TBUF_UNIT 32

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
	size_t buflen;
	size_t tokbuf_unit;
	
	/* Parameters that can be set anytime */
	void (*on_error)(struct _NlexHandle * nh, int errno);
	void (*on_next) (struct _NlexHandle * nh); /* Called after re-buffering,
	                                            * before pointer update,
	                                            * meaning (*bufptr) is the newly
	                                            * read character.
	                                            */

	/* Set by nlex_init() */
	FILE * fp;
	char * buf;          /* Cyclic buffer */
	
	/* Set/modified by the lexer several times */
	
	/* bufptr usually points to the next character to read,
	 * but not while at the end of the buffer due to the cyclic nature.
	 */
	char * bufptr;
	char * bufendptr;
	
	size_t curstate;
	
	/* 'this stack' and 'next stack' (stacks holding the states for
	 * this iteration and the next.
	 * size_t chosen because currently states are identified using compile-time
	 * addresses.
	 */
	NanTreeNodeId * tstack;
	size_t          tstack_top;
	NanTreeNodeId * nstack;
	size_t          nstack_top;
	
	/* Set by nlex_tokrec_init() and modified during runtime */
	char * tokbuf;       /* Not cyclic */
	char * tokbufptr;
	char * tokbufendptr;
	size_t tokbuflen;
	
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

/* XXX This should not be called twice without a nlex_next() in between. */
static inline void nlex_back(NlexHandle * nh)
{
	/* Trust me, it works. */
	nh->bufptr--;
}

/**
 * Call free() on buffers
 * @param free_tokbuf Usually false because you might have copied tokbuf without strcpy() or strdup()
 */
static inline void nlex_destroy(NlexHandle * nh, _Bool free_tokbuf)
{
	/* nh->fp is NULL means buf was given by the user and should not be freed. */
	if(nh->fp)
		free(nh->buf);

	if(nh->tokbuf && free_tokbuf)
		free(nh->tokbuf);
}

/* Call nlex_destroy() and then set the pointer to NULL */
static inline void nlex_destroy_and_null(NlexHandle ** nhp, _Bool free_tokbuf)
{
	nlex_destroy(*nhp, free_tokbuf);
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

/* Look at the last-read character without moving the pointer */
static inline char nlex_last(NlexHandle * nh)
{
	return *(nh->bufptr - 1);
}

static inline _Bool nlex_nstack_is_empty(NlexHandle * nh)
{
	return (nh->nstack_top == -1);
}

static inline void nlex_nstack_dump(NlexHandle * nh)
{
	int i;

	fprintf(stderr, "nlex nstack [bottom ");

	for(i = 0; i <= nh->nstack_top; i++)
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
	for(i = 0; i <= nh->nstack_top; i++)
		if(nh->nstack[i] & 1 && nh->nstack[i] < nh->nstack[lowpos])
			lowpos = i;

	/* Mark other action nodes to be avoided */
	if(nh->nstack[lowpos] & 1) { /* This means at least one action was found. */
		for(i = 0; i <= nh->nstack_top; i++)
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

static inline void nlex_nstack_push(NlexHandle * nh, size_t id)
{
	nh->nstack_top++;
	nh->nstack =
		nlex_realloc(nh, nh->nstack, sizeof(size_t) * (nh->nstack_top + 1));
	nh->nstack[nh->nstack_top] = id;
}

int nlex_next(NlexHandle * nh);

void nlex_onerror(NlexHandle * nh, int errno);

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

void nlex_tokbuf_append(NlexHandle * nh, const char c);

static inline void nlex_tokrec(NlexHandle * nh)
{
	nlex_tokbuf_append(nh, nlex_last(nh));
}

/* Initialize the token buffer */
void nlex_tokrec_init(NlexHandle * nh);

static inline void nlex_tokrec_back(NlexHandle * nh)
{
	if(nh->tokbufptr > nh->tokbuf) {
		size_t pos = (nh->tokbufptr - nh->tokbuf);

		nh->tokbuf       = nlex_realloc(nh, nh->tokbuf, pos);
		nh->tokbufptr    = nh->tokbuf + pos;
		nh->tokbufendptr = nh->tokbuf + nh->tokbuflen;

		*(nh->tokbufptr) = '\0';
	}
}

/* Just null-termination; no memory cleanup. */
static inline void nlex_tokrec_finish(NlexHandle * nh)
{
	*(nh->tokbufptr) = '\0';
}

static inline _Bool nlex_tstack_has_non_action_nodes(NlexHandle * nh)
{
	int i;

	for(i = 0; i <= nh->tstack_top; i++)
		if((nh->tstack[i] & 1) == 0)
			return 1;
	
	return 0;
}

static inline _Bool nlex_tstack_is_empty(NlexHandle * nh)
{
	return (nh->tstack_top == -1);
}

static inline size_t nlex_tstack_pop(NlexHandle * nh)
{
	size_t id = nh->tstack[nh->tstack_top--];

	if(nh->tstack_top != -1) {
		nh->tstack =
			nlex_realloc(nh, nh->tstack, sizeof(size_t) * (nh->tstack_top + 1));
	}
	else {
		free(nh->tstack);
		nh->tstack = NULL;
	}

	return id;
}
#endif
