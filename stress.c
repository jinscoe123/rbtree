/** @file stress.c
 *
 *  @brief A simple red-black tree benchmark utility
 *
 *  This file implements a simple utility to benchmark operations on red-black trees of various
 *  sizes.
 *
 *  The size of the data stored by each node in the tree can be configured with the STRESS_DATA_LEN
 *  preprocessor macro. This value represent the number of 'size_t' values to store in each node and
 *  will effect the speed of the insert and search operations.
 *
 *  @author Joshua Inscoe (jinscoe123)
 *
 *  @bug No known bugs.
 */


#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rbtree.h"


#ifndef STRESS_DATA_LEN
#define STRESS_DATA_LEN 1
#endif

/** @brief The type of data to store in the red-black tree. */
typedef size_t data_t[STRESS_DATA_LEN];

/** @brief A test data structure. */
struct test
{
    rbnode_t node;  /**< The red-black tree node. */
    data_t   data;  /**< The red-black node data. */
};

/** @brief A test type. */
typedef struct test test_t;


/** @brief Compare a test node's data with a given key.
 *
 *  @param[in] node A pointer to the current red-black node.
 *  @param[in] key  A user-specified key.
 *
 *  @return An integer indicating if the node is less-than, equal-to, or greater-than the key.
 */
static int test_compare(rbnode_t const* node, void const* key)
{
    return memcmp(rbnode_data(node, test_t, node)->data, key, sizeof(data_t));
}


/** @brief Insert a test value into the tree.
 *
 *  @param[in] tree A red-black tree pointer.
 *  @param[in] test A pointer to the test value to insert into the tree.
 */
static void insert(rbtree_t* tree, test_t* test)
{
    rbtree_insert(tree, &test->node, test->data, test_compare);
}

/** @brief Insert a test value into the tree.
 *
 *  @param[in] tree A red-black tree pointer.
 *  @param[in] test A pointer to the test value to delete from the tree.
 */
static void delete(rbtree_t* tree, test_t* test)
{
    rbtree_remove(tree, &test->node);
}

/** @brief Search for a test value in the tree.
 *
 *  Only @p test's data is exmained when searching the tree. Thus, the node returned may or may not
 *  be the same as @test. In fact, @test need not even exist in the tree.
 *
 *  @param[in] tree A red-black tree pointer.
 *  @param[in] test A pointer to the test value to search for.
 */
static test_t* search(rbtree_t* tree, test_t* test)
{
    rbnode_t* node = rbtree_search(tree, test->data, test_compare);
    if (node) {
        return rbnode_data(node, test_t, node);
    } else {
        return NULL;
    }
}


/** @brief Get the processor's current timestamp counter.
 *
 *  @return The current timestamp counter value.
 *
 *  @note This function inserts a load memory fence before reading the timestamp to ensure that all
 *        previous operations have completed before the counter is read.
 */
static inline unsigned long long rdtsc(void)
{
    unsigned int lo;
    unsigned int hi;
    __asm__ volatile ("lfence\n\trdtsc" : "=a" (lo), "=d" (hi));
    return ((unsigned long long) lo) | ((unsigned long long) hi) << 32;
}

/** @brief Get the maximum of two value of type 'unsigned long long'.
 *
 *  @param[in] x The 1st value.
 *  @param[in] y The 2nd value.
 *
 *  @return The larger of the two values.
 */
static inline unsigned long long maxllu(unsigned long long x, unsigned long long y)
{
    return x > y ? x : y;
}

/** @brief Get the integer log base 10 of a value of type 'unsigned long long'.
 *
 *  @param[in] x The value.
 *
 *  @return The log base 10 of @p x.
 */
static inline unsigned long long log10llu(unsigned long long x)
{
    unsigned long long n = 1;
    while (x >= 10) {
        x /= 10;
        n +=  1;
    }
    return n;
}


/** @brief Print usage information. */
static void usage(void)
{
    printf(
        "Usage: ./stress <size>\n"                                                              \
        "\n"                                                                                    \
        "  A simple utility for benchmarking operations on red-black trees of various sizes.\n" \
        "\n"                                                                                    \
        "Arguments:\n"                                                                          \
        "  size  The size of the red-black tree to use for benchmarking.\n"                     \
        );
}


int main(int argc, char* const argv[])
{
    if (argc != 2) {
        usage( );
        return 0;
    }

    /* Get the size of the tree. */
    errno = 0;
    char* endp = NULL;
    size_t N = strtoul(argv[1], &endp, 0);
    if (errno || *endp != '\0') {
        usage( );
        return 0;
    }

    /*
     * Pre-allocate / pre-initialize all nodes beforehand.
     *
     * We are interested in how long it takes to perform red-black tree operations, not how fast
     * malloc() or free() runs.
     */
    test_t* vec = calloc(N, sizeof(*vec));
    if (!vec) {
        fprintf(stderr, "error: calloc() -- %s\n", strerror(errno));
        return 1;
    }
    for (size_t i = 0; i < N; ++i) {
        vec[i].data[STRESS_DATA_LEN - 1] = i;
    }

    rbtree_t tree = RBTREE_INIT;

    test_t* test = NULL;
    test_t  temp;

    unsigned long long tbeg = 0;
    unsigned long long tend = 0;
    unsigned long long tins = 0;
    unsigned long long tdel = 0;
    unsigned long long tsrc = 0;
    unsigned long long tmax = 0;

    /* Create the tree of the requested size. */
    for (size_t i = 0; i < N; ++i) {
        insert(&tree, &vec[i]);
    }

    /* Benchmark all insert, delete, and search operations. */
    for (size_t i = 0; i < N; ++i) {
        temp.data[STRESS_DATA_LEN - 1] = i;
        temp.node = (rbnode_t) RBNODE_INIT;
        test = &temp;

        tbeg = rdtsc();
        insert(&tree, test);
        tend = rdtsc();
        tins += tend - tbeg;
        tbeg = rdtsc();
        delete(&tree, test);
        tend = rdtsc();
        tdel += tend - tbeg;

        tbeg = rdtsc();
        test = search(&tree, test);
        tend = rdtsc();
        tsrc += tend - tbeg;

        tbeg = rdtsc();
        delete(&tree, test);
        tend = rdtsc();
        tdel += tend - tbeg;
        tbeg = rdtsc();
        insert(&tree, test);
        tend = rdtsc();
        tins += tend - tbeg;

        tbeg = rdtsc();
        test = search(&tree, test);
        tend = rdtsc();
        tsrc += tend - tbeg;
    }

    tins /= 2;
    tdel /= 2;
    tsrc /= 2;

    free(vec);

    tmax = maxllu(maxllu(tins, tdel), tsrc);

    /* Compute the maximum width of the total and average results. */
    size_t w0 = log10llu(tmax / 1);
    size_t w1 = log10llu(tmax / N);

    char fmts[128] = { '\0' };

    snprintf(fmts, sizeof(fmts), "XXXXXX (cycles): tot = %%%zullu, avg = %%%zullu\n", w0, w1);

    /* Print the results. */
    memcpy(fmts, "Insert", 6);
    printf(fmts, tins, tins / N);
    memcpy(fmts, "Delete", 6);
    printf(fmts, tdel, tdel / N);
    memcpy(fmts, "Search", 6);
    printf(fmts, tsrc, tsrc / N);

    return 0;
}
