/* main.c
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020, 2021, 2023, 2026 Nandakumar Edamana
 * Started on 2019-07-22
 */

#define PROGRAM_NAME "nlexgen"
#define VERSION "0.3.0"

#include <memory.h>

#include "error.h"
#include "fastkeywords.h"
#include "read.h"
#include "tree.h"
#include "plot.h"

int main(int argc, char * argv[])
{
	FILE * fpin = stdin;
	bool simplify = true;
	char * outpath_gv = NULL;
	bool clopt_fastkw = false;
	bool do_consume_callback = true;
	char * function_header = NULL;
	char * function_epilogue = NULL;
	
	// XXX Implemented and tested on 2023-04-07; no performance gain because:
	// 1) The current implementation was already using nested ifs to reduce comparison
	// 2) Since label-as-value works only in the local scope, it's impossible to
	//    put the table as a global constant, resulting in init every time (memcpy)
	// 3) The table is large, just to allocate NULLs that are the result of action
	//    nodes which are not to be considered?
	//    - NOTE: additionally, when the tree is simplified, the count of nodes
	//            decreases, but the numbering is kept to preserve priorities.
	//            (TODO do tree renumbering preserving the order)
	bool use_jmptab = false;

	if(argc > 1) {
		int i = 0;
	
		while(++i < argc) {
			if(0 == strcmp(argv[i], "--fastkeywords")) {
				// TODO FIXME
				nlex_die("fastkeywords is disabled because the generated code does not make sure the buffer is long enough before accessing it using `nh->buf[nh->curtokpos + n]`.");
			
				clopt_fastkw = true;
			}
			else if(0 == strcmp(argv[i], "--no-simplify")) {
				simplify = false;
			}
			else if(0 == strcmp(argv[i], "--no-consume-callback")) {
				do_consume_callback = false;
			}
			else if(0 == strcmp(argv[i], "--function-epilogue")) {
				i++;
				if(argc <= i)
					nlex_die("No path given after --function-epilogue.");
				function_epilogue = argv[i];
			}
			else if(0 == strcmp(argv[i], "--function-header")) {
				i++;
				if(argc <= i)
					nlex_die("No path given after --function-header.");
				function_header = argv[i];
			}
			else if(0 == strcmp(argv[i], "--gv")) {
				i++;

				if(argc <= i)
					nlex_die("No path given after --gv.");
			
				outpath_gv = argv[i];
			}
			else if(0 == strcmp(argv[i], "--version")) {
				puts(PROGRAM_NAME" version "VERSION);
				exit(EXIT_SUCCESS);
			}
			// The patterns all are static keywords (no wildcard or anything), the buffer being lexed is a NUL-string
			else if(0 == strcmp(argv[i], "--zstr2deterkw")) {
				zstr2deterkw = true;
			}
			else if(0 == strcmp(argv[i], "--x-use-jump-table")) {
				use_jmptab = true;
			}
			else {
				if(i != argc - 1)
					nlex_die("Options given after the input file path.");

				fpin = fopen(argv[i], "r");
				if(!fpin)
					nlex_die("Error opening the input file.");
			}
		}
	}

	// Whether to generate hash-table-based keyword detection (instead of state
	// machine); it's assumed:
	//  1) Keywords are of English lowercase letters only
	//  2) The regex pattern for keywords represent a subset of IDs
	//  3) The rule for IDs is present in the input and it is the last one
	//  4) The rule for IDs conforms to that of nguigen
	fastkeywords_init(clopt_fastkw);

	NlexHandle *  nh;
	nh = nlex_handle_new();
	if(!nh)
		nlex_die("nlex_handle_new() returned NULL.");

	nlex_init(nh, fpin, NULL);

	NanTreeNode troot;
	const char * err = nlg_build_tree(&troot, nh);
	if(err != NLEXERR_SUCCESS)
		nlex_die(err);

	if(simplify) {
		nan_tree_number(&troot); // do it first to preserve priorities
		nan_tree_simplify(&troot);
	}

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

	if(function_header) {
		fprintf(fpout, "%s\n{\n", function_header);
	}

	// Can't move out of the fun to global scope because only local
	// addresses can be taken.
	if(use_jmptab) {
		nan_tree_unvisit(&troot);
		Jmptab jmptabinfo = nan_tree_istates_to_code_mkjmptab(&troot);
		char ** jmptab = jmptabinfo.arr;
		size_t jmptabsiz = jmptabinfo.len;
		fprintf(fpout, "void * jmptab[%zu] = { ", jmptabsiz);
		for(size_t i = 0; i < jmptabsiz; i++)
			fprintf(fpout, "%s, ", jmptab[i]? jmptab[i]: "NULL");
		fprintf(fpout, "};\n");

		for(size_t i = 0; i < jmptabsiz; i++)
			free(jmptab[i]);
		free(jmptab);
	}

// TODO FIXME action nodes should not be expanded into the same loop where regular states are compared. Put them outside the scanner loop so that less comparisons are made.
// NOTE: Commits made on or just before 2021-05-22 do
// something similar. Check. I think splitting the loop would
// be a further improvement.

	if(zstr2deterkw) {
		fprintf(fpout,
				"nh->curstate = %d;\n",
				troot.id);


		fprintf(fpout,
			"nh->curtokpos = 0;\n"
			"nh->curtoklen = 0;\n"
			"_Bool reject = 0;\n"
			"while(!nlex_end_of_input(nh) && nh->last_accepted_state == 0 && !reject) {\n"
				"char ch = nlex_next(nh);\n"
				"size_t ch_read_after_accept = 0;\n" /* TODO REM? */
				"int lastmatchat = -1;\n");
	}
	else {
		fprintf(fpout,
			"if(!nlex_end_of_input(nh)) {\n"
				"char ch = 0;\n"
				"_Bool ch_set = 0;\n"
				"size_t ch_read_after_accept = 0;\n"
				"int lastmatchat = -1;\n"
				// TODO why aren't these part of reset_states()?
				"nh->curtokpos = nh->bufptr - nh->buf + 1;\n"
				"nh->curtoklen = 0;\n");
	}

	if(fastkeywords_enabled) {
		fprintf(fpout,
			"_Bool couldbekw = 0;\n"
			"_Bool couldbeid = 0;\n"
			// 'v' not accepted because it could start v, vh and vtop lines (ngg)
			"if( islower(ch = nlex_next(nh)) && ch != 'v' ) {\n"
				"while(islower(ch)) { nh->curtoklen++; ch = nlex_next(nh); }\n"
				"\n"
				"/* Maybe a keyword ended now, or it is an ID and it continues, or it is something like L\"wcharstr\" */\n"
				"int curtoklenbak = nh->curtoklen;\n"
				"while(isalpha(ch) || isdigit(ch) || ch == '_' || ch == '-') { nh->curtoklen++; ch = nlex_next(nh); }\n"
				"couldbeid = (ch != '\\'' && ch != '\\\"' && ch != '='); /* e.g.: L\"wcharstr\", sum= */\n"
				"couldbekw = couldbeid && (nh->curtoklen == curtoklenbak); /* No trailing quotes and no id-exclusive chars after keyword match */\n"

				"if(couldbeid && !couldbekw) {\n");
					nlg_gen_fastkw_onid(&troot);
		fprintf(fpout,
				"} else if(couldbekw) {\n");
		if(fastkeywords_use_strcmp)
			nlg_gen_fastkw_selection_strcmp(&troot);
		else
			nlg_gen_fastkw_selection_trie(&troot);
		fprintf(fpout,
				"} else {\n"
					"nh->bufptr = nh->buf + nh->curtokpos - 1;\n"
					"nh->curtoklen = 0;\n"
				"}\n");

		fprintf(fpout,
			"} else { /* Read only one char and it wasn't a fastkeyword/id starter */\n"
				"nh->bufptr = nh->buf + nh->curtokpos - 1;\n"
			"}\n");
	}

	if(!zstr2deterkw) {
		fprintf(fpout,
				"nlex_reset_states(nh);\n"
				"nlex_nstack_push(nh, %d);\n",
				troot.id);
		fprintf(fpout,
				"while(!nlex_nstack_is_empty(nh)) {\n");

	#ifdef NLXDEBUG
		fprintf(fpout,
					"if(nh->buf && nh->bufptr >= nh->buf)\n" // TODO is the first `nh->buf` needed?
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
						"size_t nstack_top_bak = nh->nstack_top;\n"
						"nh->curstate = nlex_tstack_pop(nh);\n"
						"if(nh->curstate == 0) continue;\n");
	}

#ifdef NLXDEBUG
	fprintf(fpout,
					"fprintf(stderr, \"curstate = %%d\\n\", nh->curstate);\n");
#endif

	nan_tree_unvisit(&troot);
	
	if(!use_jmptab) {
		//nan_tree_istates_to_code(&troot, false);
		fprintf(fpout, "switch(nh->curstate) {\n");
		nan_tree_istates_to_code_switch(&troot);
		fprintf(fpout, "}\n");
	}
	else {
		fprintf(fpout, "assert(jmptab[nh->curstate]);\n"); // TODO REM or make debug-only
		fprintf(fpout, "goto *(jmptab[nh->curstate]);\n");

		nan_tree_istates_to_code_jmp(&troot);
		fprintf(fpout, "endjmp:\n");
	}
	
	if(zstr2deterkw) {
		// lastmatchat set during the node code generation
	}
	else {
		fprintf(fpout,
						"if(nh->nstack_top != nstack_top_bak) lastmatchat = (nh->bufptr - nh->buf);\n"
					"} /* end while tstack */\n"
					"assert(nlex_tstack_is_empty(nh));\n"

					"if(hiprio_act_this_iter != UINT_MAX) { nh->last_accepted_state = hiprio_act_this_iter; } \n"
					// TODO REM
					"//if(ch == EOF || ch == '\\0') { assert(nlex_nstack_is_empty(nh)); break; }\n" // TODO done above too. Why twice?
				"} /* end while nstack */\n");
	}

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
				"assert(nh->curtokpos >= 0);\n"
				"nh->bufptr = nh->buf + nh->curtokpos + nh->curtoklen - 1; /* means backtracking if there was a longer partial match (resetting bufptr is needed in every case though) */"
				"switch(nh->last_accepted_state) {\n");
	nan_tree_astates_to_code(&troot, do_consume_callback);
	fprintf(fpout,
				"}\n");

	if(fastkeywords_enabled)
		fprintf(fpout, "after_fastkw:\n");

	fprintf(fpout,
			"} /* endif last_accepted_state */\n");
	fprintf(fpout, "} /* endif not end of input */ \n");

	if(function_header) {
		if(function_epilogue)
			fprintf(fpout, "%s\n", function_epilogue);

		fprintf(fpout, "}\n");
	}
	/* END Code Generation */

	nlex_handle_destruct(nh);
	free(nh);

	return 0;
}
