/** @file fuzzer.c
 *
 *  @brief A test harness for fuzzing with AFL
 *
 *  This file implements a test harness to perform fuzzing of red-black trees with AFL. This program
 *  can also be built without an AFL compiler, but it will only be able to fuzz a single input per
 *  program invocation.
 *
 *  To compile this program, it is recommended to run `make` like so:
 *
 *  ```
 *  CC=afl-clang-lto CFLAGS='-g -O2 -march=native -DFUZZER_DATA_TYPE=char -DFUZZER_VALIDATE=1' make
 *  ```
 *
 *  @author Joshua Inscoe (jinscoe123)
 *
 *  @bug No known bugs.
 */


#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "rbtree.h"


/*
 * Allow AFL fuzzing to work even if persistent mode is not supported. Persistent fuzzing allows us
 * to go A LOT faster, so compile with afl-clang-fast or afl-clang-lto, if possible.
 */
#ifndef __AFL_FUZZ_TESTCASE_LEN
#define __AFL_FUZZ_TESTCASE_LEN kFuzzLen
#define __AFL_FUZZ_TESTCASE_BUF kFuzzBuf
#define __AFL_FUZZ_INIT() void sync(void)
#define __AFL_LOOP(x) ((kFuzzLen = read(0, kFuzzBuf, sizeof(kFuzzBuf))) > 0 ? 1 : 0)
#define __AFL_INIT() sync()
#endif

#ifdef __AFL_HAVE_MANUAL_CONTROL
__AFL_FUZZ_INIT();
#endif


/** @brief Convert a pointer to a signed integer. */
#define IPTR( p )   (( intptr_t)(void*)(p))

/** @brief Convert a pointer to an unsigned integer. */
#define UPTR( p )   ((uintptr_t)(void*)(p))

/** @brief Convert an integer to a pointer. */
#define VPTR( x )   ((void*)(uintptr_t)(x))


#ifndef FUZZER_DATA_TYPE
#define FUZZER_DATA_TYPE int32_t
#endif


/** @brief The data type to fuzz. */
typedef FUZZER_DATA_TYPE data_t;

/** @brief The data structure to be fuzzed. */
struct fuzz
{
    rbnode_t node;  /**< The red-black tree node. */
    data_t   data;  /**< The red-black node data. */
};


/** @brief The type to be fuzzed. */
typedef struct fuzz fuzz_t;


/** @brief A buffer to contain the fuzzing data.
 *
 *  @note This is only used when persistent fuzzing is not used or supported.
 */
static data_t kFuzzBuf[16 * 1024];

/** @brief The length of the fuzzing data.
 *
 *  @note This is only used when persistent fuzzing is not used or supported.
 */
static ssize_t kFuzzLen = sizeof(kFuzzBuf);


/** @brief Compare a fuzz node's data with a given key.
 *
 *  @param[in] node A pointer to the current red-black node.
 *  @param[in] key  A user-specified key.
 *
 *  @return An integer indicating if the node is less-than, equal-to, or greater-than the key.
 */
static int fuzz_compare(rbnode_t const* node, void const* key)
{
    return rbnode_data(node, fuzz_t, node)->data - IPTR(key);
}


/** @brief A visitor function to free each fuzz node in a red-black tree.
 *
 *  @param[in] N   A red-black node pointer.
 *  @param[in] arg A pointer to the last visited fuzz node.
 *  @param[in] d   The direction from which the current node was reached.
 *
 *  @return The direction to move next.
 */
static int free_node_visitor(rbnode_t const* N, void* arg, int d)
{
    switch (d) {
    case -1:
        if (N->branch[0]) { return 0; }
        if (N->branch[1]) { return 1; }
        break;
    case  0:
        free(arg);
        if (N->branch[1]) { return 1; }
        break;
    case  1:
        free(arg);
        break;
    }
    arg = rbnode_data(N, fuzz_t, node);
    return -1;
}

/** @brief Free all fuzz nodes in a given tree.
 *
 *  @param[in] tree A red-black tree pointer.
 */
static void reset(rbtree_t* tree)
{
    fuzz_t* root = rbnode_data(tree->root, fuzz_t, node);
    rbtree_visit(tree, free_node_visitor, NULL);
    free(root);
    *tree = (rbtree_t) RBTREE_INIT;
}


/** @brief Insert a fuzz value into the tree.
 *
 *  @param[in] tree A red-black tree pointer.
 *  @param[in] data The data to insert into the tree.
 *
 *  @note If memory allocation fails, this function will abort.
 */
static void insert(rbtree_t* tree, int data)
{
    fuzz_t* fuzz = malloc(sizeof(*fuzz));
    if (!fuzz) {
        assert("Allocation failed" && 0);
        __builtin_unreachable();
        return;
    }
    *fuzz = (fuzz_t) {
        .node = RBNODE_INIT, .data = data
    };
    rbtree_insert(tree, &fuzz->node, VPTR(data), fuzz_compare);
}

/** @brief Delete a fuzz value from the tree.
 *
 *  @param[in] tree A red-black tree pointer.
 *  @param[in] data The data to delete from the tree.
 */
static void delete(rbtree_t* tree, int data)
{
    rbnode_t* node = rbtree_search(tree, VPTR(data), fuzz_compare);
    if (node) {
        rbtree_remove(tree, node);
    }
}

/** @brief Update the tree with the given fuzz data.
 *
 *  For each fuzz data point, if the LSB is 1, then the value is inserted into the tree.
 *  Otherwise, the value will be deleted from the tree (if it exists).
 *
 *  The FUZZER_VALIDATE preprocessor macro can be used to control how often this function checks
 *  that the red-black properities of the tree are maintained. If undefined or set to 0, then no
 *  checks will be performed. If set to 1, then a single check will be performed after each
 *  invocation of this function. If set to 2 or greater, then a check will be performed after each
 *  insert or delete operation (i.e. @p len times).
 *
 *  @param[in] tree A red-black tree pointer.
 *  @param[in] buf  The fuzz buffer.
 *  @param[in] len  The number of elements in the fuzz buffer.
 *
 *  @note If the tree is found to be corrupted, this function will abort.
 */
static void update(rbtree_t* tree, data_t* buf, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        if (buf[i] & 1) {
            insert(tree, buf[i] & ~1);
        } else {
            delete(tree, buf[i] & ~1);
        }
#if defined(FUZZER_VALIDATE) && FUZZER_VALIDATE >= 2
    assert(rbtree_check(tree) && "Tree corrupted!");
#endif
    }
#if defined(FUZZER_VALIDATE) && FUZZER_VALIDATE == 1
    assert(rbtree_check(tree) && "Tree corrupted!");
#endif
}


int main(void)
{
    rbtree_t tree = RBTREE_INIT;

    data_t* buf = kFuzzBuf;
    ssize_t len = kFuzzLen;

#ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();
#endif

#ifdef __AFL_HAVE_MANUAL_CONTROL
    buf = (data_t*) __AFL_FUZZ_TESTCASE_BUF;
    while (__AFL_LOOP(1000)) {
    len = (ssize_t) __AFL_FUZZ_TESTCASE_LEN;
    if ((len /= sizeof(*buf)) <= 0) {
        continue;
    }
#else
    /* Fuzz with a single input if compiled with a non-AFL compiler. */
    len = read(0, buf, len);
    if ((len /= sizeof(*buf)) <= 0) {
        return 1;
    }
#endif

    update(&tree, buf, len);

    reset(&tree);

#ifdef __AFL_HAVE_MANUAL_CONTROL
    } /* while (__AFL_LOOP(...) */
#endif

    return 0;
}
