// Templet of Standard_AVL_Tree

#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int key;
    int height;
    int size;
    int count;
    struct Node *left;
    struct Node *right;
} AVLTree;

static inline int max2(const int a, const int b) { return a > b ? a : b; }
static inline int H(const AVLTree *p) { return p ? p->height : 0; }
static inline int S(const AVLTree *p) { return p ? p->size : 0; }
static inline int BF(const AVLTree *p) { return H(p->left) - H(p->right); }

static inline void pull(AVLTree *p) {
    p->height = max2(H(p->left), H(p->right)) + 1;
    p->size = S(p->left) + S(p->right) + p->count;
}

static AVLTree *create_new_AVLTree_node(int key) {
    AVLTree *p = (AVLTree *)malloc(sizeof(AVLTree));
    p->key = key;
    p->height = 1;
    p->size = 1;
    p->count = 1;
    p->left = NULL;
    p->right = NULL;
    return p;
}

static AVLTree* rotate_left(AVLTree *y) {
    AVLTree *x = y->right;
    AVLTree *T2 = x->left;

    x->left = y;
    y->right = T2;

    pull(y);
    pull(x);
    return x;
}

static AVLTree* rotate_right(AVLTree *y) {
    AVLTree *x = y->left;
    AVLTree *T2 = x->right;

    x->right = y;
    y->left = T2;

    pull(y);
    pull(x);
    return x;
}

static AVLTree* rebalance(AVLTree *root) {
    pull(root);
    const int bf = BF(root);
    if (bf > 1) {
        if (BF(root->left) < 0) {
            root->left = rotate_left(root->left);
        }
        return rotate_right(root);
    }
    if (bf < -1) {
        if (BF(root->right) > 0) {
            root->right = rotate_right(root->right);
        }
        return rotate_left(root);
    }
    return root;
}

static AVLTree* min_node(AVLTree* root) {
    AVLTree *cur = root;
    while (cur != NULL && cur->left != NULL) {
        cur = cur->left;
    }
    return cur;
}

static AVLTree* avl_erase_all(AVLTree *root, int key) {
    if (!root) return NULL;

    if (key < root->key) {
        root->left = avl_erase_all(root->left, key);
    } else if (key > root->key) {
        root->right = avl_erase_all(root->right, key);
    } else {
        if (!root->left || !root->right) {
            AVLTree *child = root->left ? root->left : root->right;
            free(root);
            return child;
        } else {
            AVLTree *s = min_node(root->right);
            root->key = s->key;
            root->count = s->count;
            root->right = avl_erase_all(root->right, s->key);
        }
    }

    return rebalance(root);
}

AVLTree* avl_insert(AVLTree* root, int key) {
    if (!root) return NULL;

    if (key == root->key) {
        root->count++;
        pull(root);
    } else if (key < root->key) {
        root->left = avl_insert(root->left, key);
    } else {
        root->right = avl_insert(root->right, key);
    }
    return rebalance(root);
}

AVLTree* avl_erase(AVLTree* root, int key) {
    if (!root) return NULL;

    if (key < root->key) {
        root->left = avl_erase(root->left, key);
    } else if (key > root->key) {
        root->right = avl_erase(root->right, key);
    } else {
        if (root->count > 1) {
            root->count--;
            pull(root);
            return root;
        } else {
            if (!root->left || !root->right) {
                AVLTree *child = root->left ? root->left : root->right;
                free(root);
                return child;
            } else {
                AVLTree *s = min_node(root->right);
                root->key = s->key;
                root->count = s->count;
                root->right = avl_erase_all(root->right, s->key);
            }
        }
    }

    return rebalance(root);
}

int avl_find(AVLTree* root, int key) {
    while (root) {
        if (key == root->key) {
            return 1;
        }
        root = (key < root->key) ? root->left : root->right;
    }
    return 0;
}

int avl_count(AVLTree* root, int key) {
    while (root) {
        if (key == root->key) {
            return root->count;
        }
        root = (key < root->key) ? root->left : root->right;
    }
    return 0;
}

int avl_rank(AVLTree* root, int key) {
    int res = -1;
    while (root) {
        if (key <= root->key) {
            root = root->left;
        } else {
            res += S(root->left) + root->count;
            root = root->right;
        }
    }
    return res;
}

int avl_Kth_smallest(AVLTree* root, int k, int *out) {
    if (!root || k <= 0 || k > S(root)) return 0;
    while (root) {
        int leftSize = S(root->left);
        if (k <= leftSize) {
            root = root->left;
        } else if (k <= leftSize + root->count) {
            *out = root->key;
            return 1;
        } else {
            k -= leftSize + root->count;
            root = root->right;
        }
    }
    return 0;
}

int avl_predecessor(AVLTree* root, int key, int *out) {
    int found = 0;
    int ans = 0;
    while (root) {
        if (root->key < key) {
            ans = root->key;
            found = 1;
            root = root->right;
        } else {
            root = root->left;
        }
    }
    if (found) *out = ans;
    return ans;
}

int avl_successor(AVLTree* root, int key, int *out) {
    int found = 0;
    int ans = 0;
    while (root) {
        if (root->key > key) {
            ans = root->key;
            found = 1;
            root = root->left;
        } else {
            root = root->right;
        }
    }
    if (found) *out = ans;
    return ans;
}

void avl_free(AVLTree* root) {
    if (!root) return;
    avl_free(root->left);
    avl_free(root->right);
    free(root);
}

int main() {
    AVLTree* root = NULL;

    return 0;
}