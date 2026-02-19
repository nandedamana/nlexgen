#ifndef _N96E_LEX_TREE_TYPES_H
#define _N96E_LEX_TREE_TYPES_H

typedef struct NanCharacterList NanCharacterList;
typedef union NanTreeNodeData NanTreeNodeData;
typedef struct NanTreeNode NanTreeNode;
typedef struct NanTreeNodeVector NanTreeNodeVector;
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "types.h"
struct NanCharacterList {
	int *list;
	size_t count;
};

union NanTreeNodeData {
	NanCharacterList *chlist;
	char * actstr;
};

struct NanTreeNode {
	unsigned int id;
	int ch;
	char * fastkw_pattern;
	NanTreeNodeData data;
	NanTreeNode *klnptr;
	unsigned int klnstate_id_auto;
	NanTreeNodeVector *klnptr_from;
	NanTreeNode *first_child;
	NanTreeNode *sibling;
	_Bool visited;
};

struct NanTreeNodeVector {
	NanTreeNode **arr;
	int alloccount;
	int count;
};

void nan_character_list_destruct(NanCharacterList *this);
void nan_character_list_construct(NanCharacterList *this);
void nan_tree_node_destruct(NanTreeNode *this);
void nan_tree_node_construct(NanTreeNode *this);
void nan_tree_node_vector__resize(NanTreeNodeVector *this, int newcount);
void nan_tree_node_vector_append(NanTreeNodeVector *this, NanTreeNode *newitem);
void nan_tree_node_vector_clear(NanTreeNodeVector *this);
NanTreeNode * nan_tree_node_vector_get_item(NanTreeNodeVector *this, int index);
NanTreeNode * nan_tree_node_vector_pop(NanTreeNodeVector *this);
void nan_tree_node_vector_set_item(NanTreeNodeVector *this, int index, NanTreeNode *itm);
int nan_tree_node_vector_get_count(NanTreeNodeVector *this);
_Bool nan_tree_node_vector_is_empty(NanTreeNodeVector *this);
void nan_tree_node_vector_destruct(NanTreeNodeVector *this);
void nan_tree_node_vector_construct(NanTreeNodeVector *this);
NanTreeNodeVector * nan_tree_node_vector_new();

#endif /* _N96E_LEX_TREE_TYPES_H */
