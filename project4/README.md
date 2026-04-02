# Project 4: Data Analysis Toolkit

## Description
A data analysis toolkit in C that processes numerical datasets
using function pointers and callback functions.

## Code Structure and Documentation

### Dispatcher Logic
The program uses a `MenuItem` array where each entry contains
a label and a function pointer (`void (*fn)(Dataset*)`).
The main loop indexes into this array based on user input and
calls `menu[idx].fn(d)` directly — no if/else or switch chain.
Adding a new operation requires only a new row in the array.

### Callback Function Usage
Three callback types are defined:
- `FilterFunc`    — `int (*)(DataType)` — returns 1 if value passes filter
- `TransformFunc` — `DataType (*)(DataType)` — returns transformed value
- `CompareFunc`   — `int (*)(const void*, const void*)` — used by qsort

`apply_filter(d, fn, out)` iterates the dataset and calls the
FilterFunc callback on each element, copying matches to output.

`apply_transform(d, fn)` iterates the dataset and replaces each
element with the result of the TransformFunc callback.

`qsort()` receives compare_asc or compare_desc as a CompareFunc
callback at runtime for ascending or descending sort.

### Memory Management Strategy
- Dataset is heap-allocated at startup via `malloc()`
- `dataset_push()` calls `realloc()` to double capacity when full
- `dataset_reset()` clears size without freeing memory (reuse)
- `dataset_free()` releases the data array
- Temporary copies for median/top-N use `malloc()` + `free()`
- All allocations are NULL-checked before use
- Memory is fully released before program exit

## Compilation
\`\`\`bash
gcc -o data_toolkit data_toolkit.c -lm
\`\`\`

## Usage
\`\`\`bash
./data_toolkit
\`\`\`
