#ifndef _N96E_LEX_TREEBUILD_H
#define _N96E_LEX_TREEBUILD_H

typedef struct vstring vstring;
typedef struct _ngg_vtab_t_vstring {
} _ngg_vtab_t_vstring;

typedef struct vstring {
	_ngg_vtab_t_vstring _ngg_vtab_vstring;
	char *s;
	size_t len;
} vstring;

static inline char* vstring_get(vstring * this) {
return this->s;
}
static inline size_t vstring_get_length(vstring * this) {
return this->len;
}
#include "types.h"
#include "tree_types.h"
#include "error.h"
#include "read.h"
const char * nlg_tree_add_rule(
	NanTreeNode * root, NlexHandle * nh, const char * pattern, const char * action);
void nlg_tree_init_root(NanTreeNode * root);
typedef struct _ngg_tuple_nlg_get_rule {
	char * m0;
	char * m1;
	const char * m2;
} _ngg_tuple_nlg_get_rule;

void vstring_construct(vstring *this, const char * inits);
void vstring_append(vstring *this, const char * suffix);
void vstring_appendc(vstring *this, char suffix);
void vstring_clear(vstring *this);
char * vstring_detach(vstring *this);
void vstring_destruct(vstring *this);
_ngg_tuple_nlg_get_rule nlg_get_rule(NlexHandle *nh);
const char * nlg_build_tree(NanTreeNode *root, NlexHandle *nh);
_ngg_tuple_nlg_get_rule _ngg_tuple_nlg_get_rule_default();

#endif /* _N96E_LEX_TREEBUILD_H */
