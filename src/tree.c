/* tree.c
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020 Nandakumar Edamana
 * File started on 2020-11-28, contains old code.
 */

#include "tree.h"

void nan_tree_build(NanTreeNode * root, NlexHandle * nh)
{
	NanTreeNode * tcurnode = root;
	NanTreeNode * tcurnode_parent;

	NlexCharacter ch;
	_Bool         escaped = 0;
	_Bool         in_list = 0; /* [] */
	_Bool         list_inverted;

	NanCharacterList * chlist;

	nan_tree_init(root);

	while( (ch = nlex_next(nh)) != 0 && ch != EOF) {
		if(ch == '\t' || (ch == ' ' && !in_list)) { /* token-action separator */
			/* Copy everything until line break or EOF into the action buffer */
			
			nlex_shift(nh);
			while((ch = nlex_next(nh)) && (ch != '\n' && ch != EOF));
			
			/* BEGIN Create/attach the action node to the tree */
			NanTreeNode * anode = nlex_malloc(NULL, sizeof(NanTreeNode));
			anode->id = 0;
			anode->ch = NLEX_CASE_ACT;

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

				chlist        = nan_character_list_new();
				list_inverted = 0;
				in_list       = 1;

				continue;
			}
			else if(ch == ']') {
				if(!in_list)
					nlex_die("Closing a list that was never open."); // TODO line and col

				in_list = 0;
				
				if(list_inverted)
					ch = -(NLEX_CASE_LIST | NLEX_CASE_INVERT);
				else
					ch = -NLEX_CASE_LIST;				

				/* Go on; the list will be added to the tree. */
			}
			else if(ch == '^') {
				if(!in_list)
					nlex_die("Inverting a list that was never open."); // TODO line and col
				
				list_inverted = 1;
				
				continue;
			}
			else if(ch == '.') {
				if(in_list)
					nlex_die("dot wildcard is not permitted inside lists."); // TODO line and col

				ch      = -NLEX_CASE_ANYCHAR;
				/* Go on; this will be added to the tree. */
			}
			else if(ch == '*') { /* Kleene star */
				if(tcurnode == root)
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
						tptr->sibling     = tcurnode->sibling;
						tcurnode->sibling = tptr;
						
						tcurnode = tptr;
					}
				}

				/* Skipping the rest because no new node is to be added */
				continue;
			}
			else if(ch == '+') { /* Kleene plus */
				if(tcurnode == root)
					nlex_die("Kleene plus without any preceding character."); // TODO line and col
				
				/* TODO warning on other forbidden cases */
				
				/* Simply clone curnode, make it a Kleene star, and then
				 * add it as the child of curnode.
				 */
				
				NanTreeNode * newnode = nlex_malloc(nh, sizeof(NanTreeNode));
				memcpy(newnode, tcurnode, sizeof(NanTreeNode));
				newnode->first_child  = NULL;

				nan_tree_node_convert_to_kleene(newnode);
				
				/* Prepend */
				newnode->sibling      = tcurnode->first_child;
				tcurnode->first_child = newnode;
				
				tcurnode = newnode;
				
				/* Skip the rest since no new node is to be added. */
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

	if(in_list)
		nlex_die("List opened but not closed."); // TODO line and col
}
