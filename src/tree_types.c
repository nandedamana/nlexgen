#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>

#include "tree_types.h"


void nan_character_list_destruct(NanCharacterList *this)
{
}

void nan_character_list_construct(NanCharacterList *this)
{
	this->count = 0u;
	this->list = NULL;
}

void nan_tree_node_destruct(NanTreeNode *this)
{
}

void nan_tree_node_construct(NanTreeNode *this)
{
	this->visited = false;
	this->sibling = NULL;
	this->first_child = NULL;
	this->klnptr_from = NULL;
	this->klnstate_id_auto = 0u;
	this->klnptr = NULL;
	this->fastkw_pattern = NULL;
	this->ch = 0;
	this->id = 0u;
}

void nan_tree_node_vector__resize(NanTreeNodeVector *this, int newcount)
{
	if(newcount > this->alloccount) {
		int alloccountbak = this->alloccount;

		this->alloccount = 2 * this->alloccount;

		assert(this->alloccount > alloccountbak); /* /home/nandakumar/nandakumar/my-works/software/nguigen/src/ngg-include/vector.ngg:17 */

		assert(newcount <= this->alloccount); /* /home/nandakumar/nandakumar/my-works/software/nguigen/src/ngg-include/vector.ngg:18 */

		this->arr = realloc(this->arr, this->alloccount * sizeof(this->arr[0]));
		if(this->arr == NULL) { perror(NULL); exit(EXIT_FAILURE); }

	}

	this->count = newcount;
}

void nan_tree_node_vector_append(NanTreeNodeVector *this, NanTreeNode *newitem)
{
	int newcount = 1 + this->count;

	nan_tree_node_vector__resize(this, newcount);

	this->arr[newcount - 1] = newitem;
}

void nan_tree_node_vector_clear(NanTreeNodeVector *this)
{
	this->count = 0;
	this->alloccount = 8;
	this->arr = realloc(this->arr, this->alloccount * sizeof(this->arr[0]));
	if(this->arr == NULL) { perror(NULL); exit(EXIT_FAILURE); }

}

NanTreeNode * nan_tree_node_vector_get_item(NanTreeNodeVector *this, int index)
{
	assert(index < this->count); /* /home/nandakumar/nandakumar/my-works/software/nguigen/src/ngg-include/vector.ngg:43 */

	return this->arr[index];
}

NanTreeNode * nan_tree_node_vector_pop(NanTreeNodeVector *this)
{
	assert(this->count > 0); /* /home/nandakumar/nandakumar/my-works/software/nguigen/src/ngg-include/vector.ngg:49 */
	NanTreeNode *r = nan_tree_node_vector_get_item(this, this->count - 1);
	this->count = this->count - 1;

	return r;
}

void nan_tree_node_vector_set_item(NanTreeNodeVector *this, int index, NanTreeNode *itm)
{
	assert(index < this->count); /* /home/nandakumar/nandakumar/my-works/software/nguigen/src/ngg-include/vector.ngg:62 */

	this->arr[index] = itm;
}

int nan_tree_node_vector_get_count(NanTreeNodeVector *this)
{
	return this->count;
}

_Bool nan_tree_node_vector_is_empty(NanTreeNodeVector *this)
{
	return 0 == this->count;
}

void nan_tree_node_vector_destruct(NanTreeNodeVector *this)
{
	if(this->arr) {
		free(this->arr);
	}
}

void nan_tree_node_vector_construct(NanTreeNodeVector *this)
{
	this->alloccount = 8;
	NanTreeNode **_tmp_1 = (NanTreeNode * *) calloc((size_t) 8, sizeof(NanTreeNode *));
	if(_tmp_1 == NULL) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	this->arr = _tmp_1;
	this->count = 0;
}

NanTreeNodeVector * nan_tree_node_vector_new()
{
	NanTreeNodeVector *_tmp_1 = (NanTreeNodeVector *) malloc(sizeof(NanTreeNodeVector));
	if(_tmp_1 == NULL) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	nan_tree_node_vector_construct(_tmp_1);
	return _tmp_1;
}
