/* error.h
 * Nandakumar Edamana
 * Started on 2019-08-14 by moving code from main.c
 */

#ifndef _N96E_LEX_ERROR_H
#define _N96E_LEX_ERROR_H

#include <stdio.h>

static inline void die(const char *msg) {
	fflush(stdout);
	fprintf(stderr, "nlexgen ERROR: %s\n", msg);
	exit(1);
}

#endif
