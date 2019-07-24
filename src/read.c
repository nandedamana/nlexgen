/* read.c used by lexgen as well as the offsprings
 * Nandakumar Edamana
 * Started on 2019-07-22
 */

#include "read.h"

FILE * fpin;
FILE * fpout;

char         buf[BUFLEN];
char       * bufptr;
const char * bufendptr;

#ifndef NLEX_ITSELF
/* Features not used by the lexgen itself */
_Bool    rectok;
char   * tokbuf;
char   * tokbufptr;
char   * tokbufendptr;
size_t   tokbuflen;
#endif
