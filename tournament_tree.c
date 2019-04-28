#include "tournament_tree.h"
#include "types.h"
#include "user.h"
#include "kthread.h"

trnmnt_tree *trnmnt_tree_alloc(int depth) {
	if (depth <= 0)
		return 0;

	const int num_of_mutexes = (1 << depth) - 1;
	trnmnt_tree *tree = (trnmnt_tree *) malloc(sizeof(trnmnt_tree));
	tree->children = (int *) malloc(num_of_mutexes * sizeof(int));
	tree->depth = depth;
	tree->waiting = 0;
	for (int i = 0; i < num_of_mutexes; ++i)
		if ((tree->children[i] = kthread_mutex_alloc()) < 0) {
			for (int j = 0; j < i; ++j)
				kthread_mutex_dealloc(tree->children[j]);
			free(tree->children);
			free(tree);
			return 0;
		}
	return tree;
}

int trnmnt_tree_dealloc(trnmnt_tree *tree) {
	if (tree->waiting > 0)
		return -1;
	int ans = 0;
	for (int i = 0; i < (1 << tree->depth) - 1; ++i)
		ans = kthread_mutex_dealloc(tree->children[i]) < 0 || ans < 0 ? -1 : 0;
	free(tree->children);
	free(tree);
	return ans;
}

int trnmnt_tree_acquire(trnmnt_tree *tree, int ID) {
	tree->waiting++;
	//(leaf - 1) / 2 ==father of current node
	for (int i = (((1 << tree->depth) - 1 + ID) - 1) / 2; i >= 0; i = (i - 1) / 2) {
		kthread_mutex_lock(tree->children[i]);
		if (i == 0) //??   //root
			break;
	}
	return 0;
}


int release_rec(struct trnmnt_tree *tree, int _id) {
	if (_id == 0)
		return kthread_mutex_unlock(tree->children[0]);
	release_rec(tree, (_id - 1) / 2);
	return kthread_mutex_unlock(tree->children[_id]);
}


int trnmnt_tree_release(struct trnmnt_tree *tree, int ID) {
	tree->waiting--;
	return release_rec(tree, (((1 << tree->depth) - 1 + ID) - 1) / 2);

}
