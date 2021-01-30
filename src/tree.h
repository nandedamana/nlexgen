/* tree.h
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020 Nandakumar Edamana
 * Started on 2019-07-22
 */

#ifndef _N96E_LEX_TREE_H
#define _N96E_LEX_TREE_H

#include "error.h"
#include "read.h"
#include "types.h"

#define NLEX_CASE_NONE    -1
#define NLEX_CASE_ROOT    -2
#define NLEX_CASE_ACT     -4

/* Will be negated. */
#define NLEX_CASE_ANYCHAR    8
#define NLEX_CASE_DIGIT     16
#define NLEX_CASE_EOF       32
#define NLEX_CASE_WORDCHAR  64
#define NLEX_CASE_LIST     128
#define NLEX_CASE_INVERT   512

#define NAN_CHARACTER_LIST(x) ((NanCharacterList *) (x))

/* XXX 'Nan' prefix is used to distinguish internal components from
 * the components that can be used by other programs.
 */

typedef struct _NanTreeNode {
	NanTreeNodeId         id;

  NlexCharacter         ch;

	/* If ch is NLEX_CASE_ACT, ptr points to the user-given output code.
	 * If ch is NLEX_CASE_LIST, ptr points to the a NanCharacterList.
	 * Otherwise unused and may hold junk value.
	 */
  void                * ptr;

	/* Points to the first node in the Kleene sub-expression (points to self
	 * if only one node in the Kleene group.)
	 * NULL for non-Kleene nodes.
	 */
	struct _NanTreeNode * klnptr;

	struct _NanTreeNode * first_child;
	struct _NanTreeNode * sibling;
} NanTreeNode;

/* To match multiple characters at a time */
typedef struct _NanCharacterList {
	NlexCharacter * list;
	size_t          count;
} NanCharacterList;

extern NanTreeNodeId treebuild_id_lastact;
extern NanTreeNodeId treebuild_id_lastnonact;

/* Print a character to the C source code output with escaping if needed */
static inline void
	nan_character_print_c_comp(NlexCharacter c, const char * id, FILE * fp)
{
	NlexCharacter escin;

	if(c < 0) {
		if(-c & NLEX_CASE_ANYCHAR)
			fprintf(fp, "(%s != 0 && %s != EOF)", id, id);
		else if(-c & NLEX_CASE_DIGIT)
			fprintf(fp, "isdigit(%s)", id);
		else if(-c & NLEX_CASE_EOF)
			fprintf(fp, "%s == EOF", id);
		else if(-c & NLEX_CASE_WORDCHAR)
			fprintf(fp, "isalpha(%s) || isdigit(%s) || %s == '_'", id, id, id);
	}
	else {
		escin = nlex_get_counterpart(c, escout_c, escin_c);

		if(escin == NAN_NOMATCH)
			fprintf(fp, "%s == '%c'", id, c);
		else
			fprintf(fp, "%s == '\\%c'", id, escin);
	}
}

static inline void
	nan_character_list_append(NanCharacterList * ncl, NlexCharacter c)
{
	ncl->list =
		nlex_realloc(NULL, ncl->list, sizeof(NlexCharacter) * (ncl->count + 1));

	ncl->list[ncl->count++] = c;
}

static inline NanCharacterList * nan_character_list_new()
{
	NanCharacterList * ncl = nlex_malloc(NULL, sizeof(NanCharacterList));
	
	ncl->count = 0;
	ncl->list  = NULL;

	return ncl;
}

static inline NanCharacterList *
	nan_character_list_new_from_character(NlexCharacter c)
{
	NanCharacterList * nclist;

	nclist = nan_character_list_new();

	NAN_CHARACTER_LIST(nclist)->list =
		nlex_malloc(NULL, sizeof(NlexCharacter));

	NAN_CHARACTER_LIST(nclist)->list[0] = c;
	NAN_CHARACTER_LIST(nclist)->count   = 1;
	
	return nclist;
}

/* Character list to Boolean expression */
static inline void
	nan_character_list_to_expr(
		NanCharacterList * ncl, const char * id, FILE * fp)
{
	int i;
	
	if(ncl->count) {
		nan_character_print_c_comp(ncl->list[0], id, fp);
	
		for(i = 1; i < ncl->count; i++) {
			fprintf(fp, " || ");
			nan_character_print_c_comp(ncl->list[i], id, fp);
		}
	}
}

/* Conversion of action nodes */
void nan_tree_astates_to_code(NanTreeNode * root, _Bool if_printed);

const char * nan_tree_build(NanTreeNode * root, NlexHandle * nh);

static inline void nan_tree_node_convert_to_kleene(NanTreeNode * node)
{
	node->klnptr = node;
}

/* Conversion of intermediate nodes */
void nan_tree_istates_to_code(NanTreeNode * root, NanTreeNode * grandparent);

static inline NanTreeNodeId nan_tree_node_id(NanTreeNode * node)
{
	/* Make the action node ids odd and others even.
	 * This is for easy identification at runtime.
	 */
	
	if(node->id == 0) {
		if(node->ch == NLEX_CASE_ACT)
			node->id = ((++treebuild_id_lastact) * 2) + 1;
		else
			node->id = ((++treebuild_id_lastnonact) * 2);
	}

	return node->id;
}

/* TODO FIXME This comparison is order-sensitive for lists. */
/* TODO what if one node is single character and the other is a single-element list? */
// TODO make non-inline
static inline _Bool
	nan_tree_nodes_match(NanTreeNode * node1, NanTreeNode * node2)
{
	_Bool matches;

	NanCharacterList * nclist1;
	NanCharacterList * nclist2;
	
	_Bool free1; /* Free list1 after comparison */
	_Bool free2;

	/* Four possibilities with two nodes since each can be either a
	 * single character node or list node.
	 * Both being non-lists can be handled directly.
	 * The easy way for others is to convert the single character one(s) to
	 * list node(s) before comparison.
	 */

	/* Case: both are single character nodes */
	if( (node1->ch >= 0 || !(-(node1->ch) & NLEX_CASE_LIST)) &&
		(node2->ch >= 0 || !(-(node2->ch) & NLEX_CASE_LIST)) )
	{
		return (node1->ch == node2->ch);
	}

	/* Other cases; convert to lists (when needed) and then compare. */
	if(node1->ch < 0 && -(node1->ch) & NLEX_CASE_LIST) {
		nclist1 = NAN_CHARACTER_LIST(node1->ptr);
		free1 = 0;
	}
	else {
		nclist1 = nan_character_list_new_from_character(node1->ch);
		free1 = 1;
	}
	
	if(node2->ch < 0 && -(node2->ch) & NLEX_CASE_LIST) {
		nclist2 = NAN_CHARACTER_LIST(node2->ptr);
		free2 = 0;
	}
	else {
		nclist2 = nan_character_list_new_from_character(node2->ch);
		free2 = 1;
	}
	
	matches = 1;

	int i = 0;
	
	while(i < nclist1->count &&	i < nclist2->count) {
		if(nclist1->list[i] != nclist2->list[i]) {
			matches = 0;
			break;
		}
		
		i++;
	}
	
	/* Free the list created by this function. */
	if(free1) {
		free(nclist1->list);
		free(nclist1);
	}
	
	if(free2) {
		free(nclist2->list);
		free(nclist2);
	}
	
	/* XXX For lists not created by this function,
	 * I can free one list and reuse the pointer to the other
	 * in case of a match to save memory.
	 * But it has no practical advantage for a lexer.
	 */
	
	return matches;
}

/* TODO only for debug mode; make non inline */
static inline void nan_tree_dump(NanTreeNode * root, signed int level)
{
	NanTreeNode * tptr;

	int i;
	for(i = 0; i < level; i++)
		fprintf(stderr, "-");

	/* Printing root separately can be helpful if root is an inavlid address.
	 * Otherwise segfault will hide the information.
	 */
	fprintf(stderr, "%p ", root);
	fprintf(stderr, "ch=%d (id = %d)\n", root->ch, nan_tree_node_id(root));

	level++;

	for(tptr = root->first_child; tptr; tptr = tptr->sibling)
		nan_tree_dump(tptr, level);
}

static inline void nan_treenode_init(NanTreeNode * root)
{
	root->first_child = NULL;
	root->sibling     = NULL;
	root->ptr         = NULL;
	root->klnptr      = NULL;
	root->id          = 0;
}

static inline NanTreeNode * nan_treenode_new(NlexHandle * nh, NlexCharacter ch)
{
	NanTreeNode * newnode = nlex_malloc(nh, sizeof(NanTreeNode));
	nan_treenode_init(newnode);
	newnode->ch = ch;
	
	return newnode;
}

#endif
