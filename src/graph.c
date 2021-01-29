/* graph.h
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020, 2021 Nandakumar Edamana
 * File started on 2021-01-29
 */

#ifdef DEBUG

#include <ctype.h>
#include <stdio.h>

#include "tree.h"

void nan_graph_rec(NanTreeNode * node, FILE * fp)
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
		
		if(-(node->ch) & NLEX_CASE_KLEENE)
			fprintf(fp, "\nKLEENE\n");
		
		if(-(node->ch) & NLEX_CASE_LIST) {
			fprintf(fp, "\n");
			nan_character_list_to_expr(NAN_CHARACTER_LIST(node->ptr), "ch", fp);
		}

		fprintf(fp, "\"]");
	}

	fprintf(fp, ";\n");

	for(NanTreeNode * chld = node->first_child; chld; chld = chld->sibling) {
		nan_graph_rec(chld, fp);
		fprintf(fp, "%d -> %d;\n", node->id, chld->id);
	}
}

#endif
