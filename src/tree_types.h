#ifndef _N96E_LEX_TREE_TYPES_H
#define _N96E_LEX_TREE_TYPES_H

typedef struct NanCharacterList NanCharacterList;
typedef struct _ngg_vtab_t_NanCharacterList {
} _ngg_vtab_t_NanCharacterList;

typedef struct NanCharacterList {
	_ngg_vtab_t_NanCharacterList _ngg_vtab_nan_character_list;
	int *list;
	size_t count;
} NanCharacterList;

typedef union NanTreeNodeData {
	NanCharacterList *chlist;
	char * actstr;
} NanTreeNodeData;

typedef struct NanTreeNode NanTreeNode;
typedef struct _ngg_vtab_t_NanTreeNode {
} _ngg_vtab_t_NanTreeNode;

typedef struct NanTreeNode {
	_ngg_vtab_t_NanTreeNode _ngg_vtab_nan_tree_node;
	unsigned int id;
	int ch;
	NanTreeNodeData data;
	NanTreeNode *klnptr;
	unsigned int klnstate_id_auto;
	struct NanTreeNodeVector *klnptr_from;
	NanTreeNode *first_child;
	NanTreeNode *sibling;
	_Bool visited;
} NanTreeNode;

typedef struct NanTreeNodeVector NanTreeNodeVector;
typedef struct _ngg_vtab_t_NanTreeNodeVector {
} _ngg_vtab_t_NanTreeNodeVector;

typedef struct NanTreeNodeVector {
	_ngg_vtab_t_NanTreeNodeVector _ngg_vtab_nan_tree_node_vector;
	NanTreeNode* *arr;
	size_t alloccount;
	size_t count;
} NanTreeNodeVector;

static inline size_t nan_tree_node_vector_get_count(NanTreeNodeVector * this) {
return this->count;
}
void nan_character_list_construct(NanCharacterList *this);
void nan_character_list_destruct(NanCharacterList *this);
void nan_tree_node_construct(NanTreeNode *this);
void nan_tree_node_destruct(NanTreeNode *this);
void nan_tree_node_vector_construct(NanTreeNodeVector *this);
void nan_tree_node_vector_destruct(NanTreeNodeVector *this);
void nan_tree_node_vector__resize(NanTreeNodeVector *this, size_t newcount);
void nan_tree_node_vector_append(NanTreeNodeVector *this, NanTreeNode *newitem);
void nan_tree_node_vector_clear(NanTreeNodeVector *this);
NanTreeNode* nan_tree_node_vector_get_item(NanTreeNodeVector *this, size_t index);
NanTreeNode* nan_tree_node_vector_pop(NanTreeNodeVector *this);
void nan_tree_node_vector_set_item(NanTreeNodeVector *this, size_t index, NanTreeNode *itm);
_Bool nan_tree_node_vector_is_empty(NanTreeNodeVector *this);
NanTreeNodeVector* nan_tree_node_vector_new();
NanTreeNodeData nan_tree_node_data_default();

#endif /* _N96E_LEX_TREE_TYPES_H */
