/** @file rbtree.c
 *
 *  @brief Intrusive red-black trees
 *
 *  This file contains the implementations of functions used for creating/manipulating intrusive
 *  red-black trees. The code is this file was adapted from the examples given on the [Wikipedia
 *  page](https://en.wikipedia.org/wiki/Red%E2%80%93black_tree) for red-black trees.
 *
 *  @author Joshua Inscoe (jinscoe123)
 *
 *  @bug No known bugs.
 */


#include "rbtree.h"

#include <stddef.h>
#include <stdint.h>


/** @brief A data structure to check red-black tree properties. */
struct rbnode_check_data
{
    size_t black;   /**< The current path's black-depth. */
    size_t depth;   /**< The expected black-depth of all paths. */
};


#define RBNODE_COLOR_BLK 0  /**< Black node color. */
#define RBNODE_COLOR_RED 1  /**< Red node color. */


/** @brief Convert a pointer to an unsigned integer. */
#define UPTR( p ) ((uintptr_t)(p))

/** @brief Swap the values of two objects. */
#define SWAP( a, b, type ) \
    do {                   \
        type __t = (a);    \
        a = (b);           \
        b = __t;           \
    } while (0)


/** @brief Get the color of a red-black node. */
#define RBCOL( N ) ((N)->parent & UPTR(1))

/** @brief Get the parent node of a red-black node. */
#define RBPAR( N ) ((rbnode_t*)((N)->parent & ~UPTR(1)))

/** @brief Get the direction (left/right) of a red-black node. */
#define RBDIR( N ) (RBPAR(N)->branch[0] != (N))

/** @brief Get the sibling of a red-black node. */
#define RBSIB( N ) (RBPAR(N)->branch[1 - RBDIR(N)])


/** @brief Set the parent of a red-black node (if it is not a nil node). */
#define RBNODE_SET_PARENT( N, P )             \
    do {                                      \
        if ((N)) {                            \
            (N)->parent = UPTR(P) | RBCOL(N); \
        }                                     \
    } while (0)


/** @brief Determine the direction of @p B relative to @p A.
 *
 *  This function returns -1, 0, or +1 if @p A is the parent, left branch, or right-branch of @p B,
 *  respectively. Otherwise, if @p A and @p B are not directly related, then this function will
 *  return -2.
 *
 *  @param[in] A A red-black node pointer (relative from).
 *  @param[in] B A red-black node pointer (relative to).
 *
 *  @return The direction of @p B relative to @p A, or -2 if the two nodes are not directly related.
 */
static int rbfrom(rbnode_t* A, rbnode_t* B)
{
    if (A == RBPAR(B)) {
        return -1;
    }
    if (A == B->branch[0]) {
        return  0;
    }
    if (A == B->branch[1]) {
        return  1;
    }
    return -2;
}


/** @brief Swap two red-black nodes in a given red-black tree.
 *
 *  This function swaps red-black nodes, @p A and @p B, in the red-black tree, @p T.
 *
 *  @param[in] T A red-black tree pointer.
 *  @param[in] A A red-black node pointer.
 *  @param[in] B A red-black node pointer.
 */
static void rbswap(rbtree_t* T, rbnode_t* A, rbnode_t* B)
{
    rbnode_t* P = RBPAR(A);
    rbnode_t* Q = RBPAR(B);

    if (P) {
        P->branch[RBDIR(A)] = B;
    } else {
        T->root = B;
    }
    if (Q) {
        Q->branch[RBDIR(B)] = A;
    } else {
        T->root = A;
    }

    SWAP( A->parent   , B->parent   , uintptr_t );
    SWAP( A->branch[0], B->branch[0], rbnode_t* );
    SWAP( A->branch[1], B->branch[1], rbnode_t* );

    RBNODE_SET_PARENT( A->branch[0], A );
    RBNODE_SET_PARENT( A->branch[1], A );
    RBNODE_SET_PARENT( B->branch[0], B );
    RBNODE_SET_PARENT( B->branch[1], B );
}


/** @brief Perform a tree rotation about a given node.
 *
 *  This function will perform a tree rotation in the direction, given by @p d, about the node,
 *  given by @p P.
 *
 *  @param[in] T A red-black tree pointer.
 *  @param[in] P A red-black node pointer.
 *  @param[in] d The direction of the rotation (0 = left, 1 = right).
 *
 *  @return A pointer to the red-black node that will take the position of @p P.
 */
static rbnode_t* rbrotate(rbtree_t* T, rbnode_t* P, int d)
{
    rbnode_t* G = RBPAR(P);
    rbnode_t* S = P->branch[1 - d];
    rbnode_t* C = S->branch[d - 0];

    P->branch[1 - d] = C;
    S->branch[d - 0] = P;
    if (G) {
        d = RBDIR(P);
        G->branch[d] = S;
    } else {
        T->root = S;
    }
    P->parent = UPTR(S) | RBCOL(P);
    S->parent = UPTR(G) | RBCOL(S);
    if (C) {
        C->parent = UPTR(P) | RBCOL(C);
    }

    return S;
}


/** @brief Insert a node as the child of another node in a red-black tree.
 *
 *  This function inserts @p N as the child of @p P in the red-black tree, specified by @p T. If @p
 *  d is 0, @p N will be inserted as the left child of @p P, and if @p d is 1, it will be inserted
 *  as the right child of @p P. The branch at which @p N will be inserted must already be empty
 *  before insertion.
 *
 *  @param[in] T A red-black tree pointer.
 *  @param[in] N A pointer to the red-black node to be inserted.
 *  @param[in] P A pointer to the parent of the red-black node to be inserted.
 *  @param[in] d The direction of the branch of at which the new node will be inserted
 *               (0 = left branch, 1 = right branch).
 *
 *  @note This is an O(log(n)) operation.
 */
static void rbtree_insert_at_node(rbtree_t* T, rbnode_t* N, rbnode_t* P, int d)
{
    int insert_case = 2;

    rbnode_t* G;
    rbnode_t* U;
    rbnode_t* t;

    N->parent = UPTR(P) | RBNODE_COLOR_RED;
    N->branch[0] = NULL;
    N->branch[1] = NULL;
    if (P) {
        P->branch[d] = N;
    } else {
        T->root = N;
        return;
    }

    while (1) {
        if (!P) {
            insert_case = 3;
            break;
        }
        if (RBCOL(P) == RBNODE_COLOR_BLK) {
            insert_case = 1;
            break;
        }
        G = RBPAR(P);
        if (!G) {
            insert_case = 4;
            break;
        }
        U = RBSIB(P);
        if (!U) {
            insert_case = 5;
            break;
        }
        if (RBCOL(U) == RBNODE_COLOR_BLK) {
            insert_case = 5;
            break;
        }
        P->parent = UPTR(G) | RBNODE_COLOR_BLK;
        U->parent = UPTR(G) | RBNODE_COLOR_BLK;
        N = G;
        P = RBPAR(N);
        N->parent = UPTR(P) | RBNODE_COLOR_RED;
    }

    switch (insert_case) {
    case 1:
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        P->parent = UPTR(G) | RBNODE_COLOR_BLK;
        break;
    case 5:
        d = RBDIR(P);
        if (RBDIR(N) != d) {
            rbrotate(T, P, d);
            t = N;
            N = P;
            P = t;
        }
        /* Fall-through */
    case 6:
        rbrotate(T, G, 1 - d);
        G->parent = UPTR(P) | RBNODE_COLOR_RED;
        G = RBPAR(P);
        P->parent = UPTR(G) | RBNODE_COLOR_BLK;
        break;
    }
}


/** @brief Remove a given node from a red-black tree.
 *
 *  The node, specified by @p N, must be a black leaf in the red-black tree, specified by @p T.
 *
 *  @param[in] T A red-black tree pointer.
 *  @param[in] N A pointer to the red-black node to be removed.
 *
 *  @note This is an O(log(n)) operation.
 */
static void rbtree_remove_black_leaf(rbtree_t* T, rbnode_t* N)
{
    int remove_case = 1;
    int d = RBDIR(N);

    rbnode_t* G;
    rbnode_t* P;
    rbnode_t* S;
    rbnode_t* C;
    rbnode_t* D;

    P = RBPAR(N);
    P->branch[d] = NULL;

    while (1) {
        G = RBPAR(P);
        S = P->branch[1 - d];
        D = S->branch[1 - d];
        C = S->branch[d - 0];
        if (RBCOL(S) == RBNODE_COLOR_RED) {
            remove_case = 3;
            break;
        }
        if (D && RBCOL(D) == RBNODE_COLOR_RED) {
            remove_case = 6;
            break;
        }
        if (C && RBCOL(C) == RBNODE_COLOR_RED) {
            remove_case = 5;
            break;
        }
        if (RBCOL(P) == RBNODE_COLOR_RED) {
            remove_case = 4;
            break;
        }
        S->parent = UPTR(P) | RBNODE_COLOR_RED;
        N = P;
        P = G;
        if (!P) {
            remove_case = 2;
            break;
        }
        d = RBDIR(N);
    }

_rem_cases:
    switch (remove_case) {
    case 1:
        break;
    case 2:
        break;
    case 3:
        rbrotate(T, P, d);
        P->parent = UPTR(S) | RBNODE_COLOR_RED;
        S->parent = UPTR(G) | RBNODE_COLOR_BLK;
        G = S;
        S = C;
        D = S->branch[1 - d];
        C = S->branch[d - 0];
        if (D && RBCOL(D) == RBNODE_COLOR_RED) {
            remove_case = 6;
            goto _rem_cases;
        }
        if (C && RBCOL(C) == RBNODE_COLOR_RED) {
            remove_case = 5;
            goto _rem_cases;
        }
        /* Fall-through */
    case 4:
        S->parent = UPTR(P) | RBNODE_COLOR_RED;
        P->parent = UPTR(G) | RBNODE_COLOR_BLK;
        break;
    case 5:
        rbrotate(T, S, 1 - d);
        S->parent = UPTR(C) | RBNODE_COLOR_RED;
        C->parent = UPTR(P) | RBNODE_COLOR_BLK;
        D = S;
        S = C;
        /* Fall-through */
    case 6:
        rbrotate(T, P, d);
        S->parent = UPTR(G) | RBCOL(P);
        P->parent = UPTR(S) | RBNODE_COLOR_BLK;
        D->parent = UPTR(S) | RBNODE_COLOR_BLK;
        break;
    }
}


/** @brief Insert a node into a red-black tree.
 *
 *  This function inserts @p N into the red-black tree, specified by @p T, at the position,
 *  given by @p key, relative to the comparison function, @p cmp.
 *
 *  @param[in] T   A red-black tree pointer.
 *  @param[in] N   A pointer to the red-black node to be inserted.
 *  @param[in] key The key.
 *  @param[in] cmp A function to compare red-black nodes to a key.
 *
 *  @note This is an O(log(n)) operation.
 */
void rbtree_insert(rbtree_t* T, rbnode_t* N, void const* key, rbnode_cmp_f_t cmp)
{
    rbnode_t* P = rbtree_locate(T, key, cmp);
    if (P && cmp(P, key) < 0) {
        rbtree_insert_at_node(T, N, P, 1);
    } else {
        rbtree_insert_at_node(T, N, P, 0);
    }
}


/** @brief Locate the node at which a node with a given key would be inserted into a red-black tree.
 *
 *  This function acts kind of like a fuzzy search. The node which it finds is guaranteed to be
 *  either the next smallest or next largest in the tree, relative to the given @p key.
 *
 *  @param[in] T   A red-black tree pointer.
 *  @param[in] key The key.
 *  @param[in] cmp A function to compare red-black nodes to a key.
 *
 *  @return A pointer to the found red-black node, or NULL if the @p T is empty.
 *
 *  @note This is an O(log(n)) operation.
 */
rbnode_t* rbtree_locate(rbtree_t const* T, void const* key, rbnode_cmp_f_t cmp)
{
    int d = 0;
    rbnode_t* P = NULL;
    rbnode_t* N = NULL;
    for (N = T->root; N; N = N->branch[d <  0]) {
        d = cmp(N, key);
        P = N;
    }
    return  P;
}


/** @brief Search for the first node not less than a given key in a red-black tree.
 *
 *  @param[in] T   A red-black tree pointer.
 *  @param[in] key The key.
 *  @param[in] cmp A function to compare red-black nodes to a key.
 *
 *  @return A pointer to the found red-black node, or NULL if no matching node was found.
 *
 *  @note This function behaves exactly like the C++ STL's std::lower_bound().
 *
 *  @note This is an O(log(n)) operation.
 */
rbnode_t* rbtree_lower_bound(rbtree_t const* T, void const* key, rbnode_cmp_f_t cmp)
{
    int d = 0;
    rbnode_t* N = NULL;
    rbnode_t* R = NULL;
    for (N = T->root; N; N = N->branch[d <  0]) {
        d = cmp(N, key);
        if (d >= 0) {
            R = N;
        }
        if (d == 0) {
            break;
        }
    }
    return  R;
}


/** @brief Search for the first node greater than a given key in a red-black tree.
 *
 *  @param[in] T   A red-black tree pointer.
 *  @param[in] key The key.
 *  @param[in] cmp A function to compare red-black nodes to a key.
 *
 *  @return A pointer to the found red-black node, or NULL if no matching node was found.
 *
 *  @note This function behaves exactly like the C++ STL's std::lower_bound().
 *
 *  @note This is an O(log(n)) operation.
 */
rbnode_t* rbtree_upper_bound(rbtree_t const* T, void const* key, rbnode_cmp_f_t cmp)
{
    int d = 0;
    rbnode_t* N = NULL;
    rbnode_t* R = NULL;
    for (N = T->root; N; N = N->branch[d <= 0]) {
        d = cmp(N, key);
        if (d >  0) {
            R = N;
        }
        if (d == 0) {
            break;
        }
    }
    return  R;
}


/** @brief Search for a node with a given key in a red-black tree.
 *
 *  @param[in] T   A red-black tree pointer.
 *  @param[in] key The key.
 *  @param[in] cmp A function to compare red-black nodes to a key.
 *
 *  @return A pointer to the found red-black node, or NULL if no matching node was found.
 *
 *  @note This is an O(log(n)) operation.
 */
rbnode_t* rbtree_search(rbtree_t const* T, void const* key, rbnode_cmp_f_t cmp)
{
    int d = 0;
    rbnode_t* N = NULL;
    for (N = T->root; N; N = N->branch[d <  0]) {
        d = cmp(N, key);
        if (d == 0) {
            break;
        }
    }
    return  N;
}


/** @brief Remove a node from a red-black tree.
 *
 *  @param[in] T A red-black tree pointer.
 *  @param[in] N A pointer to the red-black node to be removed.
 *
 *  @note This is an O(log(n)) operation.
 */
void rbtree_remove(rbtree_t* T, rbnode_t* N)
{
    rbnode_t* P;
    rbnode_t* R;

    if (N->branch[0] && N->branch[1]) {
        R = N->branch[0];
        while (R->branch[1]) {
            R = R->branch[1];
        }
        rbswap(T, N, R);
    }

    P = RBPAR(N);
    if (RBCOL(N) == RBNODE_COLOR_RED) {
        R = NULL;
    } else if (N->branch[0]) {
        R = N->branch[0];
    } else if (N->branch[1]) {
        R = N->branch[1];
    } else if (P) {
        rbtree_remove_black_leaf(T, N);
        return;
    } else {
        R = NULL;
    }

    if (P) {
        P->branch[RBDIR(N)] = R;
    } else {
        T->root = R;
    }
    if (R) {
        R->parent = UPTR(P) | RBNODE_COLOR_BLK;
    }
}


/** @brief Walk a red-black tree and execute a given function on each node.
 *
 *  The function, @p dir, will be executed on each node and controls how the tree is traversed. It
 *  should return -1 to move up to the parent node, 0 to move to the left branch, and 1 to move to
 *  the right branch. Any other return value will cause the traversal to cease immediately, and the
 *  value will be returned as an error code.
 *
 *  @param[in] T   A red-black tree pointer.
 *  @param[in] dir The function to execute on each node.
 *  @param[in] arg A user-specified argument to be passed as the second parameter to @p dir.
 *
 *  @return Zero on success traversal, or an error code to indicate the traversal terminated early.
 *
 *  @note The time complexity of this function is determined by @p dir.
 */
int rbtree_visit(rbtree_t const* T, rbnode_dir_f_t dir, void* arg)
{
    rbnode_t* L = NULL;
    rbnode_t* N = T->root;
    while (N) {
        int d = dir(N, arg, rbfrom(L, N));
        L = N;
        switch (d) {
        case -1: N = RBPAR(N);     break;
        case  0: N = N->branch[0]; break;
        case  1: N = N->branch[1]; break;
        default: return d;
        }
    }
    return 0;
}


/** @brief Walk a red-black tree in order and execute a given function on each node.
 *
 *  The function, @p fun, will be executed on each node. It should return 0 to continue onto the
 *  next node. Any other return value will cause the traversal to cease immediately, and the value
 *  will be returned as an error code.
 *
 *  @param[in] T   A red-black tree pointer.
 *  @param[in] fun The function to execute on each node.
 *  @param[in] arg A user-specified argument to be passed as the second parameter to @p fun.
 *
 *  @return Zero on success traversal, or an error code to indicate the traversal terminated early.
 *
 *  @note This is an O(n) operation.
 */
int rbtree_visit_in_order(rbtree_t const* T, rbnode_dir_f_t fun, void* arg)
{
    int err = 0;
    rbnode_t* L = NULL;
    rbnode_t* N = T->root;
    while (N) {
        switch (rbfrom(L, N)) {
        case -1:
            if (N->branch[0]) {
                L = N;
                N = N->branch[0];
                break;
            }
            /* Fall-through */
        case  0:
            err = fun(N, arg, 0);
            if (err) {
                return err;
            }
            if (N->branch[1]) {
                L = N;
                N = N->branch[1];
                break;
            }
            /* Fall-through */
        case  1:
            L = N;
            N = RBPAR(N);
            break;
        }
    }
    return 0;
}


/** @brief Get the head node of a red-black tree.
 *
 *  @param[in] T A red-black tree pointer.
 *
 *  @return A pointer to the head node of the red-black tree, or NULL if the tree is empty.
 */
rbnode_t* rbtree_head(rbtree_t const* T)
{
    rbnode_t* N = T->root;
    if (N) {
        while (N->branch[0]) {
            N = N->branch[0];
        }
    }
    return N;
}

/** @brief Get the tail node in a red-black tree.
 *
 *  @param[in] T A red-black tree pointer.
 *
 *  @return A pointer to the tail node of the red-black tree, or NULL if the tree is empty.
 */
rbnode_t* rbtree_tail(rbtree_t const* T)
{
    rbnode_t* N = T->root;
    if (N) {
        while (N->branch[1]) {
            N = N->branch[1];
        }
    }
    return N;
}


/** @brief Get the next node in a red-black tree.
 *
 *  @param[in] N A red-black node pointer.
 *
 *  @return A pointer to the next red-black node, or NULL if there is no next node.
 */
rbnode_t* rbnode_next(rbnode_t const* N)
{
    rbnode_t const* R = NULL;
    if (N->branch[1]) {
        R = N->branch[1];
        while (R->branch[0]) {
            R = R->branch[0];
        }
    } else {
        while (N) {
            R = RBPAR(N);
            if (RBDIR(N)) {
                N = R;
            } else {
                break;
            }
        }
    }
    return (rbnode_t*) R;
}

/** @brief Get the previous node in a red-black tree.
 *
 *  @param[in] N A red-black node pointer.
 *
 *  @return A pointer to the previous red-black node, or NULL if there is no previous node.
 */
rbnode_t* rbnode_prev(rbnode_t const* N)
{
    rbnode_t const* L = NULL;
    if (N->branch[0]) {
        L = N->branch[0];
        while (L->branch[1]) {
            L = L->branch[1];
        }
    } else {
        while (N) {
            L = RBPAR(N);
            if (RBDIR(N)) {
                break;
            } else {
                N = L;
            }
        }
    }
    return (rbnode_t*) L;
}


/** @brief Check the red-black properties of a given node.
 *
 *  This function does not check that the black heights of its children are the same, since this
 *  would cause this function to not execute in constant time. To check the black heights of all
 *  nodes in a red-black tree, use rbtree_check() instead.
 *
 *  @param[in] N A red-black node pointer.
 *
 *  @return Non-zero if the red-black properties of @p N are maintained, and non-zero otherwise.
 *
 *  @note This is an O(1) operation.
 */
int rbnode_check(rbnode_t const* N)
{
    rbnode_t const* P = RBPAR(N);
    uintptr_t N_color = RBCOL(N);
    return (!P || N == P->branch[RBDIR(N)])
        && (!N->branch[0] || N == RBPAR(N->branch[0]))
        && (!N->branch[1] || N == RBPAR(N->branch[1]))
        && (!P || !(RBCOL(P) == RBNODE_COLOR_RED && N_color == RBNODE_COLOR_RED))
        ;
}


/** @brief A visitor function to check each node in a red-black tree.
 *
 *  @param[in] N   A red-black node pointer.
 *  @param[in] arg A pointer to an object of type `struct rbnode_check_data`, holding information
 *                 on current black height and expected black height.
 *  @param[in] d   The direction from which the current node was reached.
 *
 *  @return The direction to move next, or -2 if it was determined that the tree's red-black
 *          properties have been violated.
 */
static int rbnode_check_visitor(rbnode_t const* N, void* arg, int d)
{
    struct rbnode_check_data* data = (struct rbnode_check_data*) arg;
    switch (d) {
    case -1:
        data->black += (RBCOL(N) == RBNODE_COLOR_BLK);
        if (N->branch[0]) {
            return  0;
        }
        if (data->depth == 0) {
            data->depth  = data->black;
        }
        if (data->black != data->depth) {
            return -2;
        }
        /* Fall-through */
    case  0:
        if (!rbnode_check(N)) {
            return -2;
        }
        if (N->branch[1]) {
            return  1;
        }
        if (data->black != data->depth) {
            return -2;
        }
        /* Fall-through */
    case  1:
        data->black -= (RBCOL(N) == RBNODE_COLOR_BLK);
        return -1;
    }
    return -2;
}


/** @brief Check the red-black properties of a given tree.
 *
 *  @param[in] T A red-black tree pointer.
 *
 *  @return Non-zero if the red-black properties of @p T are maintained, and non-zero otherwise.
 *
 *  @note This is an O(n) operation.
 */
int rbtree_check(rbtree_t const* T)
{
    struct rbnode_check_data data = { 0, 0 };
    int err = rbtree_visit(T, rbnode_check_visitor, &data);
    if (err) {
        return 0;
    } else {
        return 1;
    }
}
