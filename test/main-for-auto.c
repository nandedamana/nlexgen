/* Nandakumar Edamana
 * 2021
 */

#include <ctype.h>
#include "read.h"

void get_token(NlexHandle *nh);

int ch;

int main()
{
	FILE * fpout = stdout;
	
	NlexHandle * nh;

	nh = nlex_handle_new();
	if(!nh)
		fprintf(stderr, "ERROR: nlex_handle_new() returned NULL.\n");
	
	nlex_init(nh, stdin, NULL);	

	do {
		get_token(nh);

		assert(nh->bufptr >= nh->buf);
		fprintf(stderr, "bufdiff: %zu\n", (nh->bufptr - nh->buf));
		fprintf(stderr, "eof: %d curtoklen: %d\n", nh->eof_read, nh->curtoklen);

	} while(!nh->eof_read && nh->curtoklen > 0);

	return 0;
}
