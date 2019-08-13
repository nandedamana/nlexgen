/* Nandakumar Edamana
 * Started on 2019-07-23
 */

#include "read.h"

#define NAN_TOK_NONE       0
#define NAN_TOK_HELLO      1
#define NAN_TOK_HI         2
#define	NAN_TOK_SEPARATOR  3

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

	while(1) {
		ch = nlex_next(nh);
		
		if(ch == ' ' || ch == '\n' || ch == 0 || ch == EOF) { /* Separator */
			if(token != NAN_TOK_NONE)
					fprintf(fpout, "Token: %d\n", token);
						
			if(ch == 0 || ch == EOF) {
				/* Don't care whether token is set or not */
				break;
			}
			else {
				if(token == NAN_TOK_NONE) {
					fprintf(stderr, "ERROR: Unknown token.\n");
					exit(1);
				}
			}
			
			/* Put two consecutive spaces in the input and
			 * you'll know why I wrote this. 
			 */
			token = NAN_TOK_SEPARATOR;

			continue;
		}		
		
		token = NAN_TOK_NONE;

		#include "lexbranch.c"
	}

	return 0;
}
