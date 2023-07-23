#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "fastkeywords.h"

_Bool fastkeywords_enabled;
_Bool fastkeywords_use_strcmp;
NanTreeNode *idactnode;
void trie_node_add(TrieNode *this, const char * key, int keyoffset, const char * action)
{
	TrieNode *chld;
	if('\0' == key[keyoffset]) {
		assert(!this->action); /* fastkeywords.ngg:29 */
		this->action = action;

		return ;
	}

	chld = this->first_child;
	while(chld) {
		if(chld->ch == key[keyoffset]) {
			break;
		}

		chld = trie_node_get_next(chld);
	}

	if((!chld) || (chld->ch != key[keyoffset])) {

		TrieNode * _ngg_tmp_1 = malloc(sizeof(TrieNode));
		if(_ngg_tmp_1 == NULL) { perror(NULL); exit(EXIT_FAILURE); }
		trie_node_construct(_ngg_tmp_1);
		chld = _ngg_tmp_1;
		chld->ch = key[keyoffset];
		trie_node_append(this, chld);
	}

	trie_node_add(chld, key, keyoffset + 1, action);
}

void trie_node_append(TrieNode *this, TrieNode *chld)
{
	if(!this->first_child) {
		assert(!this->last_child); /* fastkeywords.ngg:52 */
		this->first_child = chld;
		this->last_child = chld;
	} else {
		assert(this->last_child); /* fastkeywords.ngg:57 */
		this->last_child->sibling = chld;
		this->last_child = chld;
	}

}

TrieNode* trie_node_get_next(TrieNode *this)
{
	return this->sibling;
}

void trie_node_construct(TrieNode *this)
{
	this->ch = 0;
	this->action = NULL;
	this->sibling = NULL;
	this->last_child = NULL;
	this->first_child = NULL;
}

void trie_node_destruct(TrieNode *this)
{
	if(this->first_child) {
		trie_node_destruct(this->first_child);
		free(this->first_child);
	}

	this->first_child = NULL;
	if(this->last_child) {
		trie_node_destruct(this->last_child);
		free(this->last_child);
	}

	this->last_child = NULL;
	if(this->sibling) {
		trie_node_destruct(this->sibling);
		free(this->sibling);
	}

	this->sibling = NULL;
}

void fastkeywords_init(_Bool enabled)
{
	if(!enabled) {
		return ;
	}

	fastkeywords_enabled = true;
}

_Bool is_fastkeyword(const char * pattern)
{
	char c;
	size_t _ngg_tmp_0;
	if('v' == pattern[0]) {
		return false;
	}

	for(_ngg_tmp_0 = 0; _ngg_tmp_0 < strlen(pattern); ++_ngg_tmp_0) {
		c = pattern[_ngg_tmp_0];

		if(!(islower(c))) {
			return false;
		}

	}


	return true;
}

TrieNode* fastkeywords_trie_new()
{

	TrieNode * _ngg_tmp_2 = malloc(sizeof(TrieNode));
	if(_ngg_tmp_2 == NULL) { perror(NULL); exit(EXIT_FAILURE); }
	trie_node_construct(_ngg_tmp_2);
	return _ngg_tmp_2;
}

void fastkeywords_trie_to_code(TrieNode *root, int level, FILE * fp)
{
	TrieNode *chld;
	fprintf(fp, "if(nh->curtoklen > %d) {\n", level);
	chld = root->first_child;
	while(chld) {
		if(chld != root->first_child) {
			fputs("else ", fp);
		}

		fprintf(fp, "if(nh->buf[nh->curtokpos + %d] == %d) {\n", level, chld->ch);
		if(chld->action) {
			fprintf(fp, "if(nh->curtoklen == %d) {\n%s\ngoto after_fastkw;\n}\n", level + 1, chld->action);
		}

		fastkeywords_trie_to_code(chld, level + 1, fp);
		fprintf(fp, "} /* END if(nh->buf[nh->curtokpos + %d] == %d) { */ \n", level, chld->ch);
		chld = trie_node_get_next(chld);
	}

	fprintf(fp, "} /* END if(nh->curtoklen > %d) */\n", level);
}

