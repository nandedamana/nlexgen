/* Nandakumar Edamana
 * Started on 2019-07-23
 */

#include "read.h"

enum NanState {
	NAN_STATE_DEFAULT,
	NAN_STATE_COMMENT,
	NAN_STATE_STRING,
	NAN_STATE_STR_ESC
};

/* XXX Idea! 2019-07-04 morning.
 * NAN_TOKSTART_family and NAN_TOKEND_family makes family detection easy;
 * no need of two separate variables to store the family and the token.
 */
enum NanToken {
	NAN_TOK_NONE,

	NAN_TOKSTART_KW,
	NAN_KW_CHAR,
	NAN_KW_CONST,
	NAN_TOKEND_KW,

	NAN_TOK_COMMENT,	
	NAN_TOK_STRING,
	
	NAN_TOK_SEPARATOR
};

int main()
{
	fpin  = stdin;
	fpout = stdout;

	nlex_init();
	
	int context = NAN_STATE_DEFAULT;
	int token;

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

		if(context == NAN_STATE_DEFAULT) {
			token = NAN_TOK_NONE;

			#include "out/lexbranch-default.c"
		}
		else if(context == NAN_STATE_COMMENT) {
			#include "out/lexbranch-comment.c"
		}
		else if(context == NAN_STATE_STRING) {
			#include "out/lexbranch-string.c"
		}
		else if(context == NAN_STATE_STR_ESC) {
			char escoutch = nlex_get_escout(ch);
			if(escoutch != -1) {
				nlex_tokbuf_append(escoutch);
				context = NAN_STATE_STRING;
			}
			else {
				fprintf(stderr, "ERROR: Unknown escape sequence.\n");
				exit(1);
			}
		}
		
		/* XXX I cannot check the value of `token` until I encounter a separator.
		 * Checking here may work, but it'll work for prefixes also
		 * (e.g.: 'character' confused for 'char').
		 */
	}

	return 0;
}
