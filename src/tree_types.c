#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tree_types.h"

void nan_character_list_construct(NanCharacterList *this)
{
	this->count = 0;
	this->list = NULL;
}

void nan_character_list_destruct(NanCharacterList *this)
{
}

void nan_tree_node_construct(NanTreeNode *this)
{
	this->visited = false;
	this->sibling = NULL;
	this->first_child = NULL;
	this->klnptr_from = NULL;
	this->klnstate_id_auto = 0;
	this->klnptr = NULL;
	this->fastkw_pattern = NULL;
	this->ch = 0;
	this->id = 0;
}

void nan_tree_node_destruct(NanTreeNode *this)
{
}

void nan_tree_node_vector_construct(NanTreeNodeVector *this)
{
	this->arr = NULL;
	this->count = 0;
	this->alloccount = 0;
}

void nan_tree_node_vector_destruct(NanTreeNodeVector *this)
{
	free(this->arr);
	this->arr = NULL;
}

void nan_tree_node_vector__resize(NanTreeNodeVector *this, size_t newcount)
{
	size_t UNIT = 8;
	size_t newalloccount = this->alloccount;
	assert((newcount - this->count) == 1 || (newcount - this->count) == -1); /* vector.ngg:28 */
	if(this->alloccount != newcount) {
		if(this->alloccount < newcount) {
			newalloccount = this->alloccount + UNIT;
		} else {
			size_t spare = this->alloccount - newcount;
			if(spare > (2 * UNIT)) {
				newalloccount = this->alloccount - UNIT;
			}

		}

		assert(newalloccount >= this->count); /* vector.ngg:46 */
		if(newalloccount != this->alloccount) {
			if(newalloccount == 0) {
				free(this->arr);
				this->arr = NULL;
			} else {
				this->arr = realloc(this->arr, newalloccount * sizeof(this->arr[0]));
				if(this->arr == NULL) { perror(NULL); exit(EXIT_FAILURE); }

			}

			this->alloccount = newalloccount;
		}

	}

	this->count = newcount;
}

void nan_tree_node_vector_append(NanTreeNodeVector *this, NanTreeNode *newitem)
{
	size_t newcount = 1 + this->count;
	nan_tree_node_vector__resize(this, newcount);
	this->arr[newcount - 1] = newitem;
}

void nan_tree_node_vector_clear(NanTreeNodeVector *this)
{
	this->count = 0;
	this->alloccount = 0;
	free(this->arr);
	this->arr = NULL;
}

NanTreeNode* nan_tree_node_vector_get_item(NanTreeNodeVector *this, size_t index)
{
	assert(index < this->count); /* vector.ngg:80 */

	return this->arr[index];
}

NanTreeNode* nan_tree_node_vector_pop(NanTreeNodeVector *this)
{
	assert(this->count > 0); /* vector.ngg:86 */
	NanTreeNode *r = nan_tree_node_vector_get_item(this, this->count - 1);
	this->count = this->count - 1;

	return r;
}

void nan_tree_node_vector_set_item(NanTreeNodeVector *this, size_t index, NanTreeNode *itm)
{
	assert(index < this->count); /* vector.ngg:99 */
	this->arr[index] = itm;
}

_Bool nan_tree_node_vector_is_empty(NanTreeNodeVector *this)
{
	return 0 == this->count;
}

NanTreeNodeVector* nan_tree_node_vector_new()
{

	NanTreeNodeVector * _ngg_tmp_0 = malloc(sizeof(NanTreeNodeVector));
	if(_ngg_tmp_0 == NULL) { perror(NULL); exit(EXIT_FAILURE); }
	nan_tree_node_vector_construct(_ngg_tmp_0);
	return _ngg_tmp_0;
}

NanTreeNodeData nan_tree_node_data_default()
{
	NanTreeNodeData s;
	s.chlist = NULL;
	s.actstr = NULL;

	return s;
}

