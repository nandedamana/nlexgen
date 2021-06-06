/* plot.h
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020, 2021 Nandakumar Edamana
 * File started on 2021-01-29
 */


#include <stdio.h>
#include <stdlib.h>

#include "tree.h"

void nan_plot_rec(NanTreeNode * node, FILE * fp);

static inline void nan_plot(NanTreeNode * root, FILE * fp)
{
	nan_tree_unvisit(root);

	fprintf(fp, "digraph {\n");
	nan_plot_rec(root, fp);
	fprintf(fp, "}\n");
}

