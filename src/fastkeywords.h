#ifndef _N96E_LEX_FASTKEYWORDS_H
#define _N96E_LEX_FASTKEYWORDS_H

#include "ctype.h"
#include "tree_types.h"
extern _Bool fastkeywords_enabled;
extern _Bool fastkeywords_use_strcmp;
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
} TrieNode;

void trie_node_add(TrieNode *this, const char * key, int keyoffset, const char * action);
void trie_node_append(TrieNode *this, TrieNode *chld);
TrieNode* trie_node_get_next(TrieNode *this);
void trie_node_construct(TrieNode *this);
void trie_node_destruct(TrieNode *this);
void fastkeywords_init(_Bool enabled);
_Bool is_fastkeyword(const char * pattern);
TrieNode* fastkeywords_trie_new();
void fastkeywords_trie_to_code(TrieNode *root, int level, FILE * fp);

#endif /* _N96E_LEX_FASTKEYWORDS_H */
