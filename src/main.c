/* Nandakumar Edamana
 * Started on 2019-07-22
 */

#define NLEX_ITSELF 1

#include "read.h"
#include "tree.h"

void nan_tree2code(NanTreeNode *root, int indent_level)
{
	NanTreeNode * tptr;
	int childcount = 0;

	int i;

	// TODO precreate a padding string
	#define indent(); for(i = 0; i < indent_level; i++) fputc(' ', fpout);

	/* For non-leaf nodes */
	for(tptr = root->first_child; tptr; tptr = tptr->sibling) {
		indent();

		/* For leaf nodes (their immediate parents, actually) */
		if(tptr->first_child == NULL) {
			fprintf(fpout, "%s\n", (char *) root->first_child->data.ptr);
			continue;
		}
		
		if(childcount++ != 0)
			fprintf(fpout, "else ");
		
		if(tptr->data.i >= 0) {
			int escin = nlex_get_escin(tptr->data.i);
			if(escin == -1) /* Not an escape sequence */
				fprintf(fpout, "if(ch == '%c') {\n", tptr->data.i);
			else
				fprintf(fpout, "if(ch == '\\%c') {\n", escin);
		}
		else { /* Special cases */
			switch(tptr->data.i) {
			case NLEX_CASE_ELSE:
				fprintf(fpout, "{\n"); /* 'else' has already been printed */
				break;
			}
		}
		
		// XXX Calling indent() two times won't be useful if indent_level = 0
		indent_level++;
		indent();
		indent_level--;

		/* Reading will not be done by the leaf nodes. */		
		if(tptr->first_child->first_child != NULL)
			fprintf(fpout, "ch = nlex_getchar();\n");
		
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

	nlex_init();

	troot.first_child = NULL;
	troot.sibling     = NULL;

	int ch;

	_Bool escaped = 0;

	/* BEGIN Tree Construction */
	while( (ch = nlex_getchar()) != 0 && ch != EOF) {
		if(ch == ' ' || ch == '\t') { /* token-action separator */
			/* Copy everything until line break or EOF into the action buffer */
			char   * abuf    = NULL;
			size_t   abuflen = 0;
			size_t   abufpos = 0;
			
			do {
				if(abufpos == abuflen) {
					abuf = realloc(abuf, abuflen + BUFLEN);
					if(!abuf) {
						fprintf(stderr, "realloc() error.\n");
						exit(1);
					}
				
					abuflen += BUFLEN;
				}

				ch = nlex_getchar();
				
				/* XXX A dirty trick to make sure the buffer ends with nullchar;
				 * also helps to break the loop easily.
				 */
				if(ch == '\n' || ch == EOF)
					ch = 0;

				abuf[abufpos++] = ch;
			} while(ch);

			tcurnode->first_child = malloc(sizeof(NanTreeNode));
			if(!tcurnode->first_child) {
				fprintf(stderr, "malloc() error.\n");
				exit(1);
			}
			
			/* Nobody cares about first_child or sibling of the new node. */
			tcurnode->first_child->data.ptr = abuf;
			
			/* Reset the tree pointer */
			tcurnode = &troot;
			
			continue;
		}
		/* TODO can I avoid this [redundant] comparison? */
		/* XXX `*else* if` is unnecessary since `continue` is used above.
		 * But this ensures safety in case I change something.
		 */
		else if(ch == '\n' || ch == 0 || ch == EOF) {
			fprintf(stderr, "ERROR: No action given for a token.\n");
			exit(1);
		}

		if(escaped) {
			ch = nlex_get_escout(ch);
			if(ch != -1) {
				escaped = 0;
			}
			else {
				fprintf(stderr, "ERROR: Unknown escape sequence.\n");
				exit(1);
			}
		}
		else {
			if(ch == '\\') {
				escaped = 1;
				continue;
			}
			else if(ch == '#') { /* Unescaped '#' means the start of a case spec. */
				ch = nlex_getchar();
				if(ch == 'e') {
					ch = nlex_getchar();
					if(ch == 'l') {
						ch = nlex_getchar();
						if(ch == 's') {
							ch = nlex_getchar();
							if(ch == 'e') {
								ch = NLEX_CASE_ELSE;
							}
						}
					}
				}
				
				if(ch >= 0 || /* Still a regular character OR */
					/* the next character is not a separator */
					!( *bufptr == ' ' || *bufptr == '\t' ) )
				{ 
					fprintf(stderr, "ERROR: Unknown or incomplete case specification.\n");
					exit(1);
				}
			}
		}

		NanTreeNode * tmatching_node = NULL;
		
		/* Keeping a pointer one step behind will help me create a new node */
		NanTreeNode * tptr_prv = NULL;

		NanTreeNode * tptr;

		/* Check if any child of the current node represents the new character */
		for(tptr = tcurnode->first_child; tptr; tptr = tptr->sibling) {
			tptr_prv = tptr;

			if(tptr->data.i == ch) {
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
			
			tmp->data.i      = ch;
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
