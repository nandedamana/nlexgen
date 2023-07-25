#ifndef _N96E_LEX_FASTKEYWORDS_H
#define _N96E_LEX_FASTKEYWORDS_H

#include "ctype.h"
#include "tree_types.h"
extern _Bool fastkeywords_enabled;
extern _Bool fastkeywords_use_strcmp;
extern _Bool fastkeywords_use_length_based_trie;
extern _Bool fastkeywords_fuse_single_child;
extern _Bool fastkeywords_fuse_as_int;
extern _Bool big_endian;
extern NanTreeNode *idactnode;
typedef struct TrieNode TrieNode;
typedef struct _ngg_vtab_t_TrieNode {
} _ngg_vtab_t_TrieNode;

typedef struct TrieNode {
	_ngg_vtab_t_TrieNode _ngg_vtab_trie_node;
	TrieNode *first_child;
	TrieNode *last_child;
	TrieNode *sibling;
	const char * action;
	char ch;
	size_t keylen;
	_Bool cond_printed;
} TrieNode;

void trie_node_add(TrieNode *this, const char * key, int keyoffset, const char * action, _Bool lengthwise);
void trie_node_append(TrieNode *this, TrieNode *chld);
TrieNode* trie_node_get_next(TrieNode *this);
void trie_node_construct(TrieNode *this);
void trie_node_destruct(TrieNode *this);
void fastkeywords_init(_Bool enabled);
_Bool is_fastkeyword(const char * pattern);
TrieNode* fastkeywords_trie_new();
void fastkeywords_trie_to_code(TrieNode *root, int level, FILE * fp);
void fastkeywords_trie_to_code_not_lengthwise(TrieNode *root, int level, FILE * fp);
void fastkeywords_trie_to_code_lengthwise(TrieNode *root, int level, FILE * fp);
void fastkeywords_trie_to_code_lengthwise_nonroot(TrieNode *root, int level, FILE * fp);
void fastkeywords_trie_to_code_lengthwise_nonroot_nofuse_single_child(TrieNode *root, int level, FILE * fp);
void fastkeywords_trie_to_code_lengthwise_nonroot_fuse_single_child(TrieNode *root, int level, FILE * fp);
int count_single_children(TrieNode *root);
void fuse_single_children(TrieNode *root, int level, _Bool use_hex, FILE * fp);

#endif /* _N96E_LEX_FASTKEYWORDS_H */
