/* Nandakumar Edamana
 * Started on 2019-07-23
 */

#include <ctype.h>
#include "read.h"

#define NAN_TOK_NONE       0
#define NAN_TOK_HI         1
#define NAN_TOK_HELLO      2
#define NAN_TOK_WS      3

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

	while(1) {
		token = NAN_TOK_NONE;

		#include "lexbranch.c"
		
		if(token != NAN_TOK_NONE) {
			fprintf(fpout, "Token: %d\n", token);
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
