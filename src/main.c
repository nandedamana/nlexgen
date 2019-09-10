/* Nandakumar Edamana
 * Started on 2019-07-22
 */

#include <memory.h>

#include "read.h"
#include "tree.h"

NanTreeNodeId id_lastact    = 0;
NanTreeNodeId id_lastnonact = 0;

void nan_tree2code(NanTreeNode * root, NanTreeNode * grandparent)
{
	NanTreeNode * tptr;

	if(root->ch == NLEX_CASE_ACT) {
		fprintf(fpout, "if(nh->curstate == %d) {\n%s\n}\n",
			nan_tree_node_id(root),
			(char *) root->ptr);
		return;
	}

	for(tptr = root->first_child; tptr; tptr = tptr->sibling) {
		fprintf(fpout, "if((nh->curstate == %d", nan_tree_node_id(root));

		/* Bypass */
		if(root->ch < 0 && -(root->ch) & NLEX_CASE_KLEENE)
			fprintf(fpout, " || nh->curstate == %d", nan_tree_node_id(grandparent));
		
		/* Self loop */
		if(tptr->ch < 0 && -(tptr->ch) & NLEX_CASE_KLEENE)
			fprintf(fpout, " || nh->curstate == %d", nan_tree_node_id(tptr));

		_Bool printed = 0;

		if(tptr->ch < 0) { /* Special cases */
			if(-(tptr->ch) & NLEX_CASE_LIST) {
				fprintf(fpout, ") && (");
				nan_character_list_to_expr(NAN_CHARACTER_LIST(tptr->ptr), "ch", fpout);
				printed = 1;
			}
			else if(tptr->ch == NLEX_CASE_ACT) {
				fprintf(fpout, ") && nlex_nstack_is_empty(nh");
				printed = 1;
			}
		}

		if(!printed) {
			fprintf(fpout, ") && (");
			nan_character_print_c_comp(tptr->ch, "ch", fpout);
		}

		fprintf(fpout, ")) {\n");

		/* Push itself onto the next-stack */
		fprintf(fpout, "\tnlex_nstack_push(nh, %d);\n", nan_tree_node_id(tptr));
		fprintf(fpout, "}\n");
		nan_tree2code(tptr, root);
	}
	
	return;
}

int main()
{
	NanTreeNode   troot;
	NanTreeNode * tcurnode = &troot;
	NanTreeNode * tcurnode_parent;

	NlexHandle *  nh;
	NlexCharacter ch;
	_Bool         escaped = 0;
	_Bool         in_list = 0; /* [] */

	NanCharacterList * chlist;

	fpout = stdout;

	troot.first_child = NULL;
	troot.sibling     = NULL;
	troot.ch          = NLEX_CASE_ROOT;
	troot.id          = 1;

	nh = nlex_handle_new();
	if(!nh)
		nlex_die("nlex_handle_new() returned NULL.");
	
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

			/* BEGIN Create/attach the action node to the tree */
			if(tcurnode->ch == NLEX_CASE_ACT) {
				/* The current node is already an else (explicit #else) */
				
				/* Copy the action. */
				tcurnode->ptr = (void *) nh->tokbuf;
			}
			else {
				NanTreeNode *anode = nlex_malloc(NULL, sizeof(NanTreeNode));
				anode->id = 0;
				anode->ch = NLEX_CASE_ACT;
//				anode->sibling     = NULL; TODO REM if not needed

				/* Copy the action. */
				anode->ptr = (void *) nh->tokbuf;
				/* Nobody cares about first_child or sibling of an action node. */
				
				/* Now attach */
				tcurnode->first_child = anode;
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
			nlex_die("No action given for a token."); // TODO line and col

		if(escaped) {
			ch = nlex_get_counterpart(ch, escin, escout);
			if(ch != NAN_NOMATCH)
				escaped = 0;
			else
				nlex_die("Unknown escape sequence."); // TODO line and col
		}
		else {
			if(ch == '\\') {
				escaped = 1;
				continue;
			}
			else if(ch == '[') {
				if(in_list)
					nlex_die("List inside list."); // TODO line and col

				chlist  = nan_character_list_new();
				in_list = 1;
				continue;
			}
			else if(ch == ']') {
				if(!in_list)
					nlex_die("Closing a list that was never open."); // TODO line and col

				in_list = 0;
				ch      = -NLEX_CASE_LIST;
				/* Go on; the list will be added to the tree. */
			}
			else if(ch == '.') {
				if(in_list)
					nlex_die("dot wildcard is not permitted inside lists."); // TODO line and col

				ch      = -NLEX_CASE_ANYCHAR;
				/* Go on; this will be added to the tree. */
			}
			else if(ch == '*') { /* Kleene star */
				if(tcurnode == &troot)
					nlex_die("Kleene star without any preceding character."); // TODO line and col

				/* tcurnode points to the last added node */

				/* TODO should be modified considering the introduction of dot and other features (if any).
				if(tcurnode->ch < 0 && !(-(tcurnode->ch) & NLEX_CASE_LIST)) {
					nlex_die("Kleene star is allowed for single characters and lists only."); // TODO line and col
				}
				*/

				/* Check if curnode has children. If no, 'a' was not used before 'a*';
				 * this means I can directly use it.
				 * Otherwise, I have to create a sibling.
				 */
				
				if(tcurnode->first_child == NULL) {
					/* Simply convert it to a Kleene */
					nan_tree_node_convert_to_kleene(tcurnode);
				}
				else {
					/* Look for a sibling that has the same content but is a Kleene.
					 * If not found,
					 * create a copy, convert it to Kleene and add as sibling.
					 */
					
					NanTreeNode * tptr;
					NanTreeNode * match = NULL;
					
					for(tptr = tcurnode_parent->first_child; tptr; tptr = tptr->sibling) {
						if(tptr == tcurnode)
							continue;
						
						if(nan_tree_nodes_match(tptr, tcurnode) &&
							-(tptr->ch) & NLEX_CASE_KLEENE)
						{
							match = tptr;
							break;
						}
					}
					
					if(match) { /* Kleene sibling found */
						tcurnode = tptr;
						continue;
					}
					else { /* Kleene sibling NOT found */
						/* Clone and make it Kleene */
						
						tptr = nlex_malloc(NULL, sizeof(NanTreeNode));
						memcpy(tptr, tcurnode, sizeof(NanTreeNode));
						
						nan_tree_node_convert_to_kleene(tptr);
						
						tptr->id          = 0;
						tptr->first_child = NULL;
						tptr->sibling = tcurnode->sibling;
						tcurnode->sibling = tptr;
						
						tcurnode = tptr;
					}
				}

				/* Skipping the rest because no new node is to be added */
				continue;
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
		NanTreeNode * tmpnode = nlex_malloc(NULL, sizeof(NanTreeNode));

		tmpnode->ch  = ch;
		/* No problem if chlist is invalid since
		 * ch will not be NLEX_CASE_LIST in that case.
		 */

		tmpnode->ptr = chlist;

		/* Check if any child of the current node represents the new character */
		for(tptr = tcurnode->first_child; tptr; tptr = tptr->sibling) {
			tptr_prv = tptr;

			if( nan_tree_nodes_match(tptr, tmpnode) &&
				(tptr->ch >= 0 || !(-(tptr->ch) & NLEX_CASE_KLEENE)) ) /* Not a Kleene */
			{
				tmatching_node = tptr;
				break;
			}
		}

		free(tmpnode);

		if(tmatching_node) {
			tcurnode_parent = tcurnode;
			tcurnode        = tmatching_node;
		}
		else { /* Create a new node */
			NanTreeNode * newnode = nlex_malloc(NULL, sizeof(NanTreeNode));
			newnode->id           = 0;
			newnode->ch           = ch;
			
			/* Again, no problem if chlist is invalid since
			 * ch will not be NLEX_CASE_LIST in that case.
			 */
			newnode->ptr         = chlist;
			
			newnode->sibling     = NULL;
			newnode->first_child = NULL;
			
			if(tptr_prv == NULL) /* Case: tcurnode has no children yet */
				tcurnode->first_child = newnode;
			else
				tptr_prv->sibling = newnode;
			
			/* For the next character, this node will be the parent */
			tcurnode = newnode;
		}
	}
	/* END Tree Construction */

	if(in_list)
		nlex_die("List opened but not closed."); // TODO line and col

	/* BEGIN Code Generation */
	
	// TODO FIXME
	#define DEBUG 1
	
	if(DEBUG) {
		nan_tree_dump(&troot, 0);
		fprintf(stderr, "tree dump complete.\n");
	}

	fprintf(fpout, "nlex_nstack_push(nh, %d);\n", troot.id);
	fprintf(fpout,
		"while(!nlex_nstack_is_empty(nh)) {\n"
		"nlex_nstack_fix_actions(nh);\n"
#ifdef DEBUG
		"fprintf(stderr, "
		"\"stack after the iteration that read %%d ('%%c'):\\n\", nlex_last(nh), nlex_last(nh));\n"
		"nlex_nstack_dump(nh);\n"
#endif
		"nlex_swap_t_n_stacks(nh);\n"
		"ch = nlex_next(nh);\n"
		"while(!nlex_tstack_is_empty(nh) && "
		"(nh->curstate = nlex_tstack_pop(nh))) {\n");
	nan_tree2code(&troot, NULL);
	fprintf(fpout, "\n}\n}\n");

	/* END Code Generation */

	return 0;
}
