# Project 1: Smart Traffic Light Controller

## Project Description
A smart traffic light controller that manages traffic flow at two intersections
using Arduino (simulated in Tinkercad) and a PCB design in EasyEDA.

## Features
- Detects vehicle presence using push buttons
- Automatically adjusts traffic light timing based on traffic conditions
- Manages traffic lights using Red, Yellow, and Green LEDs
- Stores and displays vehicle counts and signal states via serial output
- Simulates concurrent operations using non-blocking programming (millis())
- Provides a serial interface menu for monitoring and manual overrides
- PCB schematic and layout designed in EasyEDA

## Requirements Met
1. Two traffic intersections simulated using LEDs (Intersection A and B)
2. Structures, pointers, and dynamic memory (malloc/free) used to manage traffic data
3. Non-blocking concurrency implemented using millis() instead of delay()
4. Traffic information logged through serial output on every state change
5. PCB schematic and layout designed in EasyEDA
6. Error handling implemented via isValidState() to prevent invalid signal states
7. Code is modular with clearly named functions and inline comments

## Pin Layout
| Component     | Intersection A | Intersection B |
|---------------|----------------|----------------|
| Red LED       | Pin 2          | Pin 8          |
| Yellow LED    | Pin 3          | Pin 9          |
| Green LED     | Pin 4          | Pin 10         |
| Push Button   | Pin 7          | Pin 11         |

## Serial Commands
| Key | Action                          |
|-----|---------------------------------|
| 1   | View full system status         |
| 2   | Toggle manual override A        |
| 3   | Toggle manual override B        |
| 4   | Force GREEN on Intersection A   |
| 5   | Force GREEN on Intersection B   |
| 6   | Reset all vehicle counts        |
| 7   | Show command menu               |

## Timing
| Phase       | Duration        |
|-------------|-----------------|
| Green       | 5s (up to 10s with vehicles) |
| Yellow      | 2s              |
| Green extension per vehicle | +2s |

## Files
- `traffic_light_controller.ino` - Main Arduino source code

## Simulation
Tinkercad simulation link: https://www.tinkercad.com/things/gWnlGV59JuB/editel?returnTo=%2Fdashboard

## PCB Design
EasyEDA project link: https://easyeda.com/editor#id=|1963dc70158c4adb84c98f25cdef9e51|f191d3afe65449969872d868b7ba2646

## How to Run
1. Open Tinkercad simulation link
2. Click Start Simulation
3. Open Serial Monitor at 9600 baud
4. Press buttons to simulate vehicle detection
5. Type commands 1-7 in Serial Monitor to interact with the system

## Concepts Demonstrated
- C structs and typedef
- Dynamic memory allocation with malloc() and free()
- Pointers and pointer arithmetic
- Non-blocking programming with millis()
- State machines
- Serial communication
- Basic error handling


# Project 4: Data Analysis Toolkit Using Function Pointers and Callbacks

## Description
A data analysis toolkit implemented in C that processes numerical datasets
using function pointers and callback functions. Operations are dynamically
dispatched at runtime through a structured function pointer table, avoiding
long conditional chains.

## Code Structure and Documentation

### Dispatcher Logic
The program uses a `MenuItem` array where each entry contains:
- A label string shown in the menu
- A function pointer `void (*fn)(Dataset*)` pointing to the operation
```c
MenuItem menu[] = {
    { "Add values to dataset",   op_add_values  },
    { "Compute sum and average", op_sum_avg     },
    { "Sort ascending",          op_sort_asc    },
    ...
};
```

The dispatcher loop calls `menu[idx].fn(d)` directly based on user input.
No if/else or switch chain is used. Adding a new operation requires only
a new row in the array.

### Callback Function Usage
Three callback types are defined as typedefs:

| Type | Signature | Purpose |
|------|-----------|---------|
| `FilterFunc` | `int (*)(DataType)` | Returns 1 if value passes filter |
| `TransformFunc` | `DataType (*)(DataType)` | Returns transformed value |
| `CompareFunc` | `int (*)(const void*, const void*)` | Used by qsort |

**Filter callbacks:**
- `filter_above_threshold` — keeps values above user-defined threshold
- `filter_below_threshold` — keeps values below user-defined threshold
- `filter_positive` — keeps only positive values
- `filter_negative` — keeps only negative values

**Transform callbacks:**
- `transform_square` — squares each value
- `transform_sqrt` — square root of each value
- `transform_scale` — multiplies by user-defined factor
- `transform_negate` — negates each value
- `transform_normalize` — maps all values to [0,1] range

**Comparison callbacks (passed to qsort):**
- `compare_asc` — ascending order
- `compare_desc` — descending order

`apply_filter(d, fn, out)` iterates the dataset and calls the
FilterFunc callback on each element, copying matches to output dataset.

`apply_transform(d, fn)` iterates the dataset and replaces each
element in-place with the result of the TransformFunc callback.

`qsort()` receives `compare_asc` or `compare_desc` as a function
pointer at runtime for flexible sorting direction.

### Memory Management Strategy
- Dataset is heap-allocated at startup via `malloc()`
- `dataset_push()` calls `realloc()` to double capacity when full
- Capacity doubling avoids repeated small allocations (amortized O(1))
- `dataset_reset()` clears size without freeing memory for reuse
- `dataset_free()` releases the internal data array
- Temporary copies for median and statistics use `malloc()` + `free()`
- All allocations are NULL-checked before use
- All heap memory is released before program exit
- NULL function pointer guard via `check_null_fn()` prevents crashes

## Supported Operations
1. Add values to dataset
2. Display dataset
3. Compute sum and average
4. Find minimum and maximum
5. Statistical summary (mean, median, std dev, variance, range)
6. Sort ascending
7. Sort descending
8. Search for a value
9. Filter dataset (4 filter callbacks)
10. Apply transformation (5 transform callbacks)
11. Load dataset from file
12. Save dataset to file
13. Reset dataset

## Files
- `data_toolkit.c` — Full source code
- `dataset.txt` — Auto-saved dataset on exit

## Compilation
```bash
gcc -o data_toolkit data_toolkit.c -lm
```

## Usage
```bash
./data_toolkit
```

## Key C Concepts Demonstrated
- Function pointers and typedef'd callback signatures
- Struct-based function pointer dispatch table
- Dynamic memory with malloc/realloc/free
- Callback-based generic algorithms (apply_filter, apply_transform)
- qsort with custom comparator callbacks
- File I/O with error handling
- Input validation on all user entries
