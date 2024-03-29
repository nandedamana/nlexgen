#ifndef _N96E_LEX_TYPES_H
#define _N96E_LEX_TYPES_H

typedef int NlexCharacter;
typedef enum NlexErr {
	NLEX_ERR_NONE = 0,
	NLEX_ERR_MALLOC,
	NLEX_ERR_REALLOC,
	NLEX_ERR_READING,
} NlexErr;

typedef struct NlexNString {
	const char * buf;
	size_t len;
} NlexNString;

typedef unsigned int NanTreeNodeId;
typedef struct NlexHandle {
	size_t buf_alloc_unit;
	void (*on_error)(struct NlexHandle *nh, NlexErr err);
	void (*on_consume)(struct NlexHandle *nh, size_t offset, size_t len);
	void * userdata;
	FILE * fp;
	char * buf;
	char * bufptr;
	char * bufendptr;
	int curtokpos;
	int curtoklen;
	_Bool eof_read;
	unsigned int curstate;
	unsigned int last_accepted_state;
	unsigned int *tstack;
	size_t tstack_top;
	size_t tstack_allocsiz;
	unsigned int *nstack;
	size_t nstack_top;
	size_t nstack_allocsiz;
} NlexHandle;

void nlex_handle_construct(NlexHandle *this);
void nlex_handle_destruct(NlexHandle *this);
NlexNString nlex_n_string_default();

#endif /* _N96E_LEX_TYPES_H */
