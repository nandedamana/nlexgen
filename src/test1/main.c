/* Nandakumar Edamana
 * Started on 2019-07-22
 */

#include "read.h"

int main()
{
	fpin  = stdin;
	fpout = stdout;

	bufptr = buf;

	/* Precalculate for efficient later comparisons.
	 * (buf + BUFLEN) actually points to the first byte next to the end of the buffer.
	*/
	bufendptr = buf + BUFLEN;

	char ch;

	ch = nan_getchar();
	#include "out/lexbranch.c"

	return 0;
}
