/* main.c
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020, 2021 Nandakumar Edamana
 * Started on 2019-07-22
 */

#include <memory.h>

#include "error.h"
#include "read.h"
#include "tree.h"
#include "plot.h"

int main(int argc, char * argv[])
{
	FILE *fpin = NULL;
	bool simplify = true;

	size_t filearg = 1;

	if(argc > 1) {
		if(0 == strcmp(argv[1], "--no-simplify")) {
			simplify = false;
			filearg++;
		}
	}

	if(argc > filearg) {
		fpin = fopen(argv[filearg], "r");
		if(!fpin)
			nlex_die("Error opening the input file.");
	}
	else {
		fpin = stdin;
	}

	NlexHandle *  nh;
	nh = nlex_handle_new();
	if(!nh)
		nlex_die("nlex_handle_new() returned NULL.");

	nlex_init(nh, fpin, NULL); // TODO FIXME not always stdin

	NanTreeNode troot;
	const char * err = nan_tree_build(&troot, nh);
	if(err != NLEXERR_SUCCESS)
		nlex_die(err);

	if(simplify)
		nan_tree_simplify(&troot);

	nan_tree_unvisit(&troot);
	// TODO FIXME currently both this fun and other funs use nan_tree_node_id(), which sets the id if not set already. split it as getter and setter. Only this fun should use the setter.
	nan_tree_assign_node_ids(&troot);
	
	nan_tree_unvisit(&troot);
	nan_assert_all_nodes_have_id(&troot);

	fpout = stdout;

	/* BEGIN Code Generation */
	
	// TODO FIXME
	//#define DEBUG 1
	
	#ifdef DEBUG
		nan_tree_unvisit(&troot);
		nan_plot(&troot);
		fprintf(stderr, "tree dump complete.\n");
	#endif

// TODO FIXME action nodes should not be expanded into the same loop where regular states are compared. Put them outside the scanner loop so that less comparisons are made.
// NOTE: Commits made on or just before 2021-05-22 do
// something similar. Check. I think splitting the loop would
// be a further improvement.

	// TODO why do I have both hiprio_act_this_iter and nh->last_accepted_state? Find and doc. Or did I introduce nh->last_accepted_state just keep record of it? If so, it's useless now that I reset to 0 for performance reasons.

	fprintf(fpout,
		"char ch = 0;\n"
		"_Bool ch_set = 0;\n"
		"size_t ch_read_after_accept = 0;\n"
		"int lastmatchat = -1;\n"
		"nh->curtokpos = nh->bufptr - nh->buf + 1;\n"
		"nh->curtoklen = 0;\n"
		"nh->last_accepted_state = 0;\n"
		"nlex_reset_states(nh);\n"
		"nlex_nstack_push(nh, %d);\n",
		troot.id);
	fprintf(fpout,
// TODO rem
//		"while(!(nh->done) && !nlex_nstack_is_empty(nh)) {\n"
		"while(!nlex_nstack_is_empty(nh)) {\n"
		"nlex_nstack_fix_actions(nh);\n" // TODO FIXME update since I've moved the action nodes out of the stack
		
		// TODO why not just use nh->eof_read?
		"if(nh->fp) {\n" /* Reading from file */
			"\tif(nh->buf && (nlex_last(nh) == 0))\n\t\tbreak;\n"
		"}"
		"else {\n"        /* Reading from string */
			"\tif(nh->bufptr >= nh->buf && nlex_last(nh) == 0)\n\t\tbreak;\n"
		"}"
#ifdef NLXDEBUG
// TODO rem because slow - or truncate the output (and print a notice about it).
//		"if(nh->buf)"
//		"\tfprintf(stderr, "
//		"\t\t\"bufptr:\\n\"); nlex_debug_print_bufptr(nh, stderr); puts(\"\");\n"

		"if(nh->buf)"		
		"\tfprintf(stderr, "
		"\t\t\"nstack after the iteration that read %%d ('%%c'):\\n\", nlex_last(nh), nlex_last(nh));\n"
		"else"
		"\tfprintf(stderr, "
		"\t\t\"nstack:\\n\");\n"
		"nlex_nstack_dump(nh);\n"
#endif
		"nlex_swap_t_n_stacks(nh);\n"
		// TODO FIXME
//		"if(nlex_tstack_has_non_action_nodes(nh)) { ch = nlex_next(nh); }else {nlex_die(\"OK\");}\n"
		"if(nlex_tstack_has_non_action_nodes(nh)) { ch = nlex_next(nh); ch_set = 1; ch_read_after_accept++; }\n"
		"unsigned int hiprio_act_this_iter = UINT_MAX;\n"
		"while(!nlex_tstack_is_empty(nh)) {\n"
		"assert(ch_set);\n"
		"_Bool match = 0;\n"
		"nh->curstate = nlex_tstack_pop(nh);\nif(nh->curstate == 0) continue;\n");
#ifdef NLXDEBUG
		fprintf(fpout,
		"\tfprintf(stderr, "
		"\t\t\"curstate = %%d\\n\", nh->curstate);\n");
#endif
	nan_tree_unvisit(&troot);
	nan_tree_istates_to_code(&troot, false);
	fprintf(fpout, "if(match) lastmatchat = (nh->bufptr - nh->buf);\n");
	fprintf(fpout, "}\n"
		"if(hiprio_act_this_iter != UINT_MAX) { nh->last_accepted_state = hiprio_act_this_iter; } \n");
	fprintf(fpout, "if(nh->eof_read) break;\n"); // TODO EOF checking done above too. Why twice?
	fprintf(fpout, "}\n");
#ifdef NLXDEBUG
	fprintf(fpout,
		"\tfprintf(stderr, "
		"\t\t\"before anode comparison, nh->last_accepted_state = %%d\\n\", nh->last_accepted_state);\n");
#endif
	nan_tree_unvisit(&troot);

	fprintf(fpout, "if(nh->last_accepted_state != 0) {\n");
	// TODO rem ch_read_after_accept if it'll always be 0
	fprintf(fpout, "\tnh->curtoklen = lastmatchat - nh->curtokpos - ch_read_after_accept + 1; assert(nh->curtoklen > 0);\n");
	fprintf(fpout, "switch(nh->last_accepted_state) {\n");
	nan_tree_astates_to_code(&troot);
	fprintf(fpout, "}\n");
	fprintf(fpout, "assert(nh->curtoklen > 0);\n");

	fprintf(fpout,
		"assert(nh->curtokpos >= 0);"
		"nh->bufptr = nh->buf + nh->curtokpos + nh->curtoklen - 1; /* means backtracking if there was a longer partial match (resetting bufptr is needed in every case though) */\n");
	fprintf(fpout, "}\n");

	/* END Code Generation */

	return 0;
}
