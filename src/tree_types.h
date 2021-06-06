#ifndef _N96E_LEX_TREE_TYPES_H
#define _N96E_LEX_TREE_TYPES_H

typedef struct NanCharacterList {
	int * list ;
	size_t count;
} NanCharacterList;

static inline void NanCharacterList_construct(NanCharacterList * this)
{
}
typedef union NanTreeNodeData {
	NanCharacterList * chlist ;
	char * actstr;
} NanTreeNodeData;

typedef struct NanTreeNode {
	unsigned int id;
	int ch;
	NanTreeNodeData data ;
	struct NanTreeNode * klnptr ;
	unsigned int klnstate_id_auto;
	struct NanTreeNodeVector * klnptr_from ;
	struct NanTreeNode * first_child ;
	struct NanTreeNode * sibling ;
	_Bool visited;
} NanTreeNode;

static inline void NanTreeNode_construct(NanTreeNode * this)
{
}
typedef struct NanTreeNodeVector {
	NanTreeNode * * arr ;
	size_t allocsiz;
	size_t count;
} NanTreeNodeVector;

static inline size_t  nan_tree_node_vector_get_count(NanTreeNodeVector * this) {
return this->count;
}
void nan_tree_node_vector_construct(NanTreeNodeVector * this );
void nan_tree_node_vector_destruct(NanTreeNodeVector * this );
void nan_tree_node_vector__resize(NanTreeNodeVector * this , size_t newcount);
void nan_tree_node_vector_append(NanTreeNodeVector * this , NanTreeNode * newitem );
void nan_tree_node_vector_clear(NanTreeNodeVector * this );
NanTreeNode *  nan_tree_node_vector_get_item(NanTreeNodeVector * this , size_t index);
void nan_tree_node_vector_set_item(NanTreeNodeVector * this , size_t index, NanTreeNode * itm );
_Bool  nan_tree_node_vector_is_empty(NanTreeNodeVector * this );
NanTreeNodeVector *  nan_tree_node_vector_new();

#endif /* _N96E_LEX_TREE_TYPES_H */
