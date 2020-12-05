/* error.h
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020 Nandakumar Edamana
 * File started on 2020-12-05.
 */

#ifndef _N96E_LEX_ERROR_H
#define _N96E_LEX_ERROR_H

#include <stdarg.h>
#include <stdio.h>

static inline void nlex_die(const char * fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	fprintf(stderr, "nlexgen error: ");
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}


extern const char * NLEXERR_SUCCESS;
extern const char * NLEXERR_CLOSING_NO_LIST;
extern const char * NLEXERR_DOT_INSIDE_LIST;
extern const char * NLEXERR_INVERTING_NO_LIST;
extern const char * NLEXERR_KLEENE_PLUS_NOTHING;
extern const char * NLEXERR_KLEENE_STAR_NOTHING;
extern const char * NLEXERR_LIST_INSIDE_LIST;
extern const char * NLEXERR_LIST_NOT_CLOSED;
extern const char * NLEXERR_NO_ACT_GIVEN;
extern const char * NLEXERR_UNKNOWN_ESCSEQ;
#endif
