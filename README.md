# C Sales Analysis: Processes vs Threads

Analyze sales records using a sequential implementation, POSIX processes, and POSIX threads. Filter by region, country, item type, or sales channel and calculate matching orders, revenue, and profit.

## Implementations

| File | Description |
| --- | --- |
| src/sequential.c | Reads all files sequentially in one process. |
| src/multiprocessing.c | Divides files among child processes using fork and collects totals through pipes. |
| src/multithreading.c | Divides files among pthread workers and aggregates their independent totals. |

## Build and run

Requires Linux or another POSIX system and a C compiler.

```sh
mkdir -p build
gcc -std=c11 -Wall -Wextra -Wpedantic src/sequential.c -o build/sequential
gcc -std=c11 -Wall -Wextra -Wpedantic src/multiprocessing.c -o build/multiprocessing
gcc -std=c11 -Wall -Wextra -Wpedantic -pthread src/multithreading.c -o build/multithreading
./build/sequential
./build/multiprocessing
./build/multithreading
```

Run from the directory containing the twenty input files, named xaa.csv through xat.csv. The dataset is not included.

Each CSV needs a header and at least eleven columns. The parser expects simple unquoted, nonempty fields: region (0), country (1), item type (2), sales channel (3), units sold (8), unit price (9), and unit cost (10). Quoted commas and empty fields are not supported. Filters use exact, case-sensitive matching after trimming surrounding spaces.

Missing files are reported and skipped; totals then cover only readable files. Invalid filter choices are rejected. Worker counts are limited to twenty; nonpositive counts default to two.

## Timing

The sequential version retains the original CPU-time measurement with clock(). The process and thread versions use elapsed wall time with gettimeofday(). These timings measure different quantities and should not be compared directly as speedup figures. Use a common wall-time measurement for a fair benchmark.

## Corrections

Restored truncated declarations, included profit in thread totals, used strtok_r for independent parsing in each worker, initialized records, checked file access and worker creation, and checked complete pipe transfers and child completion.

Compilation and dataset-based execution have not yet been verified.
