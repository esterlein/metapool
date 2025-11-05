# metapool

![metapool_logo](https://github.com/user-attachments/assets/dbcc7228-de3d-4574-9165-1f5591346d73)

<h4 align="center"><b>
lightweight, cache-friendly pool allocator with compile-time configurable layout
</b></h4>
<br><br>

:hole: Packed memory grid inside L1/L2 cache for simulation workloads

:headstone: Header-only - no external dependencies; include `mtp_memory.hpp` to start

:dna: `std::allocator` adapter for direct use with standard templates

:cyclone: Up to ~1300x faster than `malloc`, ~3.5x faster than heap-free PMR pool 

:nazar_amulet: Allocation trace tools to log and visualize memory usage


![metapool_scheme](https://github.com/user-attachments/assets/480a805b-0429-44e0-abc3-024695dcf0d9)

## :white_square_button: introduction

`metapool` is a lightweight, high-performance memory allocator with compile-time layout configuration and preallocated thread-local arenas, written in C++23 for a game engine.

Unlike general-purpose allocators, it uses a pool-style layout tailored to expected allocation patterns. This repository includes native containers (WIP) and benchmarks against `malloc`, `std::allocator` (`ptmalloc`), and `std::pmr::unsynchronized_pool_resource` backed by `std::pmr::monotonic_buffer_resource` with a thread-local upstream buffer. In these tests, `metapool`’s dynamic array `mtp::vault` reaches **up to 1300x faster** construction and `reserve()` than `std::vector`, and **up to 3.5x faster** than `std::pmr::vector`.


`metapool` implements `std::allocator` and `std::pmr::memory_resource` adapters, making it usable as a backend for standard containers and smart pointers.


## :white_square_button: benchmark

Test system:

* Arch Linux laptop (kernel 6.15.8)
* glibc 2.42 (ptmalloc)
* clang 20.1.8
* AMD Ryzen 7 PRO 8840U
* L1d: 256 KB, L1i: 256 KB, L2: 8 MB, L3: 16 MB
* 64 GB DDR5

Test cases:

* std:    `std::allocator` + `std::vector` (heap)
* pmr:    `pmr::unsynchronized_pool_resource` + `std::pmr::monotonic_buffer_resource` + `pmr::vector` (no heap, throwing upstream)
* mtp:    `metapool` + `mtp::vault` (no heap)
* malloc: raw `ptmalloc` allocation

Optimization flags:

```bash
-O3 -march=native -fstrict-aliasing -flto
```

Build & run:

```bash
./build.sh clean
./build.sh run micro
./build.sh run selective
```

Results:

![benchmark](https://github.com/user-attachments/assets/c7692e54-3de6-4ac2-beed-31a98bbf6b73)

## :white_square_button: quickstart

Standard templates:

```cpp
#include "mtp_memory.hpp"

auto vec = mtp::make_vector<int, mtp::default_set>();
auto ptr = mtp::make_unique<int, mtp::default_set>();
auto str = mtp::make_string<mtp::default_set>("hello");
auto map = mtp::make_unordered_map<int, float, mtp::default_set>();
```

Core allocator API:

```cpp
#include "mtp_memory.hpp"

// raw memory allocation
auto metapool = mtp::get_allocator<mtp::default_set>();
auto* block = metapool.alloc(size, alignment);

// metapool-native construction path (no container, efficient inlining)
auto* obj = metapool.construct<YourType>(42);
metapool.destruct(obj);

// reset all freelists (objects are lost)
metapool.reset();
```

Metaset and native containers (WIP):

```cpp
#include "mtp_memory.hpp"

// custom metaset
using custom_set = mtp::metaset <
    mtp::def<mtp::capf::mul2, 64, 8, 16, 64, 128>
>;

// dynamic array mtp::vault<T>
auto v1 = mtp::make_vault<int, custom_set>();             // no allocation
auto v2 = mtp::make_vault<int, custom_set>(10);           // reserve space
auto v3 = mtp::make_vault<YourType, custom_set>(10, 42);  // construct 10 objects
v3.emplace_back(YourType{});                              // grow and emplace 11th

mtp::vault<int, custom_set> v4;
v4.reserve(10);
```

Container set selection (optional - pass via compiler flags or define before including `mtp_memory.hpp`):

```cpp
#define MTP_CONTAINERS_MTP   // enable mtp containers (experimental)
#define MTP_CONTAINERS_STD   // enable std containers (factory helpers)
#define MTP_CONTAINERS_BOTH  // enable both mtp and std containers
#define MTP_CONTAINERS_NONE  // disable all container headers (default)
```
The `std::allocator` and `std::pmr::memory_resource` adapters are compiled either way, so you can still use `metapool` with standard containers manually.

`metapool` is initialized lazily. To force thread-local initialization:

```cpp
mtp::init<mtp::default_set>();
```

## :white_square_button: architectural overview

Each allocator instance holds a preconfigured *metapool set* - *metaset*. Each metapool manages a range of stride sizes with fast lookup through a proxy array.

A metapool contains multiple pools, each managing blocks of a specific stride, and a fast intrusive freelist backs each pool.
A stride is a size class - the block size (in bytes), always a multiple of the configured stride step.
Each allocation consists of:

* 2 bytes of metadata (header), stored just before the block’s data
* the object’s memory
* optional padding for alignment

![alloc_layout](https://github.com/user-attachments/assets/612d9b2c-50eb-4109-94c2-c4da254b73da)

Allocated objects are aligned to at least the default *alignment quantum* (8 bytes). If stricter alignment is needed, the stride is increased to fit it. Since stride steps are multiples of the alignment quantum, alignment is always resolved during stride selection. There’s no need for per-block alignment logic. Maximum supported alignment is 4096 bytes. `metapool` is SIMD-compatible.

Each allocator uses a freelist proxy array, with one entry per stride. When allocating, the stride index is computed from the size and alignment, and used to access the corresponding proxy. The same index is stored in the 2-byte header for fast deallocation.

`metapool` has no global fallback - arena size and freelist block counts are defined in the metaset at compile time.

If a freelist has no free blocks, allocation steps through the next larger stride until one succeeds. Since proxies are sorted by stride, this fallback is a fast linear scan. If all eligible freelists are exhausted, the allocator fails explicitly.

## :white_square_button: defining metaset

Each pivot is a stride.

Each stride is a multiple of the stride step and is equal to the size of the block.

Each stride step is a power of two, with a minimum of 8 bytes and a maximum of 8 MB.

Metapool entry in a metaset:

```
def <
  capacity_function,
  base_block_count,
  stride_step,
  stride_min (pivot 0),
  strides... (pivots 1...N-1),
  stride_max (pivot N)
>
```

* capacity_function – controls how block count grows between strides
* base_block_count – blocks allocated at the first stride
* stride_step – byte spacing between consecutive strides
* stride_min – starting stride (pivot 0)
* pivot 1...N-1 – intermediate strides where block count changes
* stride_max – final supported stride (pivot N)

![metapool_layout](https://github.com/user-attachments/assets/dfb55fbf-d0bb-45a0-830d-e5327c1068ef)

Each stride pivot divides a metapool’s stride range into segments. The index of a pivot defines both the segment index and the exponent used in the capacity function.

```
[ stride_min (pivot_0), pivot_1, pivot_2, pivot_3, ..., stride_max (pivot_N)]
```

If the capacity function is `mul4`, the base block count is **256**, and the pivot index is used as the exponent, then block counts are computed as:

```
block_count = base_block_count × (4^pivot_index)
```
This produces a sequence like:
```
256 * 4^0, 256 * 4^1, 256 * 4^2, 256 * 4^3, ..., 256 * 4^N
```
Capacity functions allow you to scale block counts across a stride range without needing a separate metapool for every change. Fewer metapools means faster allocation lookup.

Stride ranges across metapools must not overlap, but gaps are allowed. This helps optimize for sparse allocation patterns.
Allocation sizes are rounded up to the nearest supported stride. For example, if the smallest stride is 1024 bytes and you allocate 2 bytes, the allocator will use 2 bytes for the header and waste 1020 bytes per block.


## :white_square_button: example metaset

```cpp
metaset <
  def<capf::mul2, 128,  8,  16,  64, 128, 192>,   // metapool 1 
  def<capf::flat, 256, 16, 256, 512>,             // metapool 2
  def<capf::flat,  64,  8, 576, 576>              // metapool 3
>;
```
Metapool 1:

    Range: 16 to 192 bytes, step 8
    Base block count: 128
    Capacity grows at 64 and 128 using mul2
    Total strides: (192 − 16) / 8 = 22

Metapool 2:

    Range: 256 to 512 bytes, step 16
    Constant block count: 256 (flat)
    Total strides: (512 − 256) / 16 = 16

Metapool 3:

    Single stride: 576 bytes
    Block count: 64
    Step is irrelevant since there's only one stride


## :white_square_button: memory trace

To enable trace instrumentation, define `MTP_ENABLE_TRACE` or pass it to the compiler.

To export trace data:

```cpp
#define MTP_ENABLE_TRACE

/*
...
traced allocations
...
*/

mtp::export_trace("trace/your_traced_system.csv");
```

Then run the script (requires `python` + `matplotlib`):

```python
python plot_trace.py trace/your_traced_system.csv
```

This prints trace data to the terminal and generates a picture:

```bash
 raw_size | proxy |   count | fallbacks | raw_total | stride_total |      peak
------------------------------------------------------------------------------
        8 |     0 | 1904764 |         0 |  15238112 |     30476224 |  14476224
       16 |     0 | 1500000 |         0 |  24000000 |     36000000 |  12000000
       32 |     1 | 3404870 |         0 | 108955840 |    136194800 |  56194800
       64 |     2 | 1500002 |         0 |  96000128 |    108000144 |  36000072
      128 |     3 | 3405190 |         0 | 435864320 |    463105840 | 191105704
 16777216 |    11 |       6 |         0 | 100663296 |    100663344 |  50331672
 33554432 |    11 |       6 |         0 | 201326592 |    201326640 | 100663320
 67108864 |    11 |       4 |         0 | 268435456 |    268435488 | 134217744
134217728 |    12 |       4 |         0 | 536870912 |    536870944 | 268435472
268435456 |    13 |       2 |         0 | 536870912 |    536870928 | 268435464
```

![alloc_trace](https://github.com/user-attachments/assets/01682cdd-5b2b-4329-acd5-40f033f29733)
