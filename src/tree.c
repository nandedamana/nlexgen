/* tree.c
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020, 2021, 2023 Nandakumar Edamana
 * File started on 2020-11-28, contains old code.
 */

#include <assert.h>
#include <string.h>

#include "error.h"
#include "fastkeywords.h"
#include "tree.h"
#include "tree_types.h"

NanTreeNodeId treebuild_id_lastact    = 0;
NanTreeNodeId treebuild_id_lastnonact = 1; /* First one used for the root */

/* Assuming the siblings are sorted/grouped; check the code before 2023-04-08
 * to see how it's handled otherwise.
 */
bool can_use_else(NanTreeNode * prvsib)
{
	if(!prvsib)
		return false;

	/* If Node A has a sibling that represents a list containing
	 * the character dealt by Node A, use of `else` will cause a miss.
	 * Some special siblings might not be actually overlapping,
	 * but checking that is an overkill.
	 */
	if(prvsib->sibling->ch < 0 && prvsib->sibling->ch != NLEX_CASE_ACT)
		return false;

	/* Non-special siblings might share the same character in
	 * some cases (see tests-auto/subx-001.nlx and
	 * tests-auto/kleene-004.nlx).
	 * Happens due to how actions and priorities are handled. Remove the following
	 * if that changes.
	 */
	/* TODO in this case, instead of opening a fresh `if`, I should merge the
	 * branches.
	 */
	if(prvsib->ch >= 0 && prvsib->ch == prvsib->sibling->ch)
		return false;

	return true;
}

void nan_inode_to_code(NanTreeNode * node, bool pseudonode)
{
	NanTreeNode * tptr = NULL;

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

	NanTreeNode * sibbak = NULL;

	/* Non-action nodes */
	for(tptr = node->first_child; tptr; sibbak = tptr, tptr = tptr->sibling) {
		if(tptr->ch == NLEX_CASE_ACT || tptr->ch == NLEX_CASE_FASTKWACT)
			continue;
			
		if(nan_treenode_is_klndst(tptr))
			continue;

		if(can_use_else(sibbak)) {
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
				fprintf(fpout, "ch != EOF && ch != '\\0' && !(");
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

	/* Push itself onto the next-stack */
	fprintf(fpout, "\tnlex_nstack_push(nh, %u);\n", nan_tree_node_id(tptr));

	fprintf(fpout, "}\n");
}

void nan_inode_to_code_kleene_skipping(NanTreeNode * node)
{
	NanTreeNode * tptr = NULL;

	for(tptr = node->first_child; tptr; tptr = tptr->sibling) {
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

void nlg_gen_fastkw_onid(NanTreeNode * root)
{
	assert(idactnode);

	fprintf(fpout,
		"%s\ngoto after_fastkw;\n",
		nan_treenode_get_actstr(idactnode));
}

// TODO use hashing or trie-based branching
void nlg_gen_fastkw_selection(NanTreeNode * root)
{
	NanTreeNode * tptr;
	bool first = true;

	for(tptr = root->first_child; tptr; tptr = tptr->sibling) {
		if(tptr->fastkw_pattern) {
			if(!first)
				fprintf(fpout, "else ");
			else
				first = false;

			// XXX Checking the length is a must (see the test tests-auto/fastkw-2-prefix.nlx)
			fprintf(fpout,
				"if(nh->curtoklen == %zu && 0 == strncmp(nh->buf + nh->curtokpos, \"%s\", nh->curtoklen)) {\n%s\ngoto after_fastkw;\n}\n",
				strlen(tptr->fastkw_pattern),
				tptr->fastkw_pattern,
				nan_treenode_get_actstr(tptr));
		}
	}
	
	if(!first)
		fprintf(fpout, "else {\n");

	nlg_gen_fastkw_onid(root);

	if(!first)
		fprintf(fpout, "}\n");
}

const char * nlg_tree_add_rule(
	NanTreeNode * root, NlexHandle * nh_main, const char * pattern, char * action)
{
	if(fastkeywords_enabled && is_fastkeyword(pattern)) {
		NanTreeNode * anode = nan_treenode_new(nh_main, NLEX_CASE_FASTKWACT);
		nan_treenode_set_actstr(anode, action);
		anode->fastkw_pattern = strdup(pattern);
		assert(anode->fastkw_pattern);

		nan_tree_node_append_child(root, anode);

		return NLEXERR_SUCCESS;
	}

	// TODO try if I can avoid using nlex_* for this
	NlexHandle _nh;
	NlexHandle * nh = &_nh; // To make refactoring smooth
	nlex_init(nh, NULL, pattern);

	NanTreeNode * tcurnode = root;
	NanTreeNode * klndest = NULL;
	NanTreeNode * lastsubxparent = NULL;

	NanTreeNodeVector * subxtailbakvec = nan_tree_node_vector_new();

	NanTreeNode * subexptailbak_for_kln = NULL;

	NlexCharacter ch;
	NlexCharacter prvch = 0;
	_Bool         escaped = 0;
	_Bool         in_list = 0; /* [] */
	_Bool         list_inverted = 0;
	_Bool         join_or = 0;

	NanCharacterList * chlist = NULL;

	while( (ch = nlex_next(nh)) != 0 && ch != EOF) {
		/* TODO can I avoid this [redundant] comparison? */
		/* XXX `*else* if` is unnecessary since `continue` is used above.
		 * But this ensures safety in case I change something.
		 */

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
				nan_tree_node_vector_append(subxtailbakvec, tcurnode);
				
				// TODO why am I tracking this separate?
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
			size_t count = nan_tree_node_vector_get_count(subxtailbakvec);
		
			for(size_t i = 0; i < count; i++) {
				NanTreeNode * subexptailbak =
					nan_tree_node_vector_get_item(subxtailbakvec, i);
				assert(subexptailbak);

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
			}

			nan_tree_node_vector_clear(subxtailbakvec);

			join_or = 0;
		}
		
nextiter:
		prvch = ch;
	}

	if(in_list)
		return NLEXERR_LIST_NOT_CLOSED;
	
	/* BEGIN Create/attach the action node to the tree */
	NanTreeNode * anode = nan_treenode_new(nh_main, NLEX_CASE_ACT);

	/* Copy the action. */
	nan_treenode_set_actstr(anode, action);

	nan_tree_node_append_child(tcurnode, anode);
	/* END Attach the action node to the tree */			

	/* Last-added node is assumed to be the ID action */
	if(fastkeywords_enabled)
		idactnode = anode;

	return NLEXERR_SUCCESS;
}

void nlg_tree_init_root(NanTreeNode * root)
{
	nan_treenode_init(root);
	root->ch = NLEX_CASE_ROOT;
	
	/* ID has to be even because it is a non-action node;
	 * 0 cannot be used because it is a marker (do-not-care cases).
	 */
	root->id = 2;
}

/* XXX The resulting code will go through an extra step to reach
 * the action node.
 * This was to make the longest rule preferable (on collision), IIRC.
 * TODO do more research.
 */
/* XXX Although tests pass, doesn't work after the introduction of defrag
 * and soring in nan_tree_simplify(). Switch is faster anyway.
 */
/* Compared to the jump table variant, the output of this fun is structured and (arguably) easier to debug. */
/*
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

	if(root->sibling) {
		// happened once while defrag and merge (tree simplification)
		assert(nan_tree_node_id(root) < nan_tree_node_id(root->sibling));

		// Nested branching for efficiency (like binary search)
		fprintf(fpout, "if(nh->curstate >= %u && nh->curstate < %u) {", nan_tree_node_id(root), nan_tree_node_id(root->sibling));
	}

	fprintf(fpout, "if(nh->curstate == %u) {", nan_tree_node_id(root));
	nan_inode_to_code(root, false);
	fprintf(fpout, "}\n");

	for(tptr = root->first_child; tptr; tptr = tptr->sibling)
		nan_tree_istates_to_code(tptr, true);

	if(root->sibling)
		fprintf(fpout, "}\n");

	return;
}
*/

/* XXX The resulting code will go through an extra step to reach
 * the action node.
 * This was to make the longest rule preferable (on collision), IIRC.
 * TODO do more research.
 */
void nan_tree_istates_to_code_jmp(NanTreeNode * root)
{
	NanTreeNode * tptr = NULL;

	if(root->visited)
		return;
	else
		root->visited = true;

	fputs("goto endjmp;\n", fpout);

	fprintf(fpout, "jmp_%u:\n", nan_tree_node_id(root));
	nan_inode_to_code(root, false);

	for(tptr = root->first_child; tptr; tptr = tptr->sibling)
		nan_tree_istates_to_code_jmp(tptr);

	return;
}

void nan_tree_istates_to_code_mkjmptab_rec(NanTreeNode * root, char ** jmptbl, size_t tablen)
{
	NanTreeNode * tptr = NULL;

	if(root->visited)
		return;
	else
		root->visited = true;

	char * lbl = nlex_malloc(NULL, 32); // TODO FIXME size
	if(snprintf(lbl, 32, "&&jmp_%u", nan_tree_node_id(root)) >= 32)
		nlex_die("insufficient allocation.");

	assert(nan_tree_node_id(root) < tablen);
	jmptbl[nan_tree_node_id(root)] = lbl;

	for(tptr = root->first_child; tptr; tptr = tptr->sibling)
		nan_tree_istates_to_code_mkjmptab_rec(tptr, jmptbl, tablen);

	return;
}

Jmptab nan_tree_istates_to_code_mkjmptab(NanTreeNode * root)
{
	size_t tablen = (treebuild_id_lastnonact * 2) + 1;

	char ** jmptbl = nlex_calloc_internal(tablen, sizeof(char *));

	nan_tree_istates_to_code_mkjmptab_rec(root, jmptbl, tablen);
	
	return (Jmptab){ jmptbl, tablen };
}

/* XXX The resulting code will go through an extra step to reach
 * the action node.
 * This was to make the longest rule preferable (on collision), IIRC.
 * TODO do more research.
 */
void nan_tree_istates_to_code_switch(NanTreeNode * root)
{
	NanTreeNode * tptr = NULL;

	if(root->visited)
		return;
	else
		root->visited = true;

	if(root->ch == NLEX_CASE_ACT || root->ch == NLEX_CASE_FASTKWACT)
		return;

	fprintf(fpout, "case %u: {\n", nan_tree_node_id(root));
	nan_inode_to_code(root, false);
	fputs("break; }\n", fpout);

	for(tptr = root->first_child; tptr; tptr = tptr->sibling)
		nan_tree_istates_to_code_switch(tptr);

	return;
}

void nan_tree_number(NanTreeNode * root)
{
	nan_tree_node_id(root);

	NanTreeNode * chld = NULL;

	for(chld = root->first_child; chld; chld = chld->sibling)
		nan_tree_number(chld);
}

// Move futuresib to the slot before the current sibling (node->sibling)
void nan_tree_move(NanTreeNode * node, NanTreeNode * futuresib, NanTreeNode * futuresib_prvsib)
{
	assert(futuresib_prvsib->sibling == futuresib);
	assert(futuresib != node);
	assert(futuresib != node->sibling);
	assert(futuresib_prvsib != node);

	NanTreeNode * cursib = node->sibling;
	NanTreeNode * futuresib_sibbak = futuresib->sibling;

	node->sibling = futuresib;
	futuresib->sibling = cursib;
	futuresib_prvsib->sibling = futuresib_sibbak;
}

void nan_tree_node_defrag_children(NanTreeNode * root) {
	NanTreeNode * chld_sorted = root->first_child;

	while(chld_sorted && chld_sorted->sibling) {
		if(nan_tree_nodes_match(chld_sorted, chld_sorted->sibling)) {
			assert(chld_sorted != chld_sorted->sibling);
			chld_sorted = chld_sorted->sibling;
			continue;
		}
	
		NanTreeNode * sib2_prv = chld_sorted->sibling;
		NanTreeNode * sib2;
		for(sib2 = sib2_prv->sibling; sib2; sib2_prv = sib2, sib2 = sib2->sibling) {
			if(nan_tree_nodes_match(chld_sorted, sib2)) {
				// Swap chld_sorted->sibling with sib2
				nan_tree_move(chld_sorted, sib2, sib2_prv);
				break;
			}
		}

		assert(chld_sorted != chld_sorted->sibling);
		chld_sorted = chld_sorted->sibling;
	}
}

void nan_tree_simplify(NanTreeNode * root)
{
	if(root->visited)
		return;
	else
		root->visited = true;

	NanTreeNode * chld = NULL;

	/* Won't mess with priorities at this point since numbering has been done.
	 */
	nan_tree_node_defrag_children(root);

	/* Merge adjacent siblings with the same content */
	chld = root->first_child;
	while(chld && chld->sibling) {
		if(chld->ch == NLEX_CASE_FASTKWACT) {
			chld = chld->sibling;
			continue;
		}

		bool merged = false;
	
		bool chld_has_action    = nan_treenode_has_action(chld);
		bool chldsib_has_action = nan_treenode_has_action(chld->sibling);

		if(nan_tree_nodes_match(chld, chld->sibling)) {
			if(chld_has_action && chldsib_has_action)
				nlex_die("Input has duplicate rules."); // TODO useful info

			/* Not because extra work is needed while merging;
			 * rather, merging such a node will change the meaning.
			 */
			bool can_merge =
				(chld->klnptr == NULL) &&
				(!chld->klnptr_from || nan_tree_node_vector_get_count(chld->klnptr_from) <= 0) &&
				!chld_has_action &&
				(chld->sibling->klnptr == NULL) &&
				(!chld->klnptr_from || nan_tree_node_vector_get_count(chld->sibling->klnptr_from) <= 0) &&
				!chldsib_has_action;
			
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

// TODO rem if unused
/*
void nan_tree_node_sort_children(NanTreeNode * root) {
	NanTreeNode * chld_sorted = root->first_child;
	while(chld_sorted) {
		NanTreeNode * futuresib = chld_sorted->sibling;
		NanTreeNode * futuresib_prvsib = NULL;
		NanTreeNode * chld;
		for(chld = futuresib; chld; chld = chld->sibling) {
			if(nan_tree_node_id(chld) < nan_tree_node_id(futuresib))
				futuresib = chld;
			
			futuresib_prvsib = chld;
		}

		if(futuresib != chld_sorted->sibling) {
			// Swap futuresib and the current sibling (chld_sorted->sibling)
		
			NanTreeNode * cursib = chld_sorted->sibling;
			NanTreeNode * cursib_sibling = cursib->sibling;

			chld_sorted->sibling = futuresib;
			cursib->sibling = futuresib->sibling;
			futuresib->sibling = cursib_sibling;
			futuresib_prvsib->sibling = cursib;
		}

		chld_sorted = futuresib;
	}
}
*/
