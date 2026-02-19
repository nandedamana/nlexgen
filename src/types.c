#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>

#include "types.h"


void nlex_handle_construct(NlexHandle *this)
{
	this->nstack_allocsiz = 0u;
	this->nstack_top = 0u;
	this->nstack = NULL;
	this->tstack_allocsiz = 0u;
	this->tstack_top = 0u;
	this->tstack = NULL;
	this->last_accepted_state = 0u;
	this->curstate = 0u;
	this->eof_read = false;
	this->curtoklen = 0;
	this->curtokpos = 0;
	this->bufendptr = NULL;
	this->bufptr = NULL;
	this->buf = NULL;
	this->fp = NULL;
	this->userdata = NULL;
	this->on_consume = NULL;
	this->on_error = NULL;
	this->buf_alloc_unit = 1024;
}

void nlex_handle_destruct(NlexHandle *this)
{
}

NlexNString nlex_n_string_default()
{
	NlexNString s;
	s.buf = "";
	s.len = 0u;
	return s;
}
