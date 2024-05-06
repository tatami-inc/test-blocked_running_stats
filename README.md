# Blocked calculation of running statistics

## Overview

When transposing, a common strategy is to perform the transposition in square-sized blocks to be more cache-friendly.
The same approach could be used when computing running statistics in [**tatami_stats**](https://github.com/tatami-inc/tatami_stats), 
to improve performance by reducing cache misses on the result vector.
This would involve loading multiple dimension elements at once, and then iterating over them in cache-friendly blocks when computing the running statistics.

## Results

Well, that was disappointing.
Perhaps it's not too surprising given that the blocking involves a lot of extra looping overhead.

```
$ ./build/summer
|               ns/op |                op/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|        4,334,041.00 |              230.73 |    0.4% |      0.05 | `naive`
|        5,488,333.00 |              182.20 |    2.4% |      0.06 | `blocked`
```

Interestingly, if we set the `MULTIPLE_BUFFERS` macro, we start to see the advantage of blocking.
With enough result vectors (in this case, 3), the cache misses become more expensive than the looping.

```
$ ./build/summer
|               ns/op |                op/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|       10,185,250.00 |               98.18 |    0.4% |      0.11 | `naive`
|        8,963,125.00 |              111.57 |    1.1% |      0.10 | `blocked`
```

Note that the dense case should already favor the blocked analysis.
For the sparse case, the cache misses should be fewer, as there is less data that could push the result vector out of the cache;
and the looping overhead is higher relative to the amount of data being processed.

I think we'll just stick to the simple calculation for now.

## Build instructions

Just use the usual CMake process:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
