/* Nandakumar Edamana
 * Started on 2019-07-22
 */

#include "error.h"

#define NLEX_CASE_NONE    -1
#define NLEX_CASE_ROOT    -2
#define NLEX_CASE_ACT     -4

/* Will be negated. */
#define NLEX_CASE_ANYCHAR    8
#define NLEX_CASE_DIGIT     16
#define NLEX_CASE_EOF       32
#define NLEX_CASE_WORDCHAR  64
#define NLEX_CASE_LIST     128

/* Will be ORed with NanTreeNode.ch for lists.
 * Single character nodes will be converted to lists to enable Kleene.
 */
#define NLEX_CASE_KLEENE   256

#define NAN_CHARACTER_LIST(x) ((NanCharacterList *) (x))

/* XXX 'Nan' prefix is used to distinguish internal components from
 * the components that can be used by other programs.
 */

// TODO avoid redefinition
// TODO smaller size
typedef unsigned int NanTreeNodeId;

typedef struct _NanTreeNode {
	NanTreeNodeId         id;

  NlexCharacter         ch;

	/* If ch is NLEX_CASE_ACT, ptr points to the user-given output code.
	 * If ch is NLEX_CASE_LIST, ptr points to the a NanCharacterList.
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

extern NanTreeNodeId id_lastact;
extern NanTreeNodeId id_lastnonact;

/* Print a character to the C source code output with escaping if needed */
static inline void
	nan_character_print_c_comp(NlexCharacter c, const char * id, FILE * fp)
{
	NlexCharacter escin;

	switch(c) {
	case NLEX_CASE_DIGIT:
		fprintf(fp, "isdigit(%s)", id);
		break;
	case NLEX_CASE_EOF:
		fprintf(fp, "%s == EOF", id);
		break;
	case NLEX_CASE_WORDCHAR:
		fprintf(fp, "isalpha(%s) || isdigit(%s) || %s == '_'", id, id, id);
		break;
	default:
		escin = nlex_get_counterpart(c, escout_c, escin_c);

		if(escin == NAN_NOMATCH)
			fprintf(fp, "%s == '%c'", id, c);
		else
			fprintf(fp, "%s == '\\%c'", id, escin);
	}
}

static inline void
	nan_character_list_append(NanCharacterList * ncl, NlexCharacter c)
{
	// TODO use nlex_realloc()?
	ncl->list = realloc(ncl->list, sizeof(NlexCharacter) * (ncl->count + 1));
	if(!ncl->list)
		nlex_die("realloc() error.");

	ncl->list[ncl->count++] = c;
}

static inline NanCharacterList * nan_character_list_new()
{
	// TODO use nlex_malloc()?
	NanCharacterList * ncl = malloc(sizeof(NanCharacterList));
	if(!ncl)
		nlex_die("malloc() error.");
	
	ncl->count = 0;
	ncl->list  = NULL;

	return ncl;
}

static inline NanCharacterList *
	nan_character_list_new_from_character(NlexCharacter c)
{
	NanCharacterList * nclist;

	nclist = nan_character_list_new();

	NAN_CHARACTER_LIST(nclist)->list =
		nlex_malloc(NULL, sizeof(NlexCharacter));

	NAN_CHARACTER_LIST(nclist)->list[0] = c;
	NAN_CHARACTER_LIST(nclist)->count   = 1;
	
	return nclist;
}

/* Character list to Boolean expression */
static inline void
	nan_character_list_to_expr(
		NanCharacterList * ncl, const char * id, FILE * fp)
{
	int i;
	
	if(ncl->count) {
		nan_character_print_c_comp(ncl->list[0], id, fp);
	
		for(i = 1; i < ncl->count; i++) {
			fprintf(fp, " || ");
			nan_character_print_c_comp(ncl->list[i], id, fp);
		}
	}
}

// TODO make non-inline
static inline void nan_tree_node_convert_to_kleene(NanTreeNode * node)
{
	if(node->ch >= 0) { /* Single char */
		/* Convert to list */
		node->ptr = nan_character_list_new_from_character(node->ch);

		node->ch = -NLEX_CASE_LIST;
	}

	node->ch = -(-(node->ch) | NLEX_CASE_KLEENE);
}

static inline NanTreeNodeId nan_tree_node_id(NanTreeNode * node)
{
	/* Make the action node ids odd and others even.
	 * This is for easy identification at runtime.
	 */
	
	if(node->id == 0) {
		if(node->ch == NLEX_CASE_ACT)
			node->id = ((++id_lastact) * 2) + 1;
		else
			node->id = ((++id_lastnonact) * 2);
	}

	return node->id;
}

/* TODO FIXME This comparison is order-sensitive for lists. */
/* TODO what if one node is single character and the other is a single-element list? */
// TODO make non-inline
static inline _Bool
	nan_tree_nodes_match(NanTreeNode * node1, NanTreeNode * node2)
{
	_Bool matches;

	NanCharacterList * nclist1;
	NanCharacterList * nclist2;
	
	_Bool free1; /* Free list1 after comparison */
	_Bool free2;

	/* Four possibilities with two nodes since each can be either a
	 * single character node or list node.
	 * Both being non-lists can be handled directly.
	 * The easy way for others is to convert the single character one(s) to
	 * list node(s) before comparison.
	 */

	/* Case: both are single character nodes */
	if( (node1->ch >= 0 || !(-(node1->ch) & NLEX_CASE_LIST)) &&
		(node2->ch >= 0 || !(-(node2->ch) & NLEX_CASE_LIST)) )
	{
		return (node1->ch == node2->ch);
	}

	/* Other cases; convert to lists (when needed) and then compare. */
	if(node1->ch < 0 && -(node1->ch) & NLEX_CASE_LIST) {
		nclist1 = NAN_CHARACTER_LIST(node1->ptr);
		free1 = 0;
	}
	else {
		nclist1 = nan_character_list_new_from_character(node1->ch);
		free1 = 1;
	}
	
	if(node2->ch < 0 && -(node2->ch) & NLEX_CASE_LIST) {
		nclist2 = NAN_CHARACTER_LIST(node2->ptr);
		free2 = 0;
	}
	else {
		nclist2 = nan_character_list_new_from_character(node2->ch);
		free2 = 1;
	}
	
	matches = 1;

	int i = 0;
	
	while(i < nclist1->count &&	i < nclist2->count) {
		if(nclist1->list[i] != nclist2->list[i]) {
			matches = 0;
			break;
		}
		
		i++;
	}
	
	/* Free the list created by this function. */
	if(free1) {
		free(nclist1->list);
		free(nclist1);
	}
	
	if(free2) {
		free(nclist2->list);
		free(nclist2);
	}
	
	/* XXX For lists not created by this function,
	 * I can free one list and reuse the pointer to the other
	 * in case of a match to save memory.
	 * But it has no practical advantage for a lexer.
	 */
	
	return matches;
}

/* TODO only for debug mode; make non inline */
static inline void nan_tree_dump(NanTreeNode * root, signed int level)
{
	NanTreeNode * tptr;

	int i;
	for(i = 0; i < level; i++)
		fprintf(stderr, "-");

	fprintf(stderr, " ch=%d (%p; id = %d)\n", root->ch, root, nan_tree_node_id(root));

	level++;

	for(tptr = root->first_child; tptr; tptr = tptr->sibling)
		nan_tree_dump(tptr, level);
}
