/* read.h
 * Nandakumar Edamana
 * Started on 2019-07-22
 */

#ifndef _N96E_LEX_READ_H
#define _N96E_LEX_READ_H

#include <stdio.h>
#include <stdlib.h>

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
	
	/* bufptr usually points to the next character to read,
	 * but not while at the end of the buffer due to the cyclic nature.
	 */
	char * bufptr;

	char * bufendptr;
	
	/* Set by nlex_tokrec_init() */
	char * tokbuf;       /* Not cyclic */
	char * tokbufptr;
	char * tokbufendptr;

	/* Set by the lexer */
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
	if(!newptr)
		nh->on_error(nh, NLEX_ERR_MALLOC);
	
	return newptr;
}

/* Sets errno and calls the error handling function on error */
static inline void * nlex_realloc(NlexHandle * nh, void * ptr, size_t size)
{
	void * newptr;
	
	newptr = realloc(ptr, size);
	if(!newptr)
		nh->on_error(nh, NLEX_ERR_REALLOC);
	
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
		(const signed int c, const signed int * list1, const signed int * list2)
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

char nlex_next(NlexHandle * nh);

void nlex_onerror(NlexHandle * nh, int errno);

void nlex_tokbuf_append(NlexHandle * nh, const char c);

static inline void nlex_tokrec(NlexHandle * nh)
{
	nlex_tokbuf_append(nh, nlex_last(nh));
}

/* Initialize the token buffer */
void nlex_tokrec_init(NlexHandle * nh);

/* Just null-termination; no memory cleanup. */
static inline void nlex_tokrec_finish(NlexHandle * nh)
{
	*(nh->tokbufptr) = '\0';
}
#endif
