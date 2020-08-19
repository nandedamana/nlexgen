/* Nandakumar Edamana
 * Started on 2019-07-23
 */

#include <ctype.h>
#include "read.h"

#define NAN_TOK_NONE       0
#define NAN_TOK_HI         1
#define NAN_TOK_HELLO      2
#define NAN_TOK_WS      3

size_t line = 1;
size_t col  = 0;

void on_next(NlexHandle * nh);

int main()
{
	FILE * fpout = stdout;
	
	int token;
	char ch;

	NlexHandle * nh;

	nh = nlex_handle_new();
	if(!nh)
		fprintf(stderr, "ERROR: nlex_handle_new() returned NULL.\n");
	
	nlex_init(nh, stdin, NULL);	
	nh->on_consume = on_next;

	while(1) {
		token = NAN_TOK_NONE;

		#include "lexbranch.c"
		if(token != NAN_TOK_NONE) {
			if(token != NAN_TOK_WS) {
				fprintf(fpout, "Token: %d ends at %zu:%zu\n", token, line, col);
//				nlex_shift(nh);
			}
		}
		else {
			fprintf(stderr, "ERROR: Unknown token.\n");
			exit(1);
		}
		
		if(ch == 0 || ch == EOF)
			break;
	}

	return 0;
}

void on_next(NlexHandle * nh)
{
if(0)
fprintf(stdout, "called %c (%d) nh->lastmatchat = %d - %c\n",
	*(nh->bufptr) == '\n'? '*': *(nh->bufptr),
	*(nh->bufptr),
	nh->lastmatchat,
	nh->buf[nh->lastmatchat]);
	
	switch(nlex_last(nh)) {
	case '\n':
		line++;
		col = 0;
	default:
		col++;
	}
}
