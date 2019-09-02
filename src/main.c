/* Nandakumar Edamana
 * Started on 2019-07-22
 */

#include "read.h"
#include "tree.h"

/* Returns 1 if the elsenode was used in a goto statement */
_Bool nan_tree2code(NanTreeNode *root, NanTreeNode *elsenode, int indent_level)
{
	NanTreeNode * tptr;
	int childcount = 0;
	
	NanTreeNode *elsenode_bak   = elsenode; /* Because can be overridden */
	_Bool        goto_else_used = 0;

	int i;

	// TODO precreate a padding string
	#define indent(); for(i = 0; i < indent_level; i++) fputc(' ', fpout);

	if(root->ch == NLEX_CASE_ELSE) {
		/* Child is an action node */
		indent();
		fprintf(fpout, "%s\n", (char *) root->first_child->ptr);
		return goto_else_used;
	}

	/* TODO do I need to check this? */
	if(root->first_child == NULL) /* Leaf node */
		return goto_else_used;

	/* Override elsenode if any of the children is a genuine else node */
	for(tptr = root->first_child->sibling; tptr; tptr = tptr->sibling)
		if(tptr->ch == NLEX_CASE_ELSE)
			elsenode = tptr;
			/* No need of manual break since there will be no nodes after an else */

	for(tptr = root->first_child; tptr; tptr = tptr->sibling) {
		indent();

		if(childcount++ != 0) /* Not the first child */
			fprintf(fpout, "else ");

		if(tptr->ch >= 0) {   /* Regular character; not a special case. */
			fprintf(fpout, "if(ch == ");
			nan_c_print_character(tptr->ch, fpout);
			fprintf(fpout, ") {\n");

			/* First child itself is else means direct action.
			 * Reading should be done in cases except this.
			 * XXX No need to check if tptr->first_child is NULL because
			 */
			if(tptr->first_child && tptr->first_child->ch != NLEX_CASE_ELSE) {
				// XXX Calling indent() two times won't be useful if indent_level = 0
				indent_level++;
				indent();
				indent_level--;

				fprintf(fpout, "ch = nlex_next(nh);\n");
			}
		}
		else { /* Special cases */
			switch(tptr->ch) {
			case NLEX_CASE_ELSE:
				/* 'else' has already been printed */
				/* TODO Small numbers instead of pointer values */
				
				/* Don't worry, the else node will come only after all the siblings
				 * that can use a goto.
				 */
				if(tptr == elsenode && goto_else_used)
					fprintf(fpout, "{ state_%p:\n", tptr);
				else
					fprintf(fpout, "{\n");
				break;
			case NLEX_CASE_LIST:
				fprintf(fpout, "if ("); /* 'else' has already been printed */
				nan_character_list_to_expr(NAN_CHARACTER_LIST(tptr->ptr), fpout);
				fprintf(fpout, ") {\n");
				break;
			}
		}
		
		goto_else_used = nan_tree2code(tptr, elsenode, indent_level + 1); // TODO

		indent();

		if(elsenode && tptr->sibling == NULL && tptr->ch != NLEX_CASE_ELSE) {
			fprintf(fpout, "} else goto state_%p;\n", elsenode);
			if(elsenode == elsenode_bak)
				goto_else_used = 1;
		}
		else {
			fprintf(fpout, "}\n");
		}
	}
	
	return goto_else_used;
}

int main()
{
	NanTreeNode   troot;
	NanTreeNode * tcurnode = &troot;

	NlexHandle *  nh;
	NlexCharacter ch;
	_Bool         escaped = 0;
	_Bool         in_list = 0; /* [] */

	NanCharacterList * chlist;

	fpout = stdout;

	troot.first_child = NULL;
	troot.sibling     = NULL;
	troot.ch          = NLEX_CASE_ROOT;

	nh = nlex_handle_new();
	if(!nh)
		die("nlex_handle_new() returned NULL.");
	
	nlex_init(nh, stdin, NULL); // TODO FIXME not always stdin

	/* BEGIN Tree Construction */
	while( (ch = nlex_next(nh)) != 0 && ch != EOF) {
		if(ch == '\t' || (ch == ' ' && !in_list)) { /* token-action separator */
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
			anode->ptr = nh->tokbuf;
			/* Nobody cares about first_child or sibling of an action node. */

			/* END Create the action node */

			/* BEGIN Attach the action node to the tree */
			if(tcurnode->ch == NLEX_CASE_ELSE) {
				/* The current node is already an else */
				
				tcurnode->first_child = anode;
			}
			else {
				NanTreeNode *enode = malloc(sizeof(NanTreeNode));
				if(!enode)
					die("malloc() error.");

				enode->ch          = NLEX_CASE_ELSE;
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
			die("No action given for a token."); // TODO line and col

		if(escaped) {
			ch = nlex_get_counterpart(ch, escin, escout);
			if(ch != NAN_NOMATCH)
				escaped = 0;
			else
				die("Unknown escape sequence."); // TODO line and col
		}
		else {
			if(ch == '\\') {
				escaped = 1;
				continue;
			}
			else if(ch == '[') {
				if(in_list)
					die("List inside list."); // TODO line and col

				chlist  = nan_character_list_new();
				in_list = 1;
				continue;
			}
			else if(ch == ']') {
				if(!in_list)
					die("Closing a list that was never open."); // TODO line and col

				in_list = 0;
				ch = NLEX_CASE_LIST;
				/* Go on; the list will be added to the tree. */
			}
			else if(ch == '#') { /* Unescaped '#' means the start of a case spec. */
				if(in_list)
					die("Special cases are not permitted inside lists."); // TODO line and col

				/* Directly assigning to ch will be confusing */
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

				if(specialcase == NLEX_CASE_NONE) {
					die("Unknown or incomplete case specification.");
				}
				else {
					ch = specialcase;
					nlex_back(nh); /* Back to the separator */
				}
			}
		}

		if(in_list) {
			nan_character_list_append(chlist, ch);
			continue;
		}

		NanTreeNode * tmatching_node = NULL;
		
		/* Keeping a pointer one step behind will help me create a new node */
		NanTreeNode * tptr_prv = NULL;

		NanTreeNode * tptr;
		
		/* A temporary node to make the comparison easier */
		NanTreeNode * tmpnode = malloc(sizeof(NanTreeNode));
		if(!tmpnode)
			die("malloc() error.");

		tmpnode->ch  = ch;
		/* No problem if chlist is invalid since
		 * ch will not be NLEX_CASE_LIST in that case.
		 */

		tmpnode->ptr = chlist;

		/* Check if any child of the current node represents the new character */
		for(tptr = tcurnode->first_child; tptr; tptr = tptr->sibling) {
			tptr_prv = tptr;

			if(nan_tree_nodes_match(tptr, tmpnode)) {
				tmatching_node = tptr;
				break;
			}
		}

		free(tmpnode);

		if(tmatching_node) {
			tcurnode = tmatching_node;
		}
		else { /* Create a new node */
			NanTreeNode * newnode = malloc(sizeof(NanTreeNode));
			if(!newnode)
				die("malloc() error.");
			
			newnode->ch          = ch;
			
			/* Again, no problem if chlist is invalid since
			 * ch will not be NLEX_CASE_LIST in that case.
			 */
			newnode->ptr         = chlist;
			
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
			else if(tptr_prv->ch == NLEX_CASE_ELSE) {
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

	if(in_list)
		die("List opened but not closed."); // TODO line and col

	/* BEGIN Code Generation */

	nan_tree2code(&troot, NULL, 0);

	/* END Code Generation */

	return 0;
}
