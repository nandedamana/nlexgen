/* Nandakumar Edamana
 * Started on 2019-07-23
 * Major changes on 2019-09-06
 */

#include "read.h"

int main()
{
	fpout = stdout;
	
	char ch;

	NlexHandle * nh;

	nh = nlex_handle_new();
	if(!nh)
		fprintf(stderr, "ERROR: nlex_handle_new() returned NULL.\n");
	
	nlex_init(nh, stdin, NULL);	

	nlex_tokrec_init(nh);

	while(1) {
		ch = nlex_next(nh);
		nlex_tokrec(nh);
		
		if(ch == ' ' || ch == '\n' || ch == 0 || ch == EOF) { /* Separator */
				nlex_tokrec_finish(nh);
				fprintf(fpout, "Token: %s\n", nh->tokbuf);
				nlex_tokrec_init(nh);
						
			if(ch == 0 || ch == EOF) {
				/* Don't care whether token is set or not */
				break;
			}
		}
	}

	return 0;
}
