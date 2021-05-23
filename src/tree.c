/* tree.c
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020 Nandakumar Edamana
 * File started on 2020-11-28, contains old code.
 */

#include <assert.h>

#include "tree.h"
#include "error.h"

NanTreeNodeId treebuild_id_lastact    = 0;
NanTreeNodeId treebuild_id_lastnonact = 1; /* First one used for the root */

void nan_inode_to_code(NanTreeNode * node)
{
	NanTreeNode * tptr = NULL;
	NanTreeNode * itptr = NULL;
	NanTreeNode * jtptr = NULL;

	/* If Node A has a sibling that represents a list containing
	 * the character dealt by Node A, use of `else` will cause a miss.
	 * Some special siblings might not be actually overlapping,
	 * but checking that is an overkill.
	 */
	bool can_use_else = true;
	for(tptr = node->first_child; tptr; tptr = tptr->sibling) {
		if(tptr->ch < 0 && tptr->ch != NLEX_CASE_ACT) {
			can_use_else = false;
			break;
		}
	}
	
	/* Non-special siblings might share the same character in
	 * some cases (see tests-auto/subx-001.nlx and
	 * tests-auto/kleene-004.nlx).
	 * TODO should that happen? remove the following if that
	 * changes.
	 */
	if(can_use_else) {
		for(itptr = node->first_child; itptr; itptr = itptr->sibling) {
			for(jtptr = node->first_child; jtptr; jtptr = jtptr->sibling) {
				if(itptr == jtptr)
					continue;
				
				if(itptr->ch < 0)
					continue;

				if(itptr->ch == jtptr->ch) {
					can_use_else = false;
					break;
				}
			}
		}
	}

	size_t actcount = 0;
	for(tptr = node->first_child; tptr; tptr = tptr->sibling)
		if(tptr->ch == NLEX_CASE_ACT)
			actcount++;
	
	// TODO why isn't this the case?
	// id so, add a `break` in the following loop.
	// assert(actcount <= 1);

	for(tptr = node->first_child; tptr; tptr = tptr->sibling) {
		if(tptr->ch == NLEX_CASE_ACT) {
			/* TODO needed only when nlex_nstack_is_empty(nh)?
			 * why did I write so in the early days?
			 */
		
			fprintf(fpout,
				"\tif(%u < hiprio_act_this_iter)\n"
				"\t\thiprio_act_this_iter = %u;\n",
				nan_tree_node_id(tptr),
				nan_tree_node_id(tptr));
		}
	}

	_Bool if_printed = 0;

	/* Non-action nodes */
	for(tptr = node->first_child; tptr; tptr = tptr->sibling) {
		if(tptr->ch == NLEX_CASE_ACT)
			continue;

		_Bool printed = 0;

		if(can_use_else) {
			if(if_printed)
				fprintf(fpout, "else ");
			else
				if_printed = 1;
		}

		fprintf(fpout, "if( ");

		if(tptr->ch < 0) { /* Special cases */
			if(-(tptr->ch) & NLEX_CASE_LIST) {
				if(-(tptr->ch) & NLEX_CASE_INVERT)
					fprintf(fpout, "!(");
				else
					fprintf(fpout, "(");
				
				nan_character_list_to_expr(
					nan_treenode_get_charlist(tptr), "ch", fpout);

				fprintf(fpout, ")");
				printed = 1;
			}
		}

		if(!printed) {
			nan_character_print_c_comp(tptr->ch, "ch", fpout);
		}

		fprintf(fpout, " ) {\n");

		/* Usually Kleene star nodes push themselves into the next-stack.
		 * But upon reaching a next-to-wildcard match, I've to remove the
		 * Kleene state from the stack. This is to prevent 'cde' from
		 * being consumed in 'axyzbcde' against the regex 'a*bcde'
		 * TODO something wrong in this comment?
		 */
		if(node->klnptr) {
			fprintf(fpout,
				/* checking again to skip grandparents */
				"\nif(nh->curstate == %u) nlex_nstack_remove(nh, %u);\n",
				nan_tree_node_id(node->klnptr),
				nan_tree_node_id(node->klnptr));
		}
				
		fprintf(fpout, "\tmatch = 1;\n");

		/* Push itself onto the next-stack */
		fprintf(fpout, "\tnlex_nstack_push(nh, %u);\n", nan_tree_node_id(tptr));

		if(tptr->klnptr)
			fprintf(fpout,
				"\tnlex_nstack_push(nh, %u);\n", nan_tree_node_id(tptr->klnptr));

		fprintf(fpout, "}\n");
	}
}

bool nan_tree_astates_to_code(NanTreeNode * root, _Bool if_printed)
{
	NanTreeNode * tptr;

	if(root->visited)
		return if_printed;
	else
		root->visited = true;

	if(root->ch == NLEX_CASE_ACT) {
		if(if_printed)
			fprintf(fpout, "else ");
		else
			if_printed = 1;

		fprintf(fpout, "if(nh->last_accepted_state == %u) {\n"
			"nh->bufptr = nh->buf + nh->lastmatchat; nh->last_accepted_state = 0;\n%s\n}\n",
			nan_tree_node_id(root),
			nan_treenode_get_actstr(root));

		return 1;
	}

	for(tptr = root->first_child; tptr; tptr = tptr->sibling) {
		/* `|=` because the recursive call my return 0 but
		 * I don't want to lose the current value if it is 1.
		 */
		if_printed |= nan_tree_astates_to_code(tptr, if_printed);
	}
	
	return if_printed;
}

const char * nan_tree_build(NanTreeNode * root, NlexHandle * nh)
{
	NanTreeNode * tcurnode = root;
	NanTreeNode * tcurnode_parent = NULL;
	NanTreeNode * klndest = NULL;
	NanTreeNode * lastsubxparent = NULL;
	NanTreeNode * subexptailbak = NULL;
	NanTreeNode * subexptailbak_for_kln = NULL;

	NlexCharacter ch;
	NlexCharacter prvch = 0;
	_Bool         escaped = 0;
	_Bool         in_list = 0; /* [] */
	_Bool         list_inverted = 0;
	_Bool         start_subx = 0;
	_Bool         join_or = 0;

	bool          force_newnode_for_next_char = false;

	NanCharacterList * chlist = NULL;

	nan_treenode_init(root);
	root->ch          = NLEX_CASE_ROOT;
	
	/* ID has to be even because it is a non-action node;
	 * 0 cannot be used because it is a marker (do-not-care cases).
	 */
	root->id          = 2;

	while( (ch = nlex_next(nh)) != 0 && ch != EOF) {
		if(ch == '\t' || (ch == ' ' && !in_list)) { /* token-action separator */
			/* Copy everything until line break or EOF into the action buffer */
			
			nlex_shift(nh);
			while((ch = nlex_next(nh)) && (ch != '\n' && ch != EOF));
			
			/* BEGIN Create/attach the action node to the tree */
			NanTreeNode * anode = nan_treenode_new(nh, NLEX_CASE_ACT);

			/* Copy the action. */
			anode->ptr = (void *) nlex_bufdup(nh, 1);
			/* Nobody cares about first_child or sibling of an action node. */
			
			anode->first_child = NULL;
			
			/* Now prepend */
			anode->sibling        = tcurnode->first_child;
			tcurnode->first_child = anode;
			/* END Attach the action node to the tree */			

			/* Reset the tree pointer */
			tcurnode = root;
			
			goto nextiter;
		}
		/* TODO can I avoid this [redundant] comparison? */
		/* XXX `*else* if` is unnecessary since `continue` is used above.
		 * But this ensures safety in case I change something.
		 */
		else if(ch == '\n' || ch == 0 || ch == EOF)
			return NLEXERR_NO_ACT_GIVEN;

		if(escaped) {
			ch = nlex_get_counterpart(ch, escin, escout);
			if(ch != NAN_NOMATCH)
				escaped = 0;
			else
				return NLEXERR_UNKNOWN_ESCSEQ;
		}
		else {
			if(ch == '\\') {
				escaped = 1;
				goto nextiter;
			}
			else if(ch == '[') {
				if(in_list)
					return NLEXERR_LIST_INSIDE_LIST;

				chlist        = nan_character_list_new();
				list_inverted = 0;
				in_list       = 1;

				goto nextiter;
			}
			else if(ch == '(') {
				start_subx = 1;
				// TODO push to the subx stack
				goto nextiter;
			}
			else if(ch == ')') {
				// TODO check if balanced
				
				klndest = lastsubxparent; // TODO pop from a stack

				join_or = 1;

				goto nextiter;
			}
			else if(ch == '|') {
				// TODO check if allowed

				/* For rejoining later */
				// TODO insert into a queue to allow multiple ORs
				// TODO also, keep a stack of these queues to support nested sub-expressions?
				subexptailbak = tcurnode;
				subexptailbak_for_kln = tcurnode;
				
				/* Select the new head */
				tcurnode = lastsubxparent; // TODO pop from a stack

				/* or a node could have itself as its sibling */
				force_newnode_for_next_char = true;
				
				goto nextiter;
			}
			else if(ch == ']') {
				if(!in_list)
					return NLEXERR_CLOSING_NO_LIST;

				in_list = 0;
				
				if(list_inverted)
					ch = -(NLEX_CASE_LIST | NLEX_CASE_INVERT);
				else
					ch = -NLEX_CASE_LIST;				

				/* Go on; the list will be added to the tree. */
			}
			else if(ch == '^') {
				if(!in_list)
					return NLEXERR_INVERTING_NO_LIST;
				
				list_inverted = 1;
				
				goto nextiter;
			}
			else if(ch == '.') {
				if(in_list)
					return NLEXERR_DOT_INSIDE_LIST;

				ch = -NLEX_CASE_ANYCHAR;
				/* Go on; this will be added to the tree. */
			}
			else if(ch == '*') { /* Kleene star */
				if(tcurnode == root)
					return NLEXERR_KLEENE_STAR_NOTHING;

				/* tcurnode points to the last added node */

				/* TODO should be modified considering the introduction of dot and other features (if any).
				if(tcurnode->ch < 0 && !(-(tcurnode->ch) & NLEX_CASE_LIST)) {
					nlex_die("Kleene star is allowed for single characters and lists only."); // TODO line and col
				}
				*/

				if(prvch == ')' && subexptailbak_for_kln) {
					nan_tree_node_convert_to_kleene(subexptailbak_for_kln, klndest);
					subexptailbak_for_kln = NULL;
				}

				/* Check if curnode has children. If no, 'a' was not used before 'a*';
				 * this means I can directly use it.
				 * Otherwise, I have to create a sibling.
				 */
				
				if(tcurnode->first_child == NULL) {
					/* Simply convert it to a Kleene */
					nan_tree_node_convert_to_kleene(tcurnode, klndest);
				}
				else {
					/* TODO no need to do this for sub-expressions since they'll have exclusive branches anyway? */
					/* Look for a sibling that has the same content but is a Kleene.
					 * If not found,
					 * create a copy, convert it to Kleene and add as sibling.
					 */
					
					NanTreeNode * tptr;
					NanTreeNode * match = NULL;
					
					for(tptr = tcurnode_parent->first_child; tptr; tptr = tptr->sibling) {
						if(tptr == tcurnode)
							goto nextiter;
						
						if( nan_tree_nodes_match(tptr, tcurnode) && tptr->klnptr )
						{
							match = tptr;
							break;
						}
					}
					
					if(match) { /* Kleene sibling found */
						tcurnode = tptr;
						goto nextiter;
					}
					else { /* Kleene sibling NOT found */
						/* Clone and make it Kleene */
						
						tptr = nlex_malloc(NULL, sizeof(NanTreeNode));
						memcpy(tptr, tcurnode, sizeof(NanTreeNode));
						
						nan_tree_node_convert_to_kleene(tptr, klndest);
						
						tptr->id          = 0;
						tptr->first_child = NULL;
						tptr->sibling     = tcurnode->sibling;
						tcurnode->sibling = tptr;
						
						tcurnode = tptr;
					}
				}

				/* Skipping the rest because no new node is to be added */
				goto nextiter;
			}
			else if(ch == '+') { /* Kleene plus */
				if(tcurnode == root)
					return NLEXERR_KLEENE_PLUS_NOTHING;
				
				/* TODO warning on other forbidden cases */
				
				/* Simply clone curnode, make it a Kleene star, and then
				 * add it as the child of curnode.
				 */
				
				NanTreeNode * newnode = nlex_malloc(nh, sizeof(NanTreeNode));
				memcpy(newnode, tcurnode, sizeof(NanTreeNode));
				newnode->first_child  = NULL;

				nan_tree_node_convert_to_kleene(newnode, tcurnode);
				
				/* Prepend */
				newnode->sibling      = tcurnode->first_child;
				tcurnode->first_child = newnode;
				
				tcurnode = newnode;
				
				/* Skip the rest since no new node is to be added. */
				goto nextiter;
			}
		}

		if(in_list) {
			nan_character_list_append(chlist, ch);
			goto nextiter;
		}

		NanTreeNode * tmatching_node = NULL;

		/* Keeping a pointer one step behind will help me create a new node */
		NanTreeNode * tptr_prv = NULL;
	
		/* Sub-expressions need to be separate even if they share the same prefix */
		if(start_subx) {
			tptr_prv = tcurnode->first_child;
			lastsubxparent = tcurnode; // TODO push to a stack to support nested sub-expressions
			start_subx = 0;
		}
		else if(force_newnode_for_next_char) {
			force_newnode_for_next_char = false;

			NanTreeNode * tptr;
			
			/* Code below depends on tptr_prv being the last child */
			for(tptr = tcurnode->first_child; tptr; tptr = tptr->sibling)
				tptr_prv = tptr;
		}
		else {
			NanTreeNode * tptr;
			
			/* A temporary node to make the comparison easier */
			NanTreeNode * tmpnode = nan_treenode_new(nh, ch);

			/* No problem if chlist is invalid since
			 * ch will not be NLEX_CASE_LIST in that case.
			 */

			tmpnode->ptr = chlist;

			/* Check if any child of the current node represents the new character */
			for(tptr = tcurnode->first_child; tptr; tptr = tptr->sibling) {
				tptr_prv = tptr;

				if( nan_tree_nodes_match(tptr, tmpnode) && NULL == tptr->klnptr ) {
					tmatching_node = tptr;
					break;
				}
			}

			free(tmpnode);
		}

		klndest = tcurnode;

		if(tmatching_node) {
			tcurnode_parent = tcurnode;
			tcurnode        = tmatching_node;
		}
		else { /* Create a new node */
			NanTreeNode * newnode = nan_treenode_new(nh, ch);
			
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
		
		if(join_or) {
			if(subexptailbak) { // TODO remove from a queue
				if(subexptailbak->first_child == NULL) {
					subexptailbak->first_child = tcurnode;
				}
				else {
					for(NanTreeNode * nptr = subexptailbak->first_child; nptr; nptr = nptr->sibling) {
						if(nptr->sibling == NULL) {
							nptr->sibling = tcurnode;
							break;
						}
					}
				}

				subexptailbak = NULL;
			}

			join_or = 0;
		}
		
nextiter:
		prvch = ch;
	}

	if(in_list)
		return NLEXERR_LIST_NOT_CLOSED;
	
	return NLEXERR_SUCCESS;
}

/* XXX The resulting code will go through an extra step to reach
 * the action node.
 * This was to make the longest rule preferable (on collision), IIRC.
 * TODO do more research.
 */
void nan_tree_istates_to_code(NanTreeNode * root)
{
	// TODO make the branching better

	if(root->visited)
		return;
	else
		root->visited = true;

	bool if_printed = false;

	if(if_printed)
		fprintf(fpout, "else ");
	else
		if_printed = true;

	fprintf(fpout, "if(nh->curstate == %u) {", nan_tree_node_id(root));

	nan_inode_to_code(root);

	size_t i;
	for(i = 0; i < root->klnptr_from_len; i++)
		if(root->klnptr_from)
			nan_inode_to_code(root->klnptr_from[i]);

	fprintf(fpout, "}\n");

	NanTreeNode * tptr = NULL;
	for(tptr = root->first_child; tptr; tptr = tptr->sibling)
		nan_tree_istates_to_code(tptr);

	return;
}
