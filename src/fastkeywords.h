#ifndef _N96E_LEX_FASTKEYWORDS_H
#define _N96E_LEX_FASTKEYWORDS_H

#include "ctype.h"
#include "tree_types.h"
extern _Bool fastkeywords_enabled;
extern NanTreeNode *idactnode;
void fastkeywords_init(_Bool enabled);
_Bool is_fastkeyword(const char * pattern);

#endif /* _N96E_LEX_FASTKEYWORDS_H */
