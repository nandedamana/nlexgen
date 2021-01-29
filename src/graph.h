/* graph.h
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020, 2021 Nandakumar Edamana
 * File started on 2021-01-29
 */

#ifdef DEBUG

#include <stdio.h>
#include <stdlib.h>

#include "tree.h"

void nan_graph_rec(NanTreeNode * node, FILE * fp);

static inline void nan_graph(NanTreeNode * root)
{
	FILE * fp = fopen("/dev/shm/nlexgen.gv", "w");
	if(!fp) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	fprintf(fp, "digraph {\n");
	nan_graph_rec(root, fp);
	fprintf(fp, "}\n");
}

#endif
