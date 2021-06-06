#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree_types.h"

void nan_tree_node_vector_construct(NanTreeNodeVector * this )
{
	this->arr = NULL;
	this->count = 0;
	this->allocsiz = 0;
}

void nan_tree_node_vector_destruct(NanTreeNodeVector * this )
{
	free(this->arr);
}

void nan_tree_node_vector__resize(NanTreeNodeVector * this , size_t newcount)
{
	size_t UNIT = 8;
	size_t newallocsiz = this->allocsiz;
	assert((abs((newcount - this->count)) <= 1));
	if((this->allocsiz != newcount)) {
		if((this->allocsiz < newcount)) {
			newallocsiz = (this->allocsiz + UNIT);
		}

		else if((this->allocsiz > (newcount + UNIT))) {
			assert(((this->allocsiz - UNIT) >= 0));
			newallocsiz = (this->allocsiz - UNIT);
		}

		assert((newallocsiz >= this->count));
		if((newallocsiz != this->allocsiz)) {
			if((newallocsiz == 0)) {
				free(this->arr);
				this->arr = NULL;
			}

			else {
				this->arr = realloc(this->arr, (newallocsiz * sizeof(NanTreeNode * )));
				if((this->arr == NULL)) {
					perror(NULL);
					exit(EXIT_FAILURE);
				}

			}

			this->allocsiz = newallocsiz;
		}

	}

	this->count = newcount;
}

void nan_tree_node_vector_append(NanTreeNodeVector * this , NanTreeNode * newitem )
{
	size_t newcount = (1 + this->count);
	nan_tree_node_vector__resize(this, newcount);
	this->arr[(newcount - 1)] = newitem;
}

void nan_tree_node_vector_clear(NanTreeNodeVector * this )
{
	this->count = 0;
	this->allocsiz = 0;
	free(this->arr);
	this->arr = NULL;
}

NanTreeNode *  nan_tree_node_vector_get_item(NanTreeNodeVector * this , size_t index)
{
	assert((index < (this)->count));

	return this->arr[index];
}

void nan_tree_node_vector_set_item(NanTreeNodeVector * this , size_t index, NanTreeNode * itm )
{
	assert((index < (this)->count));
	this->arr[index] = itm;
}

_Bool  nan_tree_node_vector_is_empty(NanTreeNodeVector * this )
{
	return (0 == this->count);
}

NanTreeNodeVector *  nan_tree_node_vector_new()
{
	NanTreeNodeVector * _ngg_tmp_0 ;

	_ngg_tmp_0 = malloc(sizeof(NanTreeNodeVector));
	if(_ngg_tmp_0 == NULL) { perror(NULL); exit(EXIT_FAILURE); }
	nan_tree_node_vector_construct(_ngg_tmp_0);
	return _ngg_tmp_0;
}

