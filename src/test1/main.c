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
	fpin  = stdin;
	fpout = stdout;

	bufptr = buf;

	/* Precalculate for efficient later comparisons.
	 * (buf + BUFLEN) actually points to the first byte next to the end of the buffer.
	*/
	bufendptr = buf + BUFLEN;
	
	int token;
	char ch;

	while(1) {
		ch = nlex_getchar();
		
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

		#include "out/lexbranch.c"
	}

	return 0;
}
