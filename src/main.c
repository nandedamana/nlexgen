/* Nandakumar Edamana
 * Started on 2019-07-22
 */

#include "read.h"
#include "tree.h"

#define NLEX_CASE_NONE     -1
#define NLEX_CASE_ROOT     -2
#define NLEX_CASE_ELSE     -3
#define NLEX_CASE_SPACE    -4
#define NLEX_CASE_SPACETAB -5

void die(const char *msg) {
	fprintf(stderr, "ERROR: %s\n", msg);
	exit(1);
}

void nan_tree2code(NanTreeNode *root, NanTreeNode *elsenode, int indent_level)
{
	NanTreeNode * tptr;
	int childcount = 0;

	int i;

	// TODO precreate a padding string
	#define indent(); for(i = 0; i < indent_level; i++) fputc(' ', fpout);

	if(root->data.i == NLEX_CASE_ELSE) {
		/* Child is an action node */
		indent();
		fprintf(fpout, "%s\n", (char *) root->first_child->data.ptr);
		return;
	}

	if(root->first_child == NULL) /* Leaf node */
		return;

	/* Override elsenode if any of the children is a genuine else node */
	for(tptr = root->first_child->sibling; tptr; tptr = tptr->sibling)
		if(tptr->data.i == NLEX_CASE_ELSE)
			elsenode = tptr;
			/* No need of manual break since there will be no nodes after an else */

	for(tptr = root->first_child; tptr; tptr = tptr->sibling) {
		indent();

		if(childcount++ != 0) /* Not the first child */
			fprintf(fpout, "else ");

		if(tptr->data.i >= 0) { /* Regular character; not a special case. */
			int escin = nlex_get_escin(tptr->data.i);
			if(escin == -1) /* Not an escape sequence */
				fprintf(fpout, "if(ch == '%c') {\n", tptr->data.i);
			else
				fprintf(fpout, "if(ch == '\\%c') {\n", escin);

			/* First child itself is else means direct action.
			 * Reading should be done in cases except this.
			 */
			if(tptr->first_child->data.i != NLEX_CASE_ELSE) {
				// XXX Calling indent() two times won't be useful if indent_level = 0
				indent_level++;
				indent();
				indent_level--;

				fprintf(fpout, "ch = nlex_next(nh);\n");
			}
		}
		else { /* Special cases */
			switch(tptr->data.i) {
			case NLEX_CASE_ELSE:
				/* 'else' has already been printed */
				/* TODO Small numbers instead of pointer values */
				
				if(tptr == root->first_child) /* Not real else, but action nodes */
					fprintf(fpout, "{\n");
				else
					fprintf(fpout, "{ state_%p:\n", tptr);
				break;
			case NLEX_CASE_SPACETAB:
				fprintf(fpout, "if (ch == ' ' || ch == '\\t') {\n"); /* 'else' has already been printed */
				break;
			}
		}
		
		nan_tree2code(tptr, elsenode, indent_level + 1); // TODO

		indent();

		if(elsenode && tptr->sibling == NULL && tptr->data.i != NLEX_CASE_ELSE)
			fprintf(fpout, "} else goto state_%p;\n", elsenode);
		else
			fprintf(fpout, "}\n");
	}
}

int main()
{
	NanTreeNode   troot;
	NanTreeNode * tcurnode = &troot;

	NlexHandle * nh;
	int          ch;
	_Bool        escaped = 0;

	fpout = stdout;

	troot.first_child = NULL;
	troot.sibling     = NULL;
	troot.data.i      = NLEX_CASE_ROOT;

	nh = nlex_handle_new();
	if(!nh)
		die("nlex_handle_new() returned NULL.");
	
	nlex_init(nh, stdin, NULL); // TODO FIXME not always stdin

	/* BEGIN Tree Construction */
	while( (ch = nlex_next(nh)) != 0 && ch != EOF) {
		if(ch == ' ' || ch == '\t') { /* token-action separator */
			/* Copy everything until line break or EOF into the action buffer */
			
			nlex_tokrec_init(nh); // TODO err
			while(1) {
				ch = nlex_next(nh);
				
				if(ch == '\n' || ch == EOF) {
					nlex_tokrec_finish(nh);
					break;
				}
				
				nlex_tokbuf_append(nh, ch);
			}

			/* Now we have to create an action node. The action node is always
			 * the child of an else node, even if there is no 'if' part.
			 * This is for later convenience (from addition of more cases
			 * to conversion).
			 * So two nodes have to be created: an else node and an action node.
			 * But no need of a new else node if the current node is already an else
			 * node (explicit else specified by the user using #else)
			 */

			/* BEGIN Create the action node */
			NanTreeNode *anode = malloc(sizeof(NanTreeNode));
			if(!anode)
				die("malloc() error.");

			/* Copy the action. */
			anode->data.ptr = nh->tokbuf;
			/* Nobody cares about first_child or sibling of an action node. */

			/* END Create the action node */

			/* BEGIN Attach the action node to the tree */
			if(tcurnode->data.i == NLEX_CASE_ELSE) {
				/* The current node is already an else */
				
				tcurnode->first_child = anode;
			}
			else {
				NanTreeNode *enode = malloc(sizeof(NanTreeNode));
				if(!enode)
					die("malloc() error.");

				enode->data.i      = NLEX_CASE_ELSE;
				enode->first_child = anode;
				enode->sibling     = NULL;

				tcurnode->first_child = enode;
			}
			/* END Attach the action node to the tree */			

			/* Reset the tree pointer */
			tcurnode = &troot;
			
			continue;
		}
		/* TODO can I avoid this [redundant] comparison? */
		/* XXX `*else* if` is unnecessary since `continue` is used above.
		 * But this ensures safety in case I change something.
		 */
		else if(ch == '\n' || ch == 0 || ch == EOF)
			die("No action given for a token.");

		if(escaped) {
			ch = nlex_get_escout(ch);
			if(ch != -1)
				escaped = 0;
			else
				die("Unknown escape sequence.");
		}
		else {
			if(ch == '\\') {
				escaped = 1;
				continue;
			}
			else if(ch == '#') { /* Unescaped '#' means the start of a case spec. */
				int specialcase = NLEX_CASE_NONE;
			
				ch = nlex_next(nh);
				if(ch == 'e') {
					ch = nlex_next(nh);
					if(ch == 'l') {
						ch = nlex_next(nh);
						if(ch == 's') {
							ch = nlex_next(nh);
							if(ch == 'e') {
								ch = nlex_next(nh);
								if(ch == ' ' || ch == '\t') {
									specialcase = NLEX_CASE_ELSE;
								}
							}
						}
					}
				}
				else if(ch == 's') {
					ch = nlex_next(nh);
					if(ch == 'p') {
						ch = nlex_next(nh);
						if(ch == 'a') {
							ch = nlex_next(nh);
							if(ch == 'c') {
								ch = nlex_next(nh);
								if(ch == 'e') {
									ch = nlex_next(nh);
									if(ch == 't') {
										ch = nlex_next(nh);
										if(ch == 'a') {
											ch = nlex_next(nh);
											if(ch == 'b') {
												ch = nlex_next(nh);
												if(ch == ' ' || ch == '\t') {
													specialcase = NLEX_CASE_SPACETAB;
												}
											}
										}
									}
									else if(ch == ' ' || ch == '\t') {
										specialcase = NLEX_CASE_SPACE;
									}
								}
							}
						}
					}
				}

				if(specialcase == NLEX_CASE_NONE) {
					die("Unknown or incomplete case specification.");
				}
				else {
					ch = (specialcase == NLEX_CASE_SPACE)? NLEX_CASE_SPACE: specialcase;
					nlex_back(nh); /* Back to the separator */
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
			NanTreeNode * newnode = malloc(sizeof(NanTreeNode));
			if(!newnode)
				die("malloc() error.");
			
			newnode->data.i      = ch;
			newnode->sibling     = NULL;
			newnode->first_child = NULL;
			
			/* Possible cases:
			 * 1) The current node has no children
			 * 2) The current node has children
			 * 2.1) The last child is an else node
			 * 2.2) The last child is a non-else node
			 */

			/* If the last child is an action node, convert it to an else node.
			 * If the last child is already an else node, the new character node
			 * has to be added before it.
			 */
			
			if(tptr_prv == NULL) {
				/* Case: tcurnode has no children yet */

				tcurnode->first_child = newnode;
			}
			else if(tptr_prv->data.i == NLEX_CASE_ELSE) {
				/* Case: Last child is an else node */
				/* The insertion has to be made before this node */
				
				NanTreeNode ** tptrptr = &(tcurnode->first_child);
				while((*tptrptr)->sibling == tptr_prv)
					tptrptr = &((*tptrptr)->sibling);
				
				/* Now tptrptr is a pointer to the pointer to the else node. */
				newnode->sibling = tptr_prv;
				*tptrptr = newnode;
			}
			else {
				/* Case: Last child is a non-else node */

				tptr_prv->sibling = newnode;
			}
			
			/* For the next character, this node will be the parent */
			tcurnode = newnode;
		}
	}
	/* END Tree Construction */

	/* BEGIN Code Generation */

	nan_tree2code(&troot, NULL, 0);

	/* END Code Generation */

	return 0;
}
