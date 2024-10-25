# Blocked calculation of running statistics

## Overview

When transposing, a common strategy is to perform the transposition in square-sized blocks to be more cache-friendly.
The same approach could be used when computing running statistics in [**tatami_stats**](https://github.com/tatami-inc/tatami_stats), 
to improve performance by reducing cache misses on the result vector.
This would involve loading multiple dimension elements at once, and then iterating over them in cache-friendly blocks when computing the running statistics.

## Results

### `dense_sum`

This computes the dense row sum in a running manner for a column-major matrix.

```console
# On Mac M2:
$ ./build/dense_sum
Testing a 100000 x 2000 matrix

|               ns/op |                op/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|       45,683,500.00 |               21.89 |    2.3% |      0.50 | `naive`
|       57,245,625.00 |               17.47 |    0.1% |      0.63 | `blocked`

# On Intel i7:
$ ./build/dense_sum
Testing a 100000 x 2000 matrix

|               ns/op |                op/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|      112,511,774.00 |                8.89 |    0.6% |      1.27 | `naive`
|      105,723,480.00 |                9.46 |    0.3% |      1.20 | `blocked`
```

Well, that was disappointing.
Perhaps it's not too surprising given that the blocking involves a lot of extra looping overhead.

It gets a bit more interesting if we increase the number of rows, where the blocked method is almost twice as fast on my Intel machine.
Presumably this is because the start of the result vector is no longer in the cache,
increasing the cost of pulling it back in at every running iteration.

```console
# On Mac M2:
$ ./build/dense_sum
Testing a 10000000 x 20 matrix

|               ns/op |                op/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|       80,691,500.00 |               12.39 |    0.4% |      0.89 | `naive`
|       69,514,709.00 |               14.39 |    0.3% |      0.77 | `blocked`

# On Intel i7:
$ ./build/dense_sum -r 10000000 -c 20
Testing a 10000000 x 20 matrix
|               ns/op |                op/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|      302,018,077.00 |                3.31 |    0.3% |      3.36 | `naive`
|      166,904,457.00 |                5.99 |    0.3% |      1.87 | `blocked`
```

I see much weaker improvements on the M2, though.
Maybe the cache is bigger, or the memory bandwidth is higher, or the prefetching is better?
Who knows.

### `dense_var`

Now trying Welford's algorithm for the running variances, which involves two result vectors.
This should increase the frequency of cache misses for the naive calculation, favoring the blocked calculation.

```console
# On Mac M2:
$ ./build/dense_var
Testing a 100000 x 2000 matrix

|               ns/op |                op/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|       76,770,292.00 |               13.03 |    0.1% |      0.85 | `naive`
|      109,256,166.00 |                9.15 |    0.2% |      1.20 | `blocked`

# On Intel i7:
$ ./build/dense_var
Testing a 100000 x 2000 matrix

|               ns/op |                op/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|      171,816,541.00 |                5.82 |    0.8% |      1.92 | `naive`
|      182,211,173.00 |                5.49 |    0.7% |      2.02 | `blocked`
```

Doesn't seem to be the case, unfortunately.
I'm not sure why it does worse than the sum given that the looping overhead is the same.
Maybe it's just a case of increasing the number of rows again, just as described for the sum:

```console
# On Mac M2:
$ ./build/dense_var -r 10000000 -c 20
Testing a 10000000 x 20 matrix

|               ns/op |                op/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|      131,462,958.00 |                7.61 |    0.3% |      1.45 | `naive`
|      138,332,084.00 |                7.23 |    0.4% |      1.52 | `blocked`

# On Intel i7
$ ./build/dense_var -r 10000000 -c 20
Testing a 10000000 x 20 matrix

|               ns/op |                op/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|      537,949,840.00 |                1.86 |    1.4% |      6.07 | `naive`
|      276,380,584.00 |                3.62 |    1.1% |      3.06 | `blocked`
```

That's roughly consistent with the sum results - much better performance on Intel but not much difference (and in fact, a slight pessimisation) on the M2.

### `sparse_sum`

Trying a sparse sum this time:

```console
# On Mac M2:
$ ./build/sparse_sum

Testing a 50000 x 10000 matrix with a density of 0.1

|               ns/op |                op/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|       47,595,875.00 |               21.01 |    0.5% |      0.54 | `naive`
|      299,029,000.00 |                3.34 |    0.1% |      3.31 | `blocked`

# On Intel i7:
$ ./build/sparse_sum
Testing a 50000 x 10000 matrix with a density of 0.1
|               ns/op |                op/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|       91,873,749.00 |               10.88 |    9.8% |      1.00 | `naive`
|      301,647,839.00 |                3.32 |    1.0% |      3.33 | `blocked`
```

At least this one is easy to explain.
For sparse data, the cache misses should be less frequent, as there is less data that could push the result vector out of the cache after processing a dimension element.
On the other hand, the looping overhead is higher relative to the amount of data being processed.
It gets closer as we max out the number of rows, presumably because the cache misses from the vector updates have more effect:

```console
# On Mac M2:
$ ./build/sparse_sum -r 10000000 -c 20
Testing a 10000000 x 20 matrix with a density of 0.1

|               ns/op |                op/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|       54,699,292.00 |               18.28 |    0.2% |      0.61 | `naive`
|      134,057,666.00 |                7.46 |    0.6% |      1.47 | `blocked`

# On Intel i7:
$ ./build/sparse_sum -r 10000000 -c 20
Testing a 10000000 x 20 matrix with a density of 0.1
|               ns/op |                op/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|      192,305,633.00 |                5.20 |    1.9% |      2.13 | `naive`
|      162,791,418.00 |                6.14 |    0.2% |      1.80 | `blocked`
```

## Comments

While there are some benefits to blocking for dense matrices, I think we'll just stick to the simple calculation for now.
Reduced code complexity is worth leaving some performance on the table, especially given the fragility of the improvement across architectures.

In the **tatami** context, another factor to consider is that blocking would require us to allocate enough space to hold a block width's worth of non-target vectors.
This represents increased memory usage regardless of whether it provides a speed benefit.
Indeed, the process of filling the buffer might even evict the first vector from the cache, requiring another trip into main memory to pull it out again.

FWIW **Eigen** also seems to eschew blocking when computing various vector-wise reductions.
Instead, their `colwise()` and `rowwise()` operators just perform the reduction along each vector using the relevant iterators.
This is probably for the sake of sanity, given that they have to handle expression templates as well as matrix data.

## Build instructions

Just use the usual CMake process:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
