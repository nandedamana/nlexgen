/* error.c
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020 Nandakumar Edamana
 * File started on 2020-12-05.
 */

const char * NLEXERR_SUCCESS = "success";
const char * NLEXERR_CLOSING_NO_LIST = "closing a list that was never open";
const char * NLEXERR_DOT_INSIDE_LIST = "dot wildcard is not permitted inside lists";
const char * NLEXERR_INVERTING_NO_LIST = "inverting a list that was never open";
const char * NLEXERR_KLEENE_PLUS_NOTHING = "Kleene plus without any preceding character";
const char * NLEXERR_KLEENE_STAR_NOTHING = "Kleene star without any preceding character";
const char * NLEXERR_LIST_INSIDE_LIST = "list inside list";
const char * NLEXERR_LIST_NOT_CLOSED = "list opened but not closed";
const char * NLEXERR_NO_ACT_GIVEN = "no action given for a token";
const char * NLEXERR_UNKNOWN_ESCSEQ = "unknown escape sequence";
