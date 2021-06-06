/* tree.c
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020, 2021 Nandakumar Edamana
 * File started on 2020-11-28, contains old code.
 */

#include <assert.h>

#include "tree.h"
#include "error.h"

NanTreeNodeId treebuild_id_lastact    = 0;
NanTreeNodeId treebuild_id_lastnonact = 1; /* First one used for the root */

// TODO move
void nan_inode_to_code_kleene_skipping(NanTreeNode * node)
{
	NanTreeNode * tptr = NULL;

	for(tptr = node->first_child; tptr; tptr = tptr->sibling) {
		// TODO FIXME this makes tests-auto/subx-001.nlx pass, but fail some including str-dq.nlx

		if(tptr->ch == NLEX_CASE_PASSTHRU) {
			nan_inode_to_code_kleene_skipping(tptr);
		}

		if(tptr->klnptr_from) {
			size_t len = nan_tree_node_vector_get_count(tptr->klnptr_from);

			for(size_t i = 0; i < len; i++) {
				NanTreeNode * ni = nan_tree_node_vector_get_item(tptr->klnptr_from, i);
				assert(ni);

				nan_inode_to_code(ni, true);
			}
		}
	}
}

void nan_inode_to_code(NanTreeNode * node, bool pseudonode)
{
	NanTreeNode * tptr = NULL;
	NanTreeNode * itptr = NULL;
	NanTreeNode * jtptr = NULL;

	if(pseudonode) {
		if(node->klnptr)
			nan_inode_to_code_matchbranch(node->klnptr);
	}
	else {
		/* Enables entering back the kleene loop */
		if(node->klnptr)
			nan_inode_to_code(node, true);

		/* Children will be handled by the call for the pseudonode */
		if(node->klnptr)
			return;
	}

	nan_inode_to_code_kleene_skipping(node);

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
	
	assert(actcount <= 1);

	for(tptr = node->first_child; tptr; tptr = tptr->sibling) {
		if(tptr->ch == NLEX_CASE_ACT) {
			/* TODO needed only when nlex_nstack_is_empty(nh)?
			 * why did I write so in the early days?
			 */
		
			fprintf(fpout,
				"\tif(%u < hiprio_act_this_iter) {\n"
				"\t\thiprio_act_this_iter = %u;\n",
				nan_tree_node_id(tptr),
				nan_tree_node_id(tptr));

			fprintf(fpout, "ch_read_after_accept = 0;\n");

			#ifdef NLXDEBUG
			fprintf(fpout,
				"\tfprintf(stderr, \"set hiprio_act_this_iter = %u;\\n\");\n",
				nan_tree_node_id(tptr));
			#endif
			
			fprintf(fpout, "\t}\n");
			
			break;
		}
	}

	_Bool if_printed = 0;

	/* Non-action nodes */
	for(tptr = node->first_child; tptr; tptr = tptr->sibling) {
		if(tptr->ch == NLEX_CASE_ACT)
			continue;
			
		if(nan_treenode_is_klndst(tptr))
			continue;

		if(can_use_else) {
			if(if_printed)
				fprintf(fpout, "else ");
			else
				if_printed = 1;
		}

		nan_inode_to_code_matchbranch(tptr);
	}
}

void nan_inode_to_code_matchbranch(NanTreeNode * tptr)
{
	if(tptr->ch == NLEX_CASE_PASSTHRU) {
		NanTreeNode * chld = NULL;
		for(chld = tptr->first_child; chld; chld = chld->sibling)
			nan_inode_to_code_matchbranch(chld);
	
		return;
	}

	_Bool printed = 0;

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

	fprintf(fpout, "\tmatch = 1;\n");

	/* Push itself onto the next-stack */
	fprintf(fpout, "\tnlex_nstack_push(nh, %u);\n", nan_tree_node_id(tptr));

	fprintf(fpout, "}\n");
}

void nan_tree_astates_to_code(NanTreeNode * root)
{
	NanTreeNode * tptr;

	if(root->visited)
		return;
	else
		root->visited = true;

	if(root->ch == NLEX_CASE_ACT) {
		fprintf(fpout, "case %u:\n"
			"\tif(nh->on_consume)\n"
			"\t\tnh->on_consume(nh, nh->curtokpos, nh->curtoklen);\n\n"

			"\t%s\n"
			"\tbreak;\n",
			nan_tree_node_id(root),
			nan_treenode_get_actstr(root));

		return;
	}

	for(tptr = root->first_child; tptr; tptr = tptr->sibling)
		nan_tree_astates_to_code(tptr);
}

const char * nan_tree_build(NanTreeNode * root, NlexHandle * nh)
{
	NanTreeNode * tcurnode = root;
	NanTreeNode * klndest = NULL;
	NanTreeNode * lastsubxparent = NULL;

	NanTreeNode * subexptailbak = NULL;
	NanTreeNode * subexptailbak_for_kln = NULL;

	NlexCharacter ch;
	NlexCharacter prvch = 0;
	_Bool         escaped = 0;
	_Bool         in_list = 0; /* [] */
	_Bool         list_inverted = 0;
	_Bool         join_or = 0;

	NanCharacterList * chlist = NULL;

	nan_treenode_init(root);
	root->ch = NLEX_CASE_ROOT;
	
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
			nan_treenode_set_actstr(anode, nlex_bufdup(nh, 1, 0));
			
			nan_tree_node_append_child(tcurnode, anode);
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
			if(ch != '*' && ch != '+')
				klndest = NULL;
		
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
				NanTreeNode * startnode = nan_treenode_new(nh, NLEX_CASE_PASSTHRU);
				nan_tree_node_append_child(tcurnode, startnode);
				tcurnode = startnode;

				lastsubxparent = startnode; // TODO push to a stack to support nested sub-expressions

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

				assert(tcurnode->first_child == NULL);
				nan_tree_node_convert_to_kleene(tcurnode, klndest);

				/* Skipping the rest because no new node is to be added */
				goto nextiter;
			}
			else if(ch == '+') { /* Kleene plus */
				// TODO FIXME this will node work for subexpressions
			
				if(tcurnode == root)
					return NLEXERR_KLEENE_PLUS_NOTHING;
				
				/* TODO warning on other forbidden cases */
				
				/* Simply clone curnode, make it a Kleene star, and then
				 * add it as the child of curnode.
				 */
				
				NanTreeNode * newnode = nlex_malloc(nh, sizeof(NanTreeNode));
				memcpy(newnode, tcurnode, sizeof(NanTreeNode));
				newnode->first_child  = NULL;

				nan_tree_node_convert_to_kleene(newnode, newnode);
				
				/* Append */
				nan_tree_node_append_child(tcurnode, newnode);
				tcurnode = newnode;
				
				/* Skip the rest since no new node is to be added. */
				goto nextiter;
			}
		}

		if(in_list) {
			nan_character_list_append(chlist, ch);
			goto nextiter;
		}

		NanTreeNode * newnode = nan_treenode_new(nh, ch);
		
		/* Again, no problem if chlist is invalid since
		 * ch will not be NLEX_CASE_LIST in that case.
		 */
		// TODO FIXME why does nan_treenode_set_charlist() fail?
		newnode->data.chlist = chlist;

		nan_tree_node_append_child(tcurnode, newnode);

		/* For the next character, this node will be the parent */
		tcurnode = newnode;

		
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
void nan_tree_istates_to_code(NanTreeNode * root, bool if_printed)
{
	NanTreeNode * tptr = NULL;

	// TODO make the branching better

	if(root->visited)
		return;
	else
		root->visited = true;

	if(if_printed)
		fprintf(fpout, "else ");

	if(root->sibling)
		fprintf(fpout, "if(nh->curstate >= %u && nh->curstate < %u) {", nan_tree_node_id(root), nan_tree_node_id(root->sibling));

	fprintf(fpout, "if(nh->curstate == %u) {", nan_tree_node_id(root));
	nan_inode_to_code(root, false);
	fprintf(fpout, "}\n");

	for(tptr = root->first_child; tptr; tptr = tptr->sibling)
		nan_tree_istates_to_code(tptr, true);

	if(root->sibling)
		fprintf(fpout, "}\n");

	return;
}

void nan_tree_simplify(NanTreeNode * root)
{

	if(root->visited)
		return;
	else
		root->visited = true;

	NanTreeNode * chld = NULL;

	/* Merge adjacent siblings with the same content */
	chld = root->first_child;
	while(chld && chld->sibling) {
		bool merged = false;
	
		if(nan_tree_nodes_match(chld, chld->sibling)) {
			/* Not because extra work is needed while merging;
			 * rather, merging such a node will change the meaning.
			 */
			// TODO check: no actual problem merging nodes with act children?
			bool can_merge =
				chld->sibling &&
				(chld->klnptr == NULL) &&
				(chld->klnptr_from && nan_tree_node_vector_get_count(chld->klnptr_from) <= 0) &&
				(nan_treenode_has_action(chld) == false) &&
				(chld->sibling->klnptr == NULL) &&
				(chld->klnptr_from && nan_tree_node_vector_get_count(chld->sibling->klnptr_from) <= 0) &&
				(nan_treenode_has_action(chld->sibling) == false);
			
			/* Yes, chld->sibling might get checked again in the next iteration;
			 * not worrying about it now.
			 */

			if(can_merge) {
				nan_merge_adjacent_siblings(chld, chld->sibling);
				merged = true;
			}
		}

		/* XXX Move on to the next node only if there was no merge,
		 * or the current and the next nodes won't get merged even
		 * if they match.
		 */
		if(!merged)
			chld = chld->sibling;
	}

	for(chld = root->first_child; chld; chld = chld->sibling)
		nan_tree_simplify(chld);
}
