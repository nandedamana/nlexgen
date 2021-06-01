/* tree.h
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020 Nandakumar Edamana
 * Started on 2019-07-22
 */

#ifndef _N96E_LEX_TREE_H
#define _N96E_LEX_TREE_H

#include <assert.h>
#include <stdbool.h>
#include "error.h"
#include "read.h"
#include "types.h"

#define NLEX_CASE_NONE    -1
#define NLEX_CASE_ROOT    -2
#define NLEX_CASE_ACT     -4
#define NLEX_CASE_PASSTHRU -8

/* Will be negated. */
#define NLEX_CASE_ANYCHAR    1024
#define NLEX_CASE_DIGIT     16
#define NLEX_CASE_LETTER    2048
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
  const void          * ptr;

	/* Points to the first node in the Kleene sub-expression (points to self
	 * if only one node in the Kleene group.)
	 * NULL for non-Kleene nodes.
	 */
	struct _NanTreeNode * klnptr;
	NanTreeNodeId         klnstate_id_auto;

	struct _NanTreeNode ** klnptr_from;
	size_t                 klnptr_from_len;

	struct _NanTreeNode * first_child;
	struct _NanTreeNode * sibling;
	
	/* Because this is a graph and a node can have multiple parents */
	bool visited;
} NanTreeNode;

/* To match multiple characters at a time */
typedef struct _NanCharacterList {
	NlexCharacter * list;
	size_t          count;
} NanCharacterList;

extern NanTreeNodeId treebuild_id_lastact;
extern NanTreeNodeId treebuild_id_lastnonact;

/* @param pseudonode True if called for node->klnstate_id_auto */
void nan_inode_to_code(NanTreeNode * node, bool pseudonode);

void nan_inode_to_code_matchbranch(NanTreeNode * tptr);

static inline void nan_tree_unvisit(NanTreeNode * root)
{
	if(root->visited == false)
		return;

	root->visited = false;

	for(NanTreeNode * tptr = root->first_child; tptr; tptr = tptr->sibling)
		nan_tree_unvisit(tptr);
}

static inline void nan_treenode_init(NanTreeNode * root)
{
	root->first_child = NULL;
	root->sibling     = NULL;
	root->ptr         = NULL;
	root->klnptr      = NULL;
	root->klnstate_id_auto = 0;
	root->klnptr_from = NULL;
	root->klnptr_from_len = 0;
	root->id          = 0;
	root->visited     = false;
}

// TODO make non-inline
static inline bool
	nan_treenode_is_klndst(NanTreeNode * tptr)
{
	if(tptr->ch != NLEX_CASE_PASSTHRU) {
		return tptr->klnptr_from_len > 0;
	}
	else {
		NanTreeNode * chld = NULL;
		for(chld = tptr->first_child; chld; chld = chld->sibling)
			if(nan_treenode_is_klndst(chld) > 0)
				return true;
	}

	return false;
}

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

static inline void
	nan_treenode_set_klnptr(NanTreeNode * node, NanTreeNode * klnptr)
{
	if(!klnptr)
		klnptr = node;

	node->klnptr = klnptr;
	node->klnstate_id_auto = ((++treebuild_id_lastnonact) * 2);

	if(klnptr) {
		klnptr->klnptr_from_len++;
		// TODO Use nlex_realloc()
		klnptr->klnptr_from = realloc(klnptr->klnptr_from, sizeof(klnptr->klnptr_from[0]) * klnptr->klnptr_from_len);
		assert(klnptr->klnptr_from);

		klnptr->klnptr_from[klnptr->klnptr_from_len - 1] = node;
	}
}

static inline NanTreeNode * nan_treenode_new(NlexHandle * nh, NlexCharacter ch)
{
	NanTreeNode * newnode = nlex_malloc(nh, sizeof(NanTreeNode));
	nan_treenode_init(newnode);
	newnode->ch = ch;
	
	return newnode;
}

static inline const char * nan_treenode_get_actstr(const NanTreeNode * node)
{
	assert( node->ch == NLEX_CASE_ACT );
	return (const char *) node->ptr;
}

static inline NanCharacterList *
	nan_treenode_get_charlist(const NanTreeNode * node)
{
	assert( (node->ch < 0) && (-(node->ch) & NLEX_CASE_LIST) );
	return NAN_CHARACTER_LIST(node->ptr);
}

static inline void nan_treenode_set_actstr(NanTreeNode * node, const char * s)
{
	assert( node->ch == NLEX_CASE_ACT );
	node->ptr = s;
}

static inline void
	nan_treenode_set_charlist(NanTreeNode * node, NanCharacterList * cl)
{
	assert( (node->ch < 0) && (-(node->ch) & NLEX_CASE_LIST) );
	node->ptr = cl;
}

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
		else if(-c & NLEX_CASE_LETTER)
			fprintf(fp, "isalpha(%s)", id);
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

	nclist->list = nlex_malloc(NULL, sizeof(NlexCharacter));

	nclist->list[0] = c;
	nclist->count   = 1;
	
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
bool nan_tree_astates_to_code(NanTreeNode * root, _Bool if_printed);

const char * nan_tree_build(NanTreeNode * root, NlexHandle * nh);

static inline void
	nan_tree_node_convert_to_kleene(NanTreeNode * node, NanTreeNode * klnptr)
{
	assert(node);
	nan_treenode_set_klnptr(node, klnptr);
}

/* Conversion of intermediate nodes */
void nan_tree_istates_to_code(NanTreeNode * root, bool if_printed);

/* TODO FIXME This comparison is order-sensitive for lists. */
/* TODO what if one node is single character and the other is a single-element list? */
// TODO make non-inline
static inline bool
	nan_tree_nodes_match(NanTreeNode * node1, NanTreeNode * node2)
{

	NanCharacterList * nclist1 = NULL;
	NanCharacterList * nclist2 = NULL;
	
	_Bool free1 = 0; /* Free list1 after comparison */
	_Bool free2 = 0;

	/* Four possibilities with two nodes since each can be either a
	 * single character node or list node.
	 * Both being non-lists can be handled directly.
	 * The easy way for others is to convert the single character one(s) to
	 * list node(s) before comparison.
	 */

	if(node1->ch == NLEX_CASE_PASSTHRU || node2->ch == NLEX_CASE_PASSTHRU)
		return false;

	/* Case: both are single character nodes */
	if( (node1->ch >= 0 || !(-(node1->ch) & NLEX_CASE_LIST)) &&
		(node2->ch >= 0 || !(-(node2->ch) & NLEX_CASE_LIST)) )
	{
		return (node1->ch == node2->ch);
	}

	/* Other cases; convert to lists (when needed) and then compare. */
	if(node1->ch < 0 && -(node1->ch) & NLEX_CASE_LIST) {
		nclist1 = nan_treenode_get_charlist(node1);
		free1 = 0;
	}
	else {
		nclist1 = nan_character_list_new_from_character(node1->ch);
		free1 = 1;
	}
	
	if(node2->ch < 0 && -(node2->ch) & NLEX_CASE_LIST) {
		nclist2 = nan_treenode_get_charlist(node2);
		free2 = 0;
	}
	else {
		nclist2 = nan_character_list_new_from_character(node2->ch);
		free2 = 1;
	}
	
	bool matches = true;

	int i = 0;
	
	if(nclist1->count !=	nclist2->count) {
		matches = false;
	}
	else {
		while(i < nclist1->count) {
			if(nclist1->list[i] != nclist2->list[i]) {
				matches = false;
				break;
			}
			
			i++;
		}
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

static inline void nan_tree_assign_node_ids_rec(NanTreeNode * node)
{
	if(node->visited) {
		assert(0 != nan_tree_node_id(node));
		return;
	}
	else {
		node->visited = true;
	}

	assert(0 != nan_tree_node_id(node));

	for(NanTreeNode * chld = node->first_child; chld; chld = chld->sibling) {
		assert(chld != chld->sibling);
		nan_tree_assign_node_ids_rec(chld);
	}
}

static inline void nan_assert_all_nodes_have_id(NanTreeNode * node)
{
	if(node->visited)
		return;
	else
		node->visited = true;

	assert(node->id != 0);

	for(NanTreeNode * chld = node->first_child; chld; chld = chld->sibling) {
		assert(chld != chld->sibling);
		nan_assert_all_nodes_have_id(chld);
	}
}

static inline void nan_tree_assign_node_ids(NanTreeNode * root)
{
	nan_tree_unvisit(root);
	nan_tree_assign_node_ids_rec(root);
}

static inline void nan_tree_node_append_child(NanTreeNode * node, NanTreeNode * chld)
{
	if(!node->first_child) {
		node->first_child = chld;
	}
	else {
		NanTreeNode * tptr = node->first_child;
		while(tptr) {
			if(! tptr->sibling) {
				tptr->sibling = chld;
				break;
			}
		
			tptr = tptr->sibling;
		}
	}
}

static inline void nan_merge_treenodes(NanTreeNode * node1, NanTreeNode * node2)
{
	if(node2->first_child)
		nan_tree_node_append_child(node1, node2->first_child);

	free(node2);
}

static inline void nan_merge_adjacent_siblings(NanTreeNode * node1, NanTreeNode * node2)
{
	assert(node1->sibling == node2);

	node1->sibling = node2->sibling;
	nan_merge_treenodes(node1, node2);
}

void nan_tree_simplify(NanTreeNode * root);

static inline bool nan_treenode_has_action(NanTreeNode * root)
{
	NanTreeNode * chld = NULL;
	
	for(chld = root->first_child; chld; chld = chld->sibling)
		if(chld->ch == NLEX_CASE_ACT)
			return true;

	return false;
}

#endif
