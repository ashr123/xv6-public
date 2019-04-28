#include "types.h"

typedef struct trnmnt_tree {
	int *children, waiting;
	uint depth;
} trnmnt_tree;

trnmnt_tree *trnmnt_tree_alloc(int depth);

int trnmnt_tree_dealloc(trnmnt_tree *tree);

int trnmnt_tree_acquire(trnmnt_tree *tree, int ID);

int trnmnt_tree_release(trnmnt_tree *tree, int ID);

