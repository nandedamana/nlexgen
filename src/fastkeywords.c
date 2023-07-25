#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "fastkeywords.h"

_Bool fastkeywords_enabled;
_Bool fastkeywords_use_strcmp;
_Bool fastkeywords_use_length_based_trie = true;
_Bool fastkeywords_fuse_single_child = true;
_Bool fastkeywords_fuse_as_int;
_Bool big_endian;
NanTreeNode *idactnode;
void trie_node_add(TrieNode *this, const char * key, int keyoffset, const char * action, _Bool lengthwise)
{
	TrieNode *chld;
	if(lengthwise) {
		TrieNode *chld;
		size_t klen = strlen(key);
		assert(klen > 0); /* fastkeywords.ngg:47 */
		chld = this->first_child;
		while(chld) {
			if(chld->keylen == klen) {
				break;
			}

			chld = trie_node_get_next(chld);
		}

		if((!chld) || (chld->keylen != klen)) {

			TrieNode * _ngg_tmp_1 = malloc(sizeof(TrieNode));
			if(_ngg_tmp_1 == NULL) { perror(NULL); exit(EXIT_FAILURE); }
			trie_node_construct(_ngg_tmp_1);
			chld = _ngg_tmp_1;
			chld->keylen = klen;
			trie_node_append(this, chld);
		}

		trie_node_add(chld, key, keyoffset, action, false);

		return ;
	}

	if('\0' == key[keyoffset]) {
		assert(!this->action); /* fastkeywords.ngg:68 */
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

		TrieNode * _ngg_tmp_2 = malloc(sizeof(TrieNode));
		if(_ngg_tmp_2 == NULL) { perror(NULL); exit(EXIT_FAILURE); }
		trie_node_construct(_ngg_tmp_2);
		chld = _ngg_tmp_2;
		chld->ch = key[keyoffset];
		trie_node_append(this, chld);
	}

	trie_node_add(chld, key, keyoffset + 1, action, false);
}

void trie_node_append(TrieNode *this, TrieNode *chld)
{
	if(!this->first_child) {
		assert(!this->last_child); /* fastkeywords.ngg:91 */
		this->first_child = chld;
		this->last_child = chld;
	} else {
		assert(this->last_child); /* fastkeywords.ngg:96 */
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
	this->cond_printed = false;
	this->keylen = 0;
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

	TrieNode * _ngg_tmp_3 = malloc(sizeof(TrieNode));
	if(_ngg_tmp_3 == NULL) { perror(NULL); exit(EXIT_FAILURE); }
	trie_node_construct(_ngg_tmp_3);
	return _ngg_tmp_3;
}

void fastkeywords_trie_to_code(TrieNode *root, int level, FILE * fp)
{
	if(fastkeywords_use_length_based_trie) {
		fastkeywords_trie_to_code_lengthwise(root, level, fp);
	} else {
		fastkeywords_trie_to_code_not_lengthwise(root, level, fp);
	}

}

void fastkeywords_trie_to_code_not_lengthwise(TrieNode *root, int level, FILE * fp)
{
	TrieNode *chld;
	assert(root->keylen == 0); /* fastkeywords.ngg:141 */
	fprintf(fp, "if(nh->curtoklen > %d) {\n", level);
	chld = root->first_child;
	while(chld) {
		if(chld != root->first_child) {
			fputs("else ", fp);
		}

		fprintf(fp, "if(nh->buf[nh->curtokpos + %d] == \'%c\') {\n", level, chld->ch);
		if(chld->action) {
			fprintf(fp, "if(nh->curtoklen == %d) {\n%s\ngoto after_fastkw;\n}\n", level + 1, chld->action);
		}

		fastkeywords_trie_to_code(chld, level + 1, fp);
		fprintf(fp, "} /* END if(nh->buf[nh->curtokpos + %d] == \'%c\') { */\n", level, chld->ch);
		chld = trie_node_get_next(chld);
	}

	fprintf(fp, "} /* END if(nh->curtoklen > %d) */\n", level);
}

void fastkeywords_trie_to_code_lengthwise(TrieNode *root, int level, FILE * fp)
{
	TrieNode *chld;
	fputs("switch(nh->curtoklen) {\n", fp);
	chld = root->first_child;
	while(chld) {
		assert(chld->keylen > 0); /* fastkeywords.ngg:171 */
		fprintf(fp, "case %zu:\n", chld->keylen);
		fastkeywords_trie_to_code_lengthwise_nonroot(chld, level, fp);
		fputs("break;\n", fp);
		chld = trie_node_get_next(chld);
	}

	fputs("} /* switch(nh->curtoklen) */\n", fp);
}

void fastkeywords_trie_to_code_lengthwise_nonroot(TrieNode *root, int level, FILE * fp)
{
	if(fastkeywords_fuse_single_child) {
		fastkeywords_trie_to_code_lengthwise_nonroot_fuse_single_child(root, level, fp);
	} else {
		fastkeywords_trie_to_code_lengthwise_nonroot_nofuse_single_child(root, level, fp);
	}

}

void fastkeywords_trie_to_code_lengthwise_nonroot_nofuse_single_child(TrieNode *root, int level, FILE * fp)
{
	TrieNode *chld;
	chld = root->first_child;
	while(chld) {
		if(chld != root->first_child) {
			fputs("else ", fp);
		}

		fprintf(fp, "if(nh->buf[nh->curtokpos + %d] == \'%c\') {\n", level, chld->ch);
		if(chld->action) {
			fprintf(fp, "\n%s\ngoto after_fastkw;\n", chld->action);
		}

		fastkeywords_trie_to_code_lengthwise_nonroot_nofuse_single_child(chld, level + 1, fp);
		fprintf(fp, "} /* END if(nh->buf[nh->curtokpos + %d] == \'%c\') */\n", level, chld->ch);
		chld = trie_node_get_next(chld);
	}

}

void fastkeywords_trie_to_code_lengthwise_nonroot_fuse_single_child(TrieNode *root, int level, FILE * fp)
{
	TrieNode *chld;
	chld = root->first_child;
	while(chld) {
		if(chld != root->first_child) {
			fputs("else ", fp);
		}

		if(!chld->cond_printed) {
			int nschld = 0;
			if(fastkeywords_fuse_as_int) {
				nschld = count_single_children(chld);
			}

			_Bool use_hex = true;
			switch(nschld) {
			case 1:
			{
				fprintf(fp, "if(*((short *) (nh->buf + nh->curtokpos + %d)) == ", level);
				break;
			}
			case 3:
			{
				fprintf(fp, "if(*((int *) (nh->buf + nh->curtokpos + %d)) == ", level);
				break;
			}
			case 7:
			{
				fprintf(fp, "if(*((long *) (nh->buf + nh->curtokpos + %d)) == ", level);
				break;
			}
			default:
			{
				fprintf(fp, "if(nh->buf[nh->curtokpos + %d] == \'%c\'", level, chld->ch);
				use_hex = false;
				break;
			}
			}

			if(use_hex) {
				if(big_endian) {
					fprintf(fp, "0x%2x", chld->ch);
				} else {
					fprintf(fp, "0x", chld->ch);
				}

			}

			fuse_single_children(chld, level + 1, use_hex, fp);
			if(use_hex && (!big_endian)) {
				fprintf(fp, "%2x", chld->ch);
			}

			fputs(") {\n", fp);
		}

		if(chld->action) {
			fprintf(fp, "\n%s\ngoto after_fastkw;\n", chld->action);
		}

		fastkeywords_trie_to_code_lengthwise_nonroot(chld, level + 1, fp);
		if(!chld->cond_printed) {
			if(fastkeywords_fuse_single_child) {
				fputs("}\n", fp);
			} else {
				fprintf(fp, "} /* END if(nh->buf[nh->curtokpos + %d] == \'%c\') */\n", level, chld->ch);
			}

		}

		chld = trie_node_get_next(chld);
	}

}

int count_single_children(TrieNode *root)
{
	if(root->first_child && (!root->first_child->sibling)) {
		return (count_single_children(root->first_child)) + 1;
	}


	return 0;
}

void fuse_single_children(TrieNode *root, int level, _Bool use_hex, FILE * fp)
{
	if(root->first_child && (!root->first_child->sibling)) {
		if(use_hex && (!big_endian)) {
			fuse_single_children(root->first_child, level + 1, use_hex, fp);
		}

		if(use_hex) {
			fprintf(fp, "%2x", root->first_child->ch);
		} else {
			fprintf(fp, " && nh->buf[nh->curtokpos + %d] == \'%c\'", level, root->first_child->ch);
		}

		root->first_child->cond_printed = true;
		if((!use_hex) || big_endian) {
			fuse_single_children(root->first_child, level + 1, use_hex, fp);
		}

	}

}

