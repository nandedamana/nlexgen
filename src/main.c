/* main.c
 * This file is part of nlexgen, a lexer generator.
 * Copyright (C) 2019, 2020 Nandakumar Edamana
 * Started on 2019-07-22
 */

#include <memory.h>

#include "read.h"
#include "tree.h"

NanTreeNodeId id_lastact    = 0;
NanTreeNodeId id_lastnonact = 1; /* First one used for the root */

/* Conversion of action nodes */
void nan_tree_astates_to_code(NanTreeNode * root, _Bool if_printed)
{
	NanTreeNode * tptr;

	if(root->ch == NLEX_CASE_ACT) {
		if(if_printed)
			fprintf(fpout, "else ");
		else
			if_printed = 1;

		fprintf(fpout, "if(nh->last_accepted_state == %d) {\n"
			"nh->bufptr = nh->buf + nh->lastmatchat;\n%s\n}\n",
			nan_tree_node_id(root),
			(char *) root->ptr);

		return;
	}

	for(tptr = root->first_child; tptr; tptr = tptr->sibling) {
		nan_tree_astates_to_code(tptr, if_printed);
	}
	
	return;
}

/* Conversion of intermediate nodes */
/* XXX The resulting code will go through an extra step to reach
 * the action node.
 * This was to make the longest rule preferable (on collision), IIRC.
 * TODO do more research.
 */
void nan_tree_istates_to_code(NanTreeNode * root, NanTreeNode * grandparent)
{
	NanTreeNode * tptr;

	for(tptr = root->first_child; tptr; tptr = tptr->sibling) {
		fprintf(fpout, "if((nh->curstate == %d", nan_tree_node_id(root));

		/* Bypass */
		if(root->ch < 0 && -(root->ch) & NLEX_CASE_KLEENE)
			fprintf(fpout, " || nh->curstate == %d", nan_tree_node_id(grandparent));
		
		/* Self loop */
		if(tptr->ch < 0 && -(tptr->ch) & NLEX_CASE_KLEENE)
			fprintf(fpout, " || nh->curstate == %d", nan_tree_node_id(tptr));

		_Bool printed = 0;

// TODO make the branching better

		if(tptr->ch < 0) { /* Special cases */
			if(-(tptr->ch) & NLEX_CASE_LIST) {
				fprintf(fpout, ") && ");

				if(-(tptr->ch) & NLEX_CASE_INVERT)
					fprintf(fpout, "!(");
				else
					fprintf(fpout, "(");
				
				nan_character_list_to_expr(NAN_CHARACTER_LIST(tptr->ptr), "ch", fpout);
				printed = 1;
			}
			else if(tptr->ch == NLEX_CASE_ACT) {
// TODO FIXME
//				fprintf(fpout, ") && nlex_nstack_is_empty(nh");

// TODO FIXME enabling again for semicolon in nguigen
				fprintf(fpout, ") && (1");
				printed = 1;
			}
		}

		if(!printed) {
			fprintf(fpout, ") && (");
			nan_character_print_c_comp(tptr->ch, "ch", fpout);
		}

		fprintf(fpout, ")) {\n");

		if(tptr->ch == NLEX_CASE_ACT) {
			fprintf(fpout,
				"\tif(%d < hiprio_act_this_iter)\n"
				"\t\thiprio_act_this_iter = %d;\n",
				nan_tree_node_id(tptr),
				nan_tree_node_id(tptr));
		}
		else { /* Non-action node */
			/* Usually Kleene star nodes push themselves into the next-stack.
			 * But upon reaching a next-to-wildcard match, I've to remove the
			 * Kleene state from the stack. This is to prevent 'cde' from
			 * being consumed in 'axyzbcde' against the regex 'a*bcde'
			 */
			if(root->ch < 0 && -(root->ch) & NLEX_CASE_KLEENE) {
				fprintf(fpout,
					/* checking again to skip grandparents */
					"\nif(nh->curstate == %d) nlex_nstack_remove(nh, %d);\n",
					nan_tree_node_id(root),
					nan_tree_node_id(root));
			}
					
			fprintf(fpout, "\t/* Useful for line counting, col counting, etc. */\n"
				"	if(!on_consume_called && nh->on_consume) { nh->on_consume(nh); on_consume_called = 1; }\n");
			fprintf(fpout, "\tnh->lastmatchat = (nh->bufptr - nh->buf);\n");

			/* Push itself onto the next-stack */
			fprintf(fpout, "\tnlex_nstack_push(nh, %d);\n", nan_tree_node_id(tptr));
		}

		fprintf(fpout, "}\n");
		nan_tree_istates_to_code(tptr, root);
	}
	
	return;
}

int main(int argc, char * argv[])
{
	FILE *fpin;

	if(argc == 2) {
		fpin = fopen(argv[1], "r");
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
	nan_tree_build(&troot, nh);

	fpout = stdout;

	/* BEGIN Code Generation */
	
	// TODO FIXME
	//#define DEBUG 1
	
	#ifdef DEBUG
		nan_tree_dump(&troot, 0);
		fprintf(stderr, "tree dump complete.\n");
	#endif

// TODO FIXME action nodes should not be expanded into the same loop where regular states are compared. Put them outside the scanner loop so that less comparisons are made.

	fprintf(fpout,
		"nlex_reset_states(nh);\n"
		"nlex_nstack_push(nh, %d);\n",
		troot.id);
	fprintf(fpout,
// TODO rem
//		"while(!(nh->done) && !nlex_nstack_is_empty(nh)) {\n"
		"while(!nlex_nstack_is_empty(nh)) {\n"
		"nlex_nstack_fix_actions(nh);\n" // TODO FIXME update since I've moved the action nodes out of the stack
		"if(nh->fp) {\n" /* Reading from file */
			"\tif(nh->buf && (nlex_last(nh) == 0))\n\t\tbreak;\n"
		"}"
		"else {\n"        /* Reading from string */
			"\tif(nh->bufptr >= nh->buf && nlex_last(nh) == 0)\n\t\tbreak;\n"
		"}"
#ifdef DEBUG
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
		"if(nlex_tstack_has_non_action_nodes(nh)) { ch = nlex_next(nh); }\n"
		"_Bool on_consume_called = 0; // see ngg bug 20200831000\n"
		"unsigned int hiprio_act_this_iter = UINT_MAX;\n"
		"while(!nlex_tstack_is_empty(nh)) {\n"
		"nh->curstate = nlex_tstack_pop(nh);\nif(nh->curstate == 0) continue;\n");
#ifdef DEBUG
		fprintf(fpout,
		"\tfprintf(stderr, "
		"\t\t\"curstate = %%d\\n\", nh->curstate);\n");
#endif
	nan_tree_istates_to_code(&troot, NULL);
	fprintf(fpout, "\n}\n"
		"if(hiprio_act_this_iter != UINT_MAX) nh->last_accepted_state = hiprio_act_this_iter;"
		"}\n");
#ifdef DEBUG
	fprintf(fpout,
		"\tfprintf(stderr, "
		"\t\t\"enterng anode comparison with nh->last_accepted_state = %%d\\n\", nh->last_accepted_state);\n");
#endif
	nan_tree_astates_to_code(&troot, 0);

	/* END Code Generation */

	return 0;
}
