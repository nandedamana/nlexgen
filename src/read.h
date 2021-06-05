/* read.h
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020, 2021 Nandakumar Edamana
 * Started on 2019-07-22
 */

#ifndef _N96E_LEX_READ_H
#define _N96E_LEX_READ_H

#include <assert.h>
#include <limits.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "types.h"

#define NLEX_DEFT_BUF_ALLOC_UNIT 4096

// TODO make the latter number the maximum expected concurrent states
#define NLEX_STATESTACK_ALLOC_UNIT 32

/* Because EOF can be any value and writing down a constant here can
 * cause confusion with EOF.
 * -1 because EOF is already -ve and +N may make it some ASCII character.
 */
#define NAN_NOMATCH (EOF - 1)

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

static inline NlexNString
	nlex_bufdup_info(NlexHandle * nh, size_t offset, size_t len)
{
	NlexNString ns;

	assert(nh->bufptr >= nh->buf);

	ns.buf = nh->buf + offset;
	ns.len = (len > 0)? len: (nh->bufptr - nh->buf);

	return ns;
}

static inline char *
	nlex_bufdup(NlexHandle * nh, size_t offset, size_t len)
{
	NlexNString ns = nlex_bufdup_info(nh, offset, len);

	char * newbuf = nlex_malloc(nh, ns.len + 1);
	memcpy(newbuf, ns.buf, ns.len);
	newbuf[ns.len] = '\0';
	
	return newbuf;
}

static inline char *
	nlex_tokdup(NlexHandle * nh, size_t offset, size_t rtrimlen)
{
	assert(nh->curtoklen > 0);
	return nlex_bufdup(nh,
		nh->curtokpos + offset,
		nh->curtoklen - offset - rtrimlen);
}

static inline NlexNString
	nlex_tokdup_info(NlexHandle * nh, size_t offset, size_t rtrimlen)
{
	assert(nh->curtoklen > 0);
	return nlex_bufdup_info(nh,
		nh->curtokpos + offset,
		nh->curtoklen - offset - rtrimlen);
}

/**
 * @param free_tokbuf Usually false because you might have copied tokbuf without strcpy() or strdup()
 */
static inline void nlex_destroy(NlexHandle * nh)
{
	/* nh->fp is NULL means buf was given by the user and should not be freed. */
	if(nh->fp)
		free(nh->buf);

	free(nh->tstack);
	free(nh->nstack);

	free(nh);
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

/* Call after setting nh->nstack_top but before the actual push */
static inline void nlex_nstack_resize_if_needed(NlexHandle * nh)
{
	if(nh->nstack_top >= nh->nstack_allocsiz) {
		nh->nstack_allocsiz += NLEX_STATESTACK_ALLOC_UNIT;
		nh->nstack =
			nlex_realloc(nh, nh->nstack, sizeof(NanTreeNodeId) * nh->nstack_allocsiz);
	}
}

static inline void nlex_nstack_push(NlexHandle * nh, NanTreeNodeId id)
{
	nh->nstack_top++;
	nlex_nstack_resize_if_needed(nh);
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

/* Behaviour:
 * On EOF, sets nh->eof_read to 1, appends the buf with nullchar, and returns EOF.
 * Subsequent calls to nlex_last(nh) will return 0 and nlex_next() will return EOF.
 * on_next callback is not called for EOF (even for the first time).
 */
static inline int nlex_next(NlexHandle * nh)
{
	_Bool  eof_read = 0;
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

void nlex_onerror(NlexHandle * nh, int errno);

static inline void nlex_reset_states(NlexHandle * nh)
{
	nh->tstack_allocsiz = 0;
	nh->tstack_top = 0;

	nh->nstack_allocsiz = 0;
	nh->nstack_top = 0;

	nh->done       = 0; // TODO needed?
	nh->last_accepted_state = 0;
}

/* Flush the left part of the buffer so that nh->buf starts with the same
 * character currently pointed by nh->bufptr.
 */
static inline void nlex_shift(NlexHandle * nh)
{
	assert(nh->bufptr >= nh->buf);

	size_t chars_remaining    = nh->bufendptr - nh->bufptr;
	size_t flushed_char_count = nh->bufptr - nh->buf;
	
	memmove(nh->buf, nh->bufptr, chars_remaining);
	nh->buf = nlex_realloc(nh, nh->buf, chars_remaining);
	
	nh->bufptr    = nh->buf;
	nh->bufendptr = nh->buf + chars_remaining; /* Yes, just out of bound. */

	nh->curtokpos   -= flushed_char_count;
}

static inline void nlex_swap_t_n_stacks(NlexHandle * nh)
{
	NanTreeNodeId * ptmp;
	size_t          stmp;
	size_t          ttmp;

	ptmp = nh->tstack;
	stmp = nh->tstack_allocsiz;
	ttmp = nh->tstack_top;
	
	nh->tstack     = nh->nstack;
	nh->tstack_allocsiz = nh->nstack_allocsiz;
	nh->tstack_top = nh->nstack_top;
	
	nh->nstack     = ptmp;
	nh->nstack_top = ttmp;
	nh->nstack_allocsiz = stmp;
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

/* Call after popping and setting nh->tstack_top */
static inline void nlex_tstack_resize_if_needed(NlexHandle * nh)
{
	if( (nh->tstack_allocsiz - nh->tstack_top) > NLEX_STATESTACK_ALLOC_UNIT ) {
		nh->tstack_allocsiz -= NLEX_STATESTACK_ALLOC_UNIT;
		nh->tstack =
			nlex_realloc(nh, nh->tstack, sizeof(NanTreeNodeId) * nh->tstack_allocsiz);
	}
}

static inline size_t nlex_tstack_pop(NlexHandle * nh)
{
	size_t id = nh->tstack[nh->tstack_top--];
	nlex_tstack_resize_if_needed(nh);

	return id;
}

static inline void nlex_debug_print_bufptr(NlexHandle * nh, FILE * stream, size_t maxlen)
{
	size_t len = 0;

	for(char *ptr = nh->bufptr; ptr < nh->bufendptr && *ptr && len <= maxlen; ptr++, len++)
		fputc(*ptr, stream);
}

#endif
