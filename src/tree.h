/* Nandakumar Edamana
 * Started on 2019-07-22
 */

#include "error.h"

#define NLEX_CASE_NONE -1
#define NLEX_CASE_ROOT -2
#define NLEX_CASE_ELSE -3
#define NLEX_CASE_LIST -4

#define NAN_CHARACTER_LIST(x) ((NanCharacterList *) (x))

/* 'Nan' prefix is used to distinguish internal components from
 * the components that can be used by other programs.
 */

typedef struct _NanTreeNode {
  NlexCharacter         ch;

	/* If ch is NLEX_CASE_ELSE, ptr points to the user-given output code.
	 * If ch is NLEX_CASE_LIST, ptr points to the a NanCharacter array.
	 * Otherwise unused and may hold junk value.
	 */
  void                * ptr;

	struct _NanTreeNode * first_child;
	struct _NanTreeNode * sibling;
} NanTreeNode;

/* To match multiple characters at a time */
typedef struct _NanCharacterList {
	NlexCharacter * list;
	size_t          count;
} NanCharacterList;

/* Print a character to the C source code output with escaping if needed */
static inline void nan_c_print_character(char c, FILE * fp)
{
	int escin = nlex_get_counterpart(c, escout_c, escin_c);

	if(escin == NAN_NOMATCH)
		fprintf(fp, "'%c'", c);
	else if(escin == EOF)
		fprintf(fp, "EOF");
	else
		fprintf(fp, "'\\%c'", escin);
}

static inline void
	nan_character_list_append(NanCharacterList * ncl, NlexCharacter c)
{
	ncl->list = realloc(ncl->list, sizeof(NlexCharacter) * (ncl->count + 1));
	if(!ncl->list)
		die("realloc() error.");

	ncl->list[ncl->count++] = c;
}

static inline NanCharacterList * nan_character_list_new()
{
	NanCharacterList * ncl = malloc(sizeof(NanCharacterList));
	if(!ncl)
		die("malloc() error.");
	
	ncl->count = 0;
	ncl->list  = NULL;

	return ncl;
}

/* Character list to Boolean expression */
static inline void
	nan_character_list_to_expr(NanCharacterList * ncl, FILE * fp)
{
	int i;
	
	if(ncl->count) {
		fprintf(fp, "ch == ");
		nan_c_print_character(ncl->list[0], fp);
	
		for(i = 1; i < ncl->count; i++) {
			fprintf(fp, " || ch == ");
			nan_c_print_character(ncl->list[i], fp);
		}
	}
}

/* TODO This comparison is order-sensitive for lists. */
static inline _Bool
	nan_tree_nodes_match(NanTreeNode * node1, NanTreeNode * node2)
{
	_Bool matches;	
	
	if(node1->ch == NLEX_CASE_LIST) {
		if(node2->ch == NLEX_CASE_LIST) {
			matches = 1;

			int i = 0;
			
			while(i < NAN_CHARACTER_LIST(node1->ptr)->count  &&
				i < NAN_CHARACTER_LIST(node2->ptr)->count)
			{
				if(NAN_CHARACTER_LIST(node1->ptr)->list[i] !=
					NAN_CHARACTER_LIST(node2->ptr)->list[i])
				{
					matches = 0;
					break;
				}
				
				i++;
			}
			
			/* XXX I can free one list and reuse the pointer to the other
			 * in case of a match to save memory.
			 * But it has no practical advantage for a lexer.
			 */
		}
		else {
			matches = 0;
		}
	}
	else {
		matches = (node1->ch == node2->ch);
	}
	
	return matches;
}
