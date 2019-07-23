/* Nandakumar Edamana
 * Started on 2019-07-22
 */

typedef struct _NanTreeNode {
	union {
	   int                i;
	   void             * ptr;
	} data;
	struct _NanTreeNode * first_child;
	struct _NanTreeNode * sibling;
} NanTreeNode;
