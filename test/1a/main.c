/* Nandakumar Edamana
 * Started on 2019-07-23
 */

#include "read.h"

#define NAN_TOK_NONE       0
#define NAN_TOK_HELLO      1
#define NAN_TOK_HI         2
#define NAN_TOK_CSTAR      3
#define NAN_TOK_LSTAR      4

int main()
{
	fpout = stdout;
	
	int token;
	char ch;

	NlexHandle * nh;

	nh = nlex_handle_new();
	if(!nh)
		fprintf(stderr, "ERROR: nlex_handle_new() returned NULL.\n");
	
	nlex_init(nh, stdin, NULL);	

	while( (ch = nlex_next(nh)) && (ch != EOF) ) {
		token = NAN_TOK_NONE;

		#include "lexbranch.c"
		
		if(token == NAN_TOK_NONE) {
			fprintf(stderr, "ERROR: Unknown token; quitting while ch = %d.\n", ch);
			exit(1);
		}

		fprintf(stdout, "Token: %d.\n", token);
	};

	return 0;
}
