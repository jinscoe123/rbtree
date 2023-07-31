/** @file points.c
 *
 *  @brief An example program using red-black trees
 *
 *  This file demonstrates how to use the red-black trees defined/implemented in rbtree.h and
 *  rbtree.c.
 *
 *  This is an implementation of a program that allows a user to insert, delete, or search for 3D
 *  points, which are stored in 3 separate red-black trees, each of which is sorted by one of the
 *  point's x-, y-, or z-coordinates. This is a contrived example to demonstrate as much
 *  functionality as possible.
 *
 *  @author Joshua Inscoe (jinscoe123)
 *
 *  @bug No known bugs.
 */


#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "rbtree.h"


#define MIN_OPTION 1    /**< Minimum menu option. */
#define MAX_OPTION 5    /**< Maximum menu option. */


/** @brief Convert a pointer to a signed integer. */
#define IPTR( p )   (( intptr_t)(void*)(p))

/** @brief Convert a pointer to an unsigned integer. */
#define UPTR( p )   ((uintptr_t)(void*)(p))

/** @brief Convert an integer to a pointer. */
#define VPTR( x )   ((void*)(uintptr_t)(x))


/** @brief A 3D point data structure.
 *
 *  This data structure has a red-black node for each of its x-, y-, and z-coordinates, allowing for
 *  efficient searching by any one of its 3 dimensions.
 */
struct point
{
    rbnode_t x_node;    /**< The x-coordinate's red-black node. */
    int x;              /**< The x-coordinate. */
    rbnode_t y_node;    /**< The y-coordinate's red-black node. */
    int y;              /**< The y-coordinate. */
    rbnode_t z_node;    /**< The z-coordinate's red-black node. */
    int z;              /**< The z-coordinate. */
};

/** @brief A 3D point type. */
typedef struct point point_t;


/** @brief A red-black tree of points sorted by their x-coordinates. */
static rbtree_t kTreeX = RBTREE_INIT;

/** @brief A red-black tree of points sorted by their y-coordinates. */
static rbtree_t kTreeY = RBTREE_INIT;

/** @brief A red-black tree of points sorted by their z-coordinates. */
static rbtree_t kTreeZ = RBTREE_INIT;


/** @brief A flag to indicate when to quit. */
static unsigned int volatile kLoop = 1;


/** @brief Signal to the program to quit when the user presses ^C.
 *
 *  @param[in] The signal received (SIGINT).
 */
static void sigint_handler(int sig)
{
    (void) sig;
    kLoop =  0;
}


/** @brief Clear the rest of the current line from stdin.
 *
 *  @return EOF when there is no more data.
 */
static int clear_line(void)
{
    int n = scanf("%*[^\n]");
    if (n != EOF) {
        n = scanf("%*c");
    }
    return n;
}


/** @brief Compare a point's x-coordinate with a given key.
 *
 *  @param[in] node A pointer to the current red-black node.
 *  @param[in] key  A user-specified key.
 *
 *  @return An integer indicating if the node is less-than, equal-to, or greater-than the key.
 */
static int point_compare_x(rbnode_t const* node, void const* key)
{
    return rbnode_data(node, point_t, x_node)->x - IPTR(key);
}

/** @brief Compare a point's y-coordinate with a given key.
 *
 *  @param[in] node A pointer to the current red-black node.
 *  @param[in] key  A user-specified key.
 *
 *  @return An integer indicating if the node is less-than, equal-to, or greater-than the key.
 */
static int point_compare_y(rbnode_t const* node, void const* key)
{
    return rbnode_data(node, point_t, y_node)->y - IPTR(key);
}

/** @brief Compare a point's z-coordinate with a given key.
 *
 *  @param[in] node A pointer to the current red-black node.
 *  @param[in] key  A user-specified key.
 *
 *  @return An integer indicating if the node is less-than, equal-to, or greater-than the key.
 */
static int point_compare_z(rbnode_t const* node, void const* key)
{
    return rbnode_data(node, point_t, z_node)->z - IPTR(key);
}


/** @brief Insert a point into all 3 red-black trees.
 *
 *  @param[in] point The point to insert.
 */
static void insert(point_t* point)
{
    rbtree_insert(&kTreeX, &point->x_node, VPTR(point->x), point_compare_x);
    rbtree_insert(&kTreeY, &point->y_node, VPTR(point->y), point_compare_y);
    rbtree_insert(&kTreeZ, &point->z_node, VPTR(point->z), point_compare_z);
}

/** @brief Delete a point into all 3 red-black trees.
 *
 *  @param[in] point The point to delete.
 */
static void delete(point_t* point)
{
    rbtree_remove(&kTreeX, &point->x_node);
    point->x_node = (rbnode_t) RBNODE_INIT;
    rbtree_remove(&kTreeY, &point->y_node);
    point->y_node = (rbnode_t) RBNODE_INIT;
    rbtree_remove(&kTreeZ, &point->z_node);
    point->z_node = (rbnode_t) RBNODE_INIT;
}


/** @def point_t* to_point(rbnode_t const* node, name)
 *
 *  @brief Convert a red-black node to a point.
 *
 *  @param[in] node A pointer to a red-black node.
 *  @param[in] name The name of the node in the point data structure.
 *
 *  @return A pointer to the point.
 */
#define to_point( node, name ) convert_to_point(node, offsetof(point_t, name))

/** @brief Convert a red-black node to a point.
 *
 *  @param[in] node     A pointer to a red-black node.
 *  @param[in] offset   The offset of the node in the point data structure.
 *
 *  @return A pointer to the point.
 */
static point_t* convert_to_point(rbnode_t const* node, size_t offset)
{
    return node ? (point_t*)(UPTR(node) - offset) : NULL;
}

/** @brief Search for a point by x-coordinate.
 *
 *  If @p start is NULL, then this function will return the first point in the tree with the an
 *  x-coordinate equal to @p x. If @p start is non-NULL, it will return the next point in the tree
 *  after @p start, whose x-coordinate may or may not equal @p x. If a point whose x-coordinate is
 *  not equal to @p x is returned, then the point you are looking for does not exist.
 *
 *  @param[in] x        The x-coordinate.
 *  @param[in] start    The starting point (or NULL).
 *
 *  @return A pointer to a point.
 */
static point_t* search_x(int x, point_t* start)
{
    if (start) {
        return to_point(rbnode_next(&start->x_node), x_node);
    } else {
        return to_point(rbtree_lower_bound(&kTreeX, VPTR(x), point_compare_x), x_node);
    }
}

/** @brief Search for a point by y-coordinate.
 *
 *  If @p start is NULL, then this function will return the first point in the tree with the an
 *  y-coordinate equal to @p y. If @p start is non-NULL, it will return the next point in the tree
 *  after @p start, whose y-coordinate may or may not equal @p y. If a point whose y-coordinate is
 *  not equal to @p y is returned, then the point you are looking for does not exist.
 *
 *  @param[in] y        The y-coordinate.
 *  @param[in] start    The starting point (or NULL).
 *
 *  @return A pointer to a point.
 */
static point_t* search_y(int y, point_t* point)
{
    if (point) {
        return to_point(rbnode_next(&point->y_node), y_node);
    } else {
        return to_point(rbtree_lower_bound(&kTreeY, VPTR(y), point_compare_y), y_node);
    }
}

/** @brief Search for a point by z-coordinate.
 *
 *  If @p start is NULL, then this function will return the first point in the tree with the an
 *  z-coordinate equal to @p z. If @p start is non-NULL, it will return the next point in the tree
 *  after @p start, whose z-coordinate may or may not equal @p z. If a point whose z-coordinate is
 *  not equal to @p z is returned, then the point you are looking for does not exist.
 *
 *  @param[in] z        The z-coordinate.
 *  @param[in] start    The starting point (or NULL).
 *
 *  @return A pointer to a point.
 */
static point_t* search_z(int z, point_t* point)
{
    if (point) {
        return to_point(rbnode_next(&point->z_node), z_node);
    } else {
        return to_point(rbtree_lower_bound(&kTreeZ, VPTR(z), point_compare_z), z_node);
    }
}

/** @brief Search for a point with the given x-, y-, and z-coordinates.
 *
 *  @param[in] x    The x-coordinate.
 *  @param[in] y    The y-coordinate.
 *  @param[in] z    The z-coordinate.
 *
 *  @return A pointer to the found point, or NULL if it does not exist.
 */
static point_t* search(int x, int y, int z)
{
    point_t* xpnt = NULL;
    point_t* ypnt = NULL;
    point_t* zpnt = NULL;

    while (1) {
        xpnt = search_x(x, xpnt);
        if (!xpnt || xpnt->x != x) {
            return NULL;
        }
        if (xpnt->y == y && xpnt->z == z) {
            return xpnt;
        }
        ypnt = search_y(y, ypnt);
        if (!ypnt || ypnt->y != y) {
            return NULL;
        }
        if (ypnt->x == x && ypnt->z == z) {
            return ypnt;
        }
        zpnt = search_z(z, zpnt);
        if (!zpnt || zpnt->z != z) {
            return NULL;
        }
        if (zpnt->x == x && zpnt->y == y) {
            return zpnt;
        }
    }
}


/** @brief A visitor function to free each point in a red-black tree (sorted by x-coordinate).
 *
 *  @param[in] N   A red-black node pointer.
 *  @param[in] arg A pointer to the last visited point.
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
    arg = rbnode_data(N, point_t, x_node);
    return -1;
}

/** @brief Free all fuzz nodes in a given tree.
 *
 *  @param[in] tree A red-black tree pointer.
 */
static void free_all_nodes(void)
{
    /* All points are in each tree, so we just need to free one tree. */
    point_t* root = rbnode_data(kTreeX.root, point_t, x_node);
    rbtree_visit(&kTreeX, free_node_visitor, NULL);
    free(root);
    kTreeX = (rbtree_t) RBTREE_INIT;
    kTreeY = (rbtree_t) RBTREE_INIT;
    kTreeZ = (rbtree_t) RBTREE_INIT;
}


/** @brief Read an integer from stdin.
 *
 *  @param[out] value   The integer value.
 *  @param[in]  s       The name of the parameter to be read.
 *
 *  @return 0 on success, or -1 otherwise.
 */
static int read_(int* value, char const* s)
{
    printf("%s = ", s);
    fflush(stdout);
    int retv = scanf("%d", value);
    if (clear_line() == EOF) {
        return -1;
    }
    if (retv != 1) {
        printf("Bad point value -- %s ∉ [%d, %d]\n", s, INT_MIN, INT_MAX);
        return -1;
    } else {
        return  0;
    }
}

/** @brief Read an x-coordinate from stdin. */
#define read_x( x ) read_(x, "x")

/** @brief Read an y-coordinate from stdin. */
#define read_y( y ) read_(y, "y")

/** @brief Read an z-coordinate from stdin. */
#define read_z( z ) read_(z, "z")

/** @brief Read a point from stdin.
 *
 *  @param[out] x   The point's x-coordinate.
 *  @param[out] y   The point's y-coordinate.
 *  @param[out] z   The point's z-coordinate.
 *
 *  @return 0 on success, or -1 otherwise.
 */
static int read_point(int* x, int* y, int* z)
{
    if (read_x(x) != 0) {
        return -1;
    }
    if (read_y(y) != 0) {
        return -1;
    }
    if (read_z(z) != 0) {
        return -1;
    }
    return 0;
}


/** @brief Read a point from stdin and insert it into each tree. */
static void insert_node(void)
{
    int x = -1;
    int y = -1;
    int z = -1;

    if (read_point(&x, &y, &z) != 0) {
        return;
    }

    point_t* point = malloc(sizeof(*point));
    if (!point) {
        fprintf(stderr, "error: Failed to allocate point\n");
        return;
    }

    *point = (point_t) {
        .x_node = RBNODE_INIT,
        .x = x,
        .y_node = RBNODE_INIT,
        .y = y,
        .z_node = RBNODE_INIT,
        .z = z,
    };

    insert(point);
}

/** @brief Read a point from stdin and delete it from each tree. */
static void remove_node(void)
{
    int x = -1;
    int y = -1;
    int z = -1;

    if (read_point(&x, &y, &z) != 0) {
        return;
    }

    point_t* point = search(x, y, z);
    if (!point) {
        printf("Point not found!\n");
        return;
    }

    delete(point);
}

/** @brief Read a point from stdin and determine if it exists. */
static void search_node(void)
{
    int x = -1;
    int y = -1;
    int z = -1;

    if (read_point(&x, &y, &z) != 0) {
        return;
    }

    point_t* point = search(x, y, z);
    if (!point) {
        printf("Point not found!\n");
    } else {
        printf("Point found!\n");
    }
}


/** @brief Write a point to stdout.
 *
 *  @param[in] point    A pointer to a point.
 */
static void print_point(point_t const* point)
{
    printf("(%d,%d,%d)\n", point->x, point->y, point->z);
}

/** @brief A visitor function to print each point in a red-black tree.
 *
 *  @param[in] N   A red-black node pointer.
 *  @param[in] arg The offset of the node in the point data structure.
 *  @param[in] d   The direction from which the current node was reached (unused).
 *
 *  @return 0 (i.e. continue).
 */
static int print_node_visitor(rbnode_t const* N, void* arg, int d)
{
    (void) d;
    print_point(convert_to_point(N, UPTR(arg)));
    return 0;
}

/** @brief Print all points in order for each tree. */
static void print_trees(void)
{
    size_t offset = 0;
    printf("Tree X\n");
    printf("------\n");
    offset = offsetof(point_t, x_node);
    rbtree_visit_in_order(&kTreeX, print_node_visitor, VPTR(offset));
    printf("\n");
    printf("Tree Y\n");
    printf("------\n");
    offset = offsetof(point_t, y_node);
    rbtree_visit_in_order(&kTreeY, print_node_visitor, VPTR(offset));
    printf("\n");
    printf("Tree Z\n");
    printf("------\n");
    offset = offsetof(point_t, z_node);
    rbtree_visit_in_order(&kTreeZ, print_node_visitor, VPTR(offset));
    printf("\n");
}


/** @brief Display a menu to the user and prompt them to select an option.
 *
 *  @return The selected option.
 */
static int prompt(void)
{
    printf(
        "\n" \
        "--- Menu -----------------------------------------\n" \
        "(1) Insert node\n" \
        "(2) Remove node\n" \
        "(3) Search node\n" \
        "(4) Print trees\n" \
        "(5) Quit\n" \
        "--------------------------------------------------\n" \
        "\n"
    );

    int option = 0;

    while (1) {
        while (!option) {
            printf(">>> ");
            fflush(stdout);
            if (scanf("%d", &option) < 0) {
                option = 5;
            }
            if (clear_line() == EOF) {
                option = 5;
            }
        }

        if (option < MIN_OPTION || MAX_OPTION < option) {
            printf("Invalid option -- %d\n", option);
        } else {
            break;
        }
    }

    return option;
}


int main(void)
{
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        fprintf(stderr, "error: Failed to install signal handler\n");
        return 1;
    }

    /* Loop until the user quits or presses ^C. */
    while (kLoop) {
        switch (prompt()) {
        case  1: insert_node(); break;
        case  2: remove_node(); break;
        case  3: search_node(); break;
        case  4: print_trees(); break;
        case  5: kLoop = 0; break;
        default:
            assert("Unhandled option!" && 0);
            return 1;
        }
    }

    free_all_nodes();

    return 0;
}
