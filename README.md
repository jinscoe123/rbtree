# rbtree

This is a fast, space-efficient implementation of an intrusive [red-black
tree](https://en.wikipedia.org/wiki/Red%E2%80%93black_tree). Intrusive red-black trees have several
advantages over their non-intrusive equivalents, including:

- Improved data locality due to the red-black node's members being located close by to the actual
data it organizes
- Less code-bloat than one could expect from a templated red-black tree implementation
- The flexibility to place data into multiple red-black trees without having to encapsulate one
red-black tree node within another (resulting in extra pointer dereferences)

## Build

To use this implementation, just copy [rbtree.h](rbtree.h) and [rbtree.c](rbtree.c) into your
repository and use them as you would any other header and source files. There are no external
dependencies, and it should build perfectly fine with any C/C++ compiler.

The example/test programs can be built with Make (but see [Tests](#tests)).

```
make
```

## Usage

See [points.c](points.c) for a more complete usage example.

Create a type.

```c
struct demo
{
    rbnode_t node;
    int      data;
};
```

Define a comparison function for your type to be used when sorting or searching the red-black tree.

```c
int compare(rbnode_t const* node, void const* key)
{
    /* Get a pointer to the demo object that contains the current red-black node. */
    struct demo* demo = rbnode_data(node, struct demo, node);
    /* Return integer value to indicate position of key relative to current node. */
    return demo->data - (int) key;
}
```

Create a tree.

```c
rbtree_t tree = RBTREE_INIT;
```

Create some instances of your custom type.

```c
size_t const size = 1000000;
struct demo* vec = (struct demo*) malloc(sizeof(*vec) * size);
for (size_t i = 0; i < size; ++i) {
    vec[i] = (struct demo) { .node = RBNODE_INIT, .data = i };
}
```

Insert the instances into the tree.

```c
for (size_t i = 0; i < size; ++i) {
    rbtree_insert(&tree, &vec[i], (void*) vec[i].data, compare);
}
```

Search for a value in the tree.

```c
rbnode_t*    node = rbtree_search(&tree, (void*) 1337, compare);
struct demo* demo = rbnode_data(node, struct demo, node);
printf("%d\n", demo->data);
```

Remove some of the instances from the tree.

```c
rbtree_remove(&tree, &vec[  42].node);
rbtree_remove(&tree, &vec[1234].node);
rbtree_remove(&tree, &vec[1337].node);
rbtree_remove(&tree, &vec[9999].node);
```

## Tests

### Fuzzer

The [fuzzer.c](fuzzer.c) file implements a test harness for fuzzing this implementation of red-black
trees. By default, running `make` compiles the program with your system's default C compiler. In
this case, the program will read in data once per invocation, which is useful for testing individual
inputs. However, when fuzzing we are generally testing many different mutations, which would be slow
if we had to spawn a new process for each input. You can build a faster fuzzer by telling Make to
use one of [AFL](https://github.com/AFLplusplus/AFLplusplus)'s compilers instead.

```
AFL_USE_ASAN=1 CC=afl-clang-lto CFLAGS='-g -O2' make -j$(nproc)
```

There are also two preprocessor macros you might want to define in `CFLAGS` when compiling the
fuzzer. `FUZZER_DATA_TYPE` defines the type of data stored in the red-black tree and which will be
fuzzed. For example, to tell it to fuzz 8-bit integers instead of 32-bit integers (the default), add
`-DFUZZER_DATA_TYPE=int8_t` to the list of `CFLAGS`. Additionally, you can configure how often the
fuzzer checks that the red-black tree properties are maintained with the `FUZZER_VALIDATE`
preprocessor macro. A value of 1, tells it to check once per fuzzed input, while a value of 2 or
greater tells it to check after every insert or delete operation. By default, if you leave it unset,
then the red-black tree properties are not checked, and the fuzzer will only detect memory errors.

### Stress

The [stress.c](stress.c) file implements a simple utility to benchmark insert, delete, and search
operations on red-black trees of various sizes. It currently can only be compiled on x86 platforms
as it relies on the `rdtsc` instruction to measure really small time intervals.

```
$ ./stress
Usage: ./stress <size>

  A simple utility for benchmarking operations on red-black trees of various sizes.

Arguments:
  size  The size of the red-black tree to use for benchmarking.
```

Example results for a tree of 1,000,000 elements.

```
$ ./stress 1000000
Insert (cycles): tot = 411533284, avg = 411
Delete (cycles): tot =  66396386, avg =  66
Search (cycles): tot = 341378991, avg = 341
```

The size of the data stored in each node of the tree can be configured with the `STRESS_DATA_LEN`
preprocessor macro. Its value defines the number of `size_t` values to place in each node, and will
affect the speed of insert and search operations, since they perform comparisons on these values.
Delete operations, on the other hand, perform no node comparisons, and are not significantly
affected by this macro.

**NOTE:** This program does not play well with link-time optimizations (`-flto`), and the results
you see may be misleading if you compile with this option.
