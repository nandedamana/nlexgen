/* Nandakumar Edamana
 * Started on 2019-07-22
 */

#include "read.h"

#define NAN_TOK_NONE  0
#define NAN_TOK_HELLO 1
#define NAN_TOK_HI    2

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

	do {
		token = NAN_TOK_NONE;

		ch = nan_getchar();
		#include "out/lexbranch.c"
		
		if(token == NAN_TOK_NONE) {
			fprintf(stderr, "ERROR: Unknown token.\n");
			exit(1);
		}
		else {
			if(ch == ' ' || ch == '\n' || ch == 0 || ch == EOF) /* Separator */
				fprintf(fpout, "Token detected: %d\n", token);
		}
	} while(ch && ch != EOF);

	return 0;
}
