/* Nandakumar Edamana
 * Started on 2019-07-22
 */

#include "read.h"
#include "tree.h"

void nan_tree2code(NanTreeNode *root, int indent_level)
{
	NanTreeNode * tptr;
	int childcount = 0;

	int i;

	// TODO precreate a padding string
	#define indent(); for(i = 0; i < indent_level; i++) fputc(' ', fpout);


	for(tptr = root->first_child; tptr; tptr = tptr->sibling) {
		indent();
		
		if(childcount++ != 0)
			fprintf(fpout, "else ");
		
		fprintf(fpout, "if(*bufptr == '%c') {\n", tptr->data);
		indent_level++; // XXX Calling indent() two times won't be useful if indent_level = 0
		indent();
		indent_level--;
		fprintf(fpout, "bufptr++;\n");
		
		nan_tree2code(tptr, indent_level + 1);

		indent();
		fprintf(fpout, "}\n");
	}
}

int main()
{
	NanTreeNode   troot;
	NanTreeNode * tcurnode = &troot;

	fpin  = stdin;
	fpout = stdout;

	bufptr = buf;

	/* Precalculate for efficient later comparisons.
	 * (buf + BUFLEN) actually points to the first byte next to the end of the buffer.
	*/
	bufendptr = buf + BUFLEN;

	troot.first_child = NULL;
	troot.sibling     = NULL;

	char ch;

	/* BEGIN Tree Construction */
	while( (ch = nan_getchar()) != 0 && ch != EOF) {
		if(ch == '\n') { /* End of a token */
			tcurnode = &troot;
			continue;
		}

		NanTreeNode * tmatching_node = NULL;
		
		/* Keeping a pointer one step behind will help me create a new node */
		NanTreeNode * tptr_prv = NULL;

		NanTreeNode * tptr;

		/* Check if any child of the current node represents the new character */
		for(tptr = tcurnode->first_child; tptr; tptr = tptr->sibling) {
			tptr_prv = tptr;

			if(tptr->data == ch) {
				tmatching_node = tptr;
				break;
			}
		}
		
		if(tmatching_node) {
			tcurnode = tmatching_node;
		}
		else { /* Create a new node */
			NanTreeNode * tmp = malloc(sizeof(NanTreeNode));
			if(!tmp) {
				fprintf(stderr, "malloc() error.\n");
				exit(1);
			}
			
			tmp->data        = ch;
			tmp->sibling     = NULL;
			tmp->first_child = NULL;
			
			if(tptr_prv == NULL) {
				/* tcurnode has no children yet */

				tcurnode->first_child = tmp;
			}
			else {
				/* tcurnode has children and tptr_prv represents the last one */

				tptr_prv->sibling = tmp;
			}
			
			/* For the next character, this node will be the parent */
			tcurnode = tmp;
		}
	}
	/* END Tree Construction */

	/* BEGIN Code Generation */

	nan_tree2code(&troot, 0);

	/* END Code Generation */

	return 0;
}
