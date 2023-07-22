#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "fastkeywords.h"

_Bool fastkeywords_enabled;
NanTreeNode *idactnode;
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

