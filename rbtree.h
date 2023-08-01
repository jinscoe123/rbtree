/** @file rbtree.h
 *
 *  @brief Intrusive red-black trees
 *
 *  This file contains data types, constants, and functions for creating/manipulating intrusive
 *  red-black trees. To create a data structure that can be inserted into a red-black tree, simply
 *  embed a member of type `rbnode_t` into the structure, like so:
 *
 *  ```
 *  struct demo
 *  {
 *      rbnode_t node;
 *      int      data;
 *  };
 *  ```
 *
 *  The `rbnode_t` member can be named anything, and, given a pointer to a particular `rbnode_t`
 *  member, you can get a pointer to the object in which it is embedded using the rbnode_data()
 *  function.
 *
 *  You can insert an object of type `struct demo` into a red-black tree, as follows:
 *
 *  ```
 *  static int compare(rbnode_t const* node, void const* key)
 *  {
 *      struct demo* demo = rbnode_data(node, struct demo, node);
 *      return demo->data - (int) key;
 *  }
 *
 *  rbtree_t tree = RBTREE_INIT;
 *  struct demo demo = { .node = RBNODE_INIT, .data = 7 };
 *  rbtree_insert(&tree, &demo.node, (void*) demo.data, compare);
 *  ```
 *
 *  Intrusive red-black trees have several advantages over their non-intrusive equivalents,
 *  including:
 *
 *  - Improved data locality due to the red-black node's members being located close by to the
 *  actual data they organize
 *
 *  - Less code-bloat than one could expect from a templated red-black tree implementation
 *
 *  - The flexibility to place data into multiple red-black trees without having to encapsulate one
 *  red-black tree node within another (resulting in extra pointer dereferences)
 *
 *  See points.c for a more detailed example on using these types.
 *
 *  @author Joshua Inscoe (jinscoe123)
 *
 *  @bug No known bugs.
 */


#ifndef RBTREE_H_
#define RBTREE_H_


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/** @brief A red-black node data structure. */
struct rbnode
{
    uintptr_t       parent;     /**< The parent node and this node's color. */
    struct rbnode*  branch[2];  /**< The left/right branch nodes. */
};

/** @brief A red-black tree data structure. */
struct rbtree
{
    struct rbnode*  root;       /**< The root node. */
};

/** @brief A red-black node type. */
typedef struct rbnode rbnode_t;

/** @brief A red-black tree type. */
typedef struct rbtree rbtree_t;


/** @brief A function type for comparing a red-black node and a key.
 *
 *  A function of this type should return a negative integer, zero, or a positive integer to
 *  indicate that the node is less-than, equal-to, or greater-than the given key, respectively.
 *
 *  @param[in] node A pointer to the current red-black node.
 *  @param[in] key  A user-specified key.
 *
 *  @return An integer indicating if the node is less-than, equal-to, or greater-than the key.
 */
typedef int (*rbnode_cmp_f_t)(rbnode_t const*, void const*);

/** @brief A function type for indicating the direction to move next in a red-black tree.
 *
 *  A function of this type should return -1, 0, or +1 to move up the tree (to the parent node), to
 *  the left branch, or to the right branch, respectively. Any other return value will cause tree
 *  traversal to cease immediately, and will be forwarded to the caller as an error code.
 *
 *  @param[in] node A pointer to the current red-black node.
 *  @param[in] arg  A user-specified argument.
 *  @param[in] d    The direction from which we arrived at the current node
 *                  (-1 = from the parent, 0 = from the left branch, 1 = from the right branch).
 *
 *  @return An integer indicating the direction to move next, or an error code.
 */
typedef int (*rbnode_dir_f_t)(rbnode_t const*, void*, int);


/** @brief A static initializer for a red-black node. */
#define RBNODE_INIT { .parent = 0, .branch = { NULL, NULL } }

/** @brief A static initializer for a red-black tree. */
#define RBTREE_INIT { .root = NULL }


/** @def type* rbnode_data(rbnode_t* node, type, name)
 *
 *  @brief Get a pointer to the object containing a given red-black node.
 *
 *  @param[in] node A pointer to a red-black node.
 *  @param[in] type The type of the object containing the red-black node.
 *  @param[in] name The name of the red-black node member field.
 *
 *  @return A pointer to the object of the given type, containing the given red-black node.
 */
#define rbnode_data( node, type, name ) ((type*)((uintptr_t)(node) - offsetof(type, name)))


void rbtree_insert(rbtree_t* tree, rbnode_t* node, void const* key, rbnode_cmp_f_t cmp);

rbnode_t* rbtree_locate(rbtree_t const* tree, void const* key, rbnode_cmp_f_t cmp);

rbnode_t* rbtree_lower_bound(rbtree_t const* tree, void const* key, rbnode_cmp_f_t cmp);

rbnode_t* rbtree_upper_bound(rbtree_t const* tree, void const* key, rbnode_cmp_f_t cmp);

rbnode_t* rbtree_search(rbtree_t const* tree, void const* key, rbnode_cmp_f_t cmp);

void rbtree_remove(rbtree_t* tree, rbnode_t* node);


int rbtree_visit(rbtree_t const* tree, rbnode_dir_f_t dir, void* arg);

int rbtree_visit_in_order(rbtree_t const* tree, rbnode_dir_f_t fun, void* arg);


rbnode_t* rbtree_head(rbtree_t const* tree);

rbnode_t* rbtree_tail(rbtree_t const* tree);

rbnode_t* rbnode_next(rbnode_t const* node);

rbnode_t* rbnode_prev(rbnode_t const* node);


int rbnode_check(rbnode_t const* node);

int rbtree_check(rbtree_t const* tree);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif
