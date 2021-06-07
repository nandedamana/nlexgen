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
	FILE * fpin = stdin;
	bool simplify = true;
	char * outpath_gv = NULL;

	if(argc > 1) {
		size_t i = 0;
	
		while(++i < argc) {
			if(0 == strcmp(argv[i], "--no-simplify")) {
				simplify = false;
			}
			else if(0 == strcmp(argv[i], "--gv")) {
				i++;

				if(argc <= i)
					nlex_die("No path given after --gv.");
			
				outpath_gv = argv[i];
			}
			else {
				if(i != argc - 1)
					nlex_die("Arguments given after the input file path.");

				fpin = fopen(argv[i], "r");
				if(!fpin)
					nlex_die("Error opening the input file.");
			}
		}
	}

	NlexHandle *  nh;
	nh = nlex_handle_new();
	if(!nh)
		nlex_die("nlex_handle_new() returned NULL.");

	nlex_init(nh, fpin, NULL);

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
	
	if(outpath_gv) {
		FILE * fpo = fopen(outpath_gv, "w");
		if(!fpo)
			nlex_die("Error opening the input file.");

		nan_plot(&troot, fpo);

		fclose(fpo);
	}

// TODO FIXME action nodes should not be expanded into the same loop where regular states are compared. Put them outside the scanner loop so that less comparisons are made.
// NOTE: Commits made on or just before 2021-05-22 do
// something similar. Check. I think splitting the loop would
// be a further improvement.

	fprintf(fpout,
		"if(!nlex_end_of_input(nh)) {\n"
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
			"while(!nlex_nstack_is_empty(nh)) {\n");

#ifdef NLXDEBUG
	fprintf(fpout,
				"if(nh->buf)\n"
					"fprintf(stderr, "
						"\"nstack after the iteration that read %%d ('%%c'):\\n\", nlex_last(nh), nlex_last(nh));\n"
				"else\n"
					"fprintf(stderr, \"nstack:\\n\");\n"

				"nlex_nstack_dump(nh);\n");
#endif

	fprintf(fpout,
				"nlex_swap_t_n_stacks(nh);\n"
				"assert(nlex_nstack_is_empty(nh));\n"
				"if(!nlex_tstack_is_empty(nh)) {\n"
					"ch = nlex_next(nh); ch_set = 1; ch_read_after_accept++;"
				"}\n"
				
				/* We need to move on even if the input has ended (ch == 0 || ch == EOF)
				 * since the stacks can have states that do not need any input (like
				 * the states to select an action.
				 */
				
				"unsigned int hiprio_act_this_iter = UINT_MAX;\n"
				"while(!nlex_tstack_is_empty(nh)) {\n"
					"assert(ch_set);\n"
					"_Bool match = 0;\n"
					"nh->curstate = nlex_tstack_pop(nh);\n"
					"if(nh->curstate == 0) continue;\n");

#ifdef NLXDEBUG
	fprintf(fpout,
					"fprintf(stderr, \"curstate = %%d\\n\", nh->curstate);\n");
#endif

	nan_tree_unvisit(&troot);
	nan_tree_istates_to_code(&troot, false);
	fprintf(fpout,
					"if(match) lastmatchat = (nh->bufptr - nh->buf);\n"
				"} /* end while tstack */\n"
				"assert(nlex_tstack_is_empty(nh));\n"

				"if(hiprio_act_this_iter != UINT_MAX) { nh->last_accepted_state = hiprio_act_this_iter; } \n"
				// TODO REM
				"//if(ch == EOF || ch == '\\0') { assert(nlex_nstack_is_empty(nh)); break; }\n" // TODO done above too. Why twice?
			"} /* end while nstack */\n");

#ifdef NLXDEBUG
	fprintf(fpout,
			"fprintf(stderr, "
				"\"before anode comparison, nh->last_accepted_state = %%d\\n\", nh->last_accepted_state);\n");
#endif

	nan_tree_unvisit(&troot);

	fprintf(fpout,
			"if(nh->last_accepted_state != 0) {\n"
				// TODO rem ch_read_after_accept if it'll always be 0
				"nh->curtoklen = lastmatchat - nh->curtokpos - ch_read_after_accept + 1;\n"
				"assert(nh->curtoklen > 0);\n"
				"switch(nh->last_accepted_state) {\n");
	nan_tree_astates_to_code(&troot);
	fprintf(fpout,
				"}\n");

	fprintf(fpout,
				"assert(nh->curtoklen > 0);\n"
				"assert(nh->curtokpos >= 0);\n"
				"nh->bufptr = nh->buf + nh->curtokpos + nh->curtoklen - 1; /* means backtracking if there was a longer partial match (resetting bufptr is needed in every case though) */\n"
			"} /* endif last_accepted_state */\n");
	fprintf(fpout, "} /* endif not end of input */ \n");
	/* END Code Generation */

	return 0;
}
