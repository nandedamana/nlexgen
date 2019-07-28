/* Nandakumar Edamana
 * Started on 2019-07-22
 */

typedef struct _NanTreeNode {
	union {
	   signed int         i;   /* Negative values are for special cases */
	   void             * ptr;
	} data;
	struct _NanTreeNode * first_child;
	struct _NanTreeNode * sibling;
} NanTreeNode;
