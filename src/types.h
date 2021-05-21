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
	void (* on_error)(struct NlexHandle * nh , NlexErr errno );
	void (* on_consume)(struct NlexHandle * nh );
	void * userdata;
	FILE * fp;
	char * buf;
	char * bufptr;
	char * bufendptr;
	size_t lastmatchat;
	_Bool eof_read;
	char * auxbuf;
	char * auxbufptr;
	char * auxbufend;
	unsigned int curstate;
	unsigned int last_accepted_state;
	unsigned int * tstack ;
	size_t tstack_top;
	size_t tstack_allocsiz;
	unsigned int * nstack ;
	size_t nstack_top;
	size_t nstack_allocsiz;
	_Bool done;
	void * data;
} NlexHandle;

static inline void NlexHandle_construct(NlexHandle * this)
{
}

#endif /* _N96E_LEX_TYPES_H */
