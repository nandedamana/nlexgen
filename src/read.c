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

char         ch;

#ifdef NLEX_ITSELF
/* Includes the escaping of special chars used by the lexgen */
const char escin [] = {'a',  'n',  'r',  't',  'v',  '\\', '"', '#', 0};
const char escout[] = {'\a', '\n', '\r', '\t', '\v', '\\', '"', '#', 0};

#else
const char escin [] = {'a',  'n',  'r',  't',  'v',  '\\', '"', 0};
const char escout[] = {'\a', '\n', '\r', '\t', '\v', '\\', '"', 0};

/* Features not used by the lexgen itself */
_Bool    rectok;
char   * tokbuf;
char   * tokbufptr;
char   * tokbufendptr;
size_t   tokbuflen;
#endif
