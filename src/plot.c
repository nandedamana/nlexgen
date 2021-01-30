/* plot.h
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020, 2021 Nandakumar Edamana
 * File started on 2021-01-29
 */

#ifdef DEBUG

#include <ctype.h>
#include <stdio.h>

#include "tree.h"

/* XXX Keep in sync with mrkrs_s */
const NlexCharacter mrkrs[] = { NLEX_CASE_ANYCHAR,
	NLEX_CASE_DIGIT,
	NLEX_CASE_EOF,
	NLEX_CASE_WORDCHAR,
	NLEX_CASE_LIST,
	NLEX_CASE_INVERT,
};

/* XXX Keep in sync with mrkrs */
const char * mrkrs_s[] = { "ANYCHAR",
	"DIGIT",
	"EOF",
	"WORDCHAR",
	"LIST",
	"INVERT",
	NULL
};

void nan_plot_rec(NanTreeNode * node, FILE * fp)
{
	fprintf(fp, "%d", node->id);

	if(isprint(node->ch)) {
		if(node->ch == '"')
			fprintf(fp, "[label=\"%d: '\\\"'\"]", node->id);
		else
			fprintf(fp, "[label=\"%d: '%c'\"]", node->id, node->ch);
	}
	else {
		fprintf(fp, "[label=\"%d: %d", node->id, node->ch);
		
		for(size_t i = 0; mrkrs_s[i]; i++)
			if(-(node->ch) & mrkrs[i])
				fprintf(fp, "\n%s\n", mrkrs_s[i]);
		
		if(-(node->ch) & NLEX_CASE_LIST) {
			fprintf(fp, "\n");
			nan_character_list_to_expr(NAN_CHARACTER_LIST(node->ptr), "ch", fp);
		}

		fprintf(fp, "\"]");
	}

	fprintf(fp, ";\n");

	for(NanTreeNode * chld = node->first_child; chld; chld = chld->sibling) {
		nan_plot_rec(chld, fp);
		fprintf(fp, "%d -> %d;\n", node->id, chld->id);
	}
	
	if(node->klnptr)
		fprintf(fp, "%d -> %d;\n", node->id, node->klnptr->id);
}

#endif
