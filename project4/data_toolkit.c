/*
 * ============================================================
 *  Data Analysis Toolkit v1.0
 *  Uses function pointers, callbacks, and dynamic memory
 * ============================================================
 *  Architecture:
 *    - Dispatcher: array of function pointers maps menu → op
 *    - Callbacks:  filter, transform, compare passed at runtime
 *    - Dataset:    heap-allocated, grows with realloc()
 *    - File I/O:   load/save as text (one value per line)
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

/* ============================================================
 *  TYPES
 * ============================================================ */
typedef double DataType;

/* Callback signatures */
typedef int    (*CompareFunc)(const void*, const void*);
typedef int    (*FilterFunc)(DataType);
typedef DataType (*TransformFunc)(DataType);

/* ============================================================
 *  DATASET
 * ============================================================ */
typedef struct {
    DataType* data;
    int       size;
    int       capacity;
} Dataset;

/* ============================================================
 *  DISPATCHER ENTRY
 *  Each menu item maps to a function pointer
 * ============================================================ */
typedef struct {
    const char* label;
    void (*fn)(Dataset*);
} MenuItem;

/* ============================================================
 *  GLOBALS
 * ============================================================ */
Dataset* ds = NULL;

/* ============================================================
 *  FUNCTION PROTOTYPES
 * ============================================================ */
/* Dataset management */
Dataset* dataset_create(int initial_cap);
int      dataset_push(Dataset* d, DataType val);
void     dataset_free(Dataset* d);
void     dataset_reset(Dataset* d);

/* Operations (all take Dataset* — compatible with dispatcher) */
void op_display(Dataset* d);
void op_sum_avg(Dataset* d);
void op_min_max(Dataset* d);
void op_sort_asc(Dataset* d);
void op_sort_desc(Dataset* d);
void op_search(Dataset* d);
void op_filter(Dataset* d);
void op_transform(Dataset* d);
void op_statistics(Dataset* d);
void op_load_file(Dataset* d);
void op_save_file(Dataset* d);
void op_add_values(Dataset* d);
void op_reset(Dataset* d);

/* Callbacks — filters */
int filter_above_threshold(DataType val);
int filter_below_threshold(DataType val);
int filter_positive(DataType val);
int filter_negative(DataType val);

/* Callbacks — transforms */
DataType transform_square(DataType val);
DataType transform_sqrt(DataType val);
DataType transform_scale(DataType val);
DataType transform_normalize(DataType val);
DataType transform_negate(DataType val);

/* Comparison callbacks for qsort */
int compare_asc(const void* a, const void* b);
int compare_desc(const void* a, const void* b);

/* Generic apply functions */
void apply_filter(Dataset* d, FilterFunc fn, Dataset* out);
void apply_transform(Dataset* d, TransformFunc fn);

/* Helpers */
int   read_int(const char* prompt, int min, int max);
DataType read_double(const char* prompt);
void  print_separator(void);
void  check_null_fn(void* fn, const char* name);

/* Dispatcher */
void run_dispatcher(Dataset* d);

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(void) {
    ds = dataset_create(8);
    if (!ds) {
        fprintf(stderr, "[FATAL] Could not allocate dataset.\n");
        return 1;
    }

    printf("\n");
    print_separator();
    printf("   DATA ANALYSIS TOOLKIT v1.0\n");
    printf("   Function Pointers & Callbacks in C\n");
    print_separator();

    run_dispatcher(ds);

    dataset_free(ds);
    free(ds);
    return 0;
}

/* ============================================================
 *  DATASET MANAGEMENT
 * ============================================================ */
Dataset* dataset_create(int initial_cap) {
    Dataset* d = (Dataset*) malloc(sizeof(Dataset));
    if (!d) return NULL;

    d->data = (DataType*) malloc(initial_cap * sizeof(DataType));
    if (!d->data) { free(d); return NULL; }

    d->size     = 0;
    d->capacity = initial_cap;
    return d;
}

/*
 * dataset_push: appends a value, doubling capacity via realloc()
 * when the array is full.
 */
int dataset_push(Dataset* d, DataType val) {
    if (d->size >= d->capacity) {
        int new_cap = d->capacity * 2;
        DataType* tmp = (DataType*) realloc(d->data, new_cap * sizeof(DataType));
        if (!tmp) {
            printf("[ERROR] realloc failed — dataset not expanded.\n");
            return 0;
        }
        d->data     = tmp;
        d->capacity = new_cap;
    }
    d->data[d->size++] = val;
    return 1;
}

void dataset_free(Dataset* d) {
    if (d && d->data) {
        free(d->data);
        d->data     = NULL;
        d->size     = 0;
        d->capacity = 0;
    }
}

void dataset_reset(Dataset* d) {
    d->size = 0;
    printf("[OK] Dataset cleared (memory retained).\n");
}

/* ============================================================
 *  DISPATCHER
 *  Array of MenuItem structs — each holds a label and fn ptr.
 *  The main loop indexes into this array; no if/else chain.
 * ============================================================ */
void run_dispatcher(Dataset* d) {
    /*
     * Dispatcher table: maps menu index → function pointer.
     * Adding a new operation only requires a new row here.
     */
    MenuItem menu[] = {
        { "Add values to dataset",          op_add_values  },
        { "Display dataset",                op_display     },
        { "Compute sum and average",        op_sum_avg     },
        { "Find minimum and maximum",       op_min_max     },
        { "Statistical summary",            op_statistics  },
        { "Sort ascending",                 op_sort_asc    },
        { "Sort descending",                op_sort_desc   },
        { "Search for a value",             op_search      },
        { "Filter dataset",                 op_filter      },
        { "Apply transformation",           op_transform   },
        { "Load dataset from file",         op_load_file   },
        { "Save dataset to file",           op_save_file   },
        { "Reset dataset",                  op_reset       },
    };

    int num_items = (int)(sizeof(menu) / sizeof(menu[0]));

    while (1) {
        printf("\n");
        print_separator();
        printf("  MENU  (Dataset size: %d)\n", d->size);
        print_separator();

        for (int i = 0; i < num_items; i++) {
            printf("  %2d. %s\n", i + 1, menu[i].label);
        }
        printf("  %2d. Exit\n", num_items + 1);
        print_separator();

        int choice = read_int("  Choice: ", 1, num_items + 1);

        if (choice == num_items + 1) {
            op_save_file(d);   /* Auto-save on exit */
            printf("\n  Goodbye!\n\n");
            return;
        }

        /* Dispatch: call the function pointer for the chosen item */
        int idx = choice - 1;

        /* NULL guard — should never fire, but demonstrates safety */
        check_null_fn((void*)menu[idx].fn, menu[idx].label);
        if (menu[idx].fn) {
            menu[idx].fn(d);
        }
    }
}

/* ============================================================
 *  OPERATIONS
 * ============================================================ */
void op_display(Dataset* d) {
    if (d->size == 0) { printf("  Dataset is empty.\n"); return; }
    printf("\n  Dataset (%d values):\n  [ ", d->size);
    for (int i = 0; i < d->size; i++) {
        printf("%.4g", d->data[i]);
        if (i < d->size - 1) printf(", ");
        if ((i + 1) % 10 == 0 && i < d->size - 1) printf("\n    ");
    }
    printf(" ]\n");
}

void op_sum_avg(Dataset* d) {
    if (d->size == 0) { printf("  Dataset is empty.\n"); return; }
    double sum = 0.0;
    for (int i = 0; i < d->size; i++) sum += d->data[i];
    printf("  Sum:     %.6g\n", sum);
    printf("  Average: %.6g\n", sum / d->size);
}

void op_min_max(Dataset* d) {
    if (d->size == 0) { printf("  Dataset is empty.\n"); return; }
    DataType mn = d->data[0], mx = d->data[0];
    int mn_i = 0, mx_i = 0;
    for (int i = 1; i < d->size; i++) {
        if (d->data[i] < mn) { mn = d->data[i]; mn_i = i; }
        if (d->data[i] > mx) { mx = d->data[i]; mx_i = i; }
    }
    printf("  Minimum: %.6g  (index %d)\n", mn, mn_i);
    printf("  Maximum: %.6g  (index %d)\n", mx, mx_i);
}

void op_statistics(Dataset* d) {
    if (d->size == 0) { printf("  Dataset is empty.\n"); return; }

    /* Mean */
    double sum = 0.0;
    DataType mn = d->data[0], mx = d->data[0];
    for (int i = 0; i < d->size; i++) {
        sum += d->data[i];
        if (d->data[i] < mn) mn = d->data[i];
        if (d->data[i] > mx) mx = d->data[i];
    }
    double mean = sum / d->size;

    /* Variance and std deviation */
    double var = 0.0;
    for (int i = 0; i < d->size; i++) {
        double diff = d->data[i] - mean;
        var += diff * diff;
    }
    var /= d->size;
    double std = sqrt(var);

    /* Median — sort a copy */
    DataType* tmp = (DataType*) malloc(d->size * sizeof(DataType));
    if (!tmp) { printf("[ERROR] Memory error.\n"); return; }
    memcpy(tmp, d->data, d->size * sizeof(DataType));
    qsort(tmp, d->size, sizeof(DataType), compare_asc);

    double median;
    if (d->size % 2 == 0)
        median = (tmp[d->size / 2 - 1] + tmp[d->size / 2]) / 2.0;
    else
        median = tmp[d->size / 2];
    free(tmp);

    printf("\n  Statistical Summary:\n");
    printf("  %-18s %g\n",  "Count:",     (double)d->size);
    printf("  %-18s %.6g\n","Sum:",        sum);
    printf("  %-18s %.6g\n","Mean:",       mean);
    printf("  %-18s %.6g\n","Median:",     median);
    printf("  %-18s %.6g\n","Min:",        (double)mn);
    printf("  %-18s %.6g\n","Max:",        (double)mx);
    printf("  %-18s %.6g\n","Range:",      (double)(mx - mn));
    printf("  %-18s %.6g\n","Variance:",   var);
    printf("  %-18s %.6g\n","Std Dev:",    std);
}

void op_sort_asc(Dataset* d) {
    if (d->size == 0) { printf("  Dataset is empty.\n"); return; }
    /*
     * qsort receives a comparison callback — compare_asc is the
     * function pointer passed at runtime.
     */
    qsort(d->data, d->size, sizeof(DataType), compare_asc);
    printf("  [OK] Sorted ascending.\n");
    op_display(d);
}

void op_sort_desc(Dataset* d) {
    if (d->size == 0) { printf("  Dataset is empty.\n"); return; }
    qsort(d->data, d->size, sizeof(DataType), compare_desc);
    printf("  [OK] Sorted descending.\n");
    op_display(d);
}

void op_search(Dataset* d) {
    if (d->size == 0) { printf("  Dataset is empty.\n"); return; }
    DataType target = read_double("  Enter value to search: ");
    double eps = 1e-9;
    int found = 0;
    for (int i = 0; i < d->size; i++) {
        if (fabs(d->data[i] - target) < eps) {
            printf("  Found %.6g at index %d\n", target, i);
            found++;
        }
    }
    if (!found) printf("  Value %.6g not found in dataset.\n", target);
    else printf("  Total occurrences: %d\n", found);
}

/*
 * op_filter: presents a menu of FilterFunc callbacks,
 * applies the selected one, and shows the filtered result.
 */
void op_filter(Dataset* d) {
    if (d->size == 0) { printf("  Dataset is empty.\n"); return; }

    /* Filter callback table */
    struct { const char* label; FilterFunc fn; } filters[] = {
        { "Values above threshold", filter_above_threshold },
        { "Values below threshold", filter_below_threshold },
        { "Positive values only",   filter_positive        },
        { "Negative values only",   filter_negative        },
    };
    int nf = (int)(sizeof(filters) / sizeof(filters[0]));

    printf("\n  Select filter:\n");
    for (int i = 0; i < nf; i++)
        printf("  %d. %s\n", i + 1, filters[i].label);

    int choice = read_int("  Choice: ", 1, nf);
    FilterFunc selected_filter = filters[choice - 1].fn;

    Dataset* out = dataset_create(d->size);
    if (!out) { printf("[ERROR] Memory error.\n"); return; }

    /* apply_filter is the generic callback executor */
    apply_filter(d, selected_filter, out);

    printf("\n  Filtered result (%d values):\n", out->size);
    op_display(out);

    printf("\n  Replace dataset with filtered result? (1=Yes, 0=No): ");
    int replace = read_int("", 0, 1);
    if (replace) {
        dataset_free(d);
        d->data     = out->data;
        d->size     = out->size;
        d->capacity = out->capacity;
        out->data = NULL;
        printf("  [OK] Dataset replaced.\n");
    }
    dataset_free(out);
    free(out);
}

/*
 * op_transform: presents a menu of TransformFunc callbacks,
 * applies the selected one in-place.
 */
void op_transform(Dataset* d) {
    if (d->size == 0) { printf("  Dataset is empty.\n"); return; }

    /* Transform callback table */
    struct { const char* label; TransformFunc fn; } transforms[] = {
        { "Square each value",       transform_square    },
        { "Square root each value",  transform_sqrt      },
        { "Scale by factor",         transform_scale     },
        { "Negate each value",       transform_negate    },
        { "Normalize (0-1 range)",   transform_normalize },
    };
    int nt = (int)(sizeof(transforms) / sizeof(transforms[0]));

    printf("\n  Select transformation:\n");
    for (int i = 0; i < nt; i++)
        printf("  %d. %s\n", i + 1, transforms[i].label);

    int choice = read_int("  Choice: ", 1, nt);
    TransformFunc selected_transform = transforms[choice - 1].fn;

    apply_transform(d, selected_transform);
    printf("  [OK] Transformation applied.\n");
    op_display(d);
}

void op_add_values(Dataset* d) {
    printf("  How many values to add? ");
    int n = read_int("", 1, 10000);
    printf("  Enter %d value(s):\n", n);
    for (int i = 0; i < n; i++) {
        char prompt[32];
        snprintf(prompt, sizeof(prompt), "  [%d/%d]: ", i + 1, n);
        DataType val = read_double(prompt);
        if (!dataset_push(d, val)) {
            printf("[ERROR] Could not add value.\n");
            break;
        }
    }
    printf("  [OK] Dataset now has %d value(s).\n", d->size);
}

void op_load_file(Dataset* d) {
    char filename[256];
    printf("  Enter filename to load: ");
    if (!fgets(filename, sizeof(filename), stdin)) return;
    filename[strcspn(filename, "\n")] = '\0';

    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("  [ERROR] Cannot open '%s'.\n", filename);
        return;
    }

    dataset_reset(d);
    DataType val;
    int loaded = 0;
    while (fscanf(fp, "%lf", &val) == 1) {
        if (!dataset_push(d, val)) break;
        loaded++;
    }
    fclose(fp);
    printf("  [OK] Loaded %d value(s) from '%s'.\n", loaded, filename);
}

void op_save_file(Dataset* d) {
    if (d->size == 0) { printf("  Dataset is empty — nothing to save.\n"); return; }

    char filename[256];
    printf("  Enter filename to save (default: dataset.txt): ");
    if (!fgets(filename, sizeof(filename), stdin)) return;
    filename[strcspn(filename, "\n")] = '\0';
    if (strlen(filename) == 0) strncpy(filename, "dataset.txt", sizeof(filename));

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        printf("  [ERROR] Cannot write to '%s'.\n", filename);
        return;
    }

    for (int i = 0; i < d->size; i++) {
        fprintf(fp, "%.10g\n", d->data[i]);
    }
    fclose(fp);
    printf("  [OK] %d value(s) saved to '%s'.\n", d->size, filename);
}

void op_reset(Dataset* d) {
    printf("  Reset dataset? All data will be lost. (1=Yes, 0=No): ");
    int confirm = read_int("", 0, 1);
    if (confirm) dataset_reset(d);
    else printf("  Cancelled.\n");
}

/* ============================================================
 *  FILTER CALLBACKS
 * ============================================================ */
static double g_threshold = 0.0;  /* Shared threshold for callbacks */

int filter_above_threshold(DataType val) {
    static int set = 0;
    if (!set) {
        g_threshold = read_double("  Enter threshold: ");
        set = 1;
    }
    return val > g_threshold;
}

int filter_below_threshold(DataType val) {
    static int set = 0;
    if (!set) {
        g_threshold = read_double("  Enter threshold: ");
        set = 1;
    }
    return val < g_threshold;
}

int filter_positive(DataType val) { return val > 0; }
int filter_negative(DataType val) { return val < 0; }

/* ============================================================
 *  TRANSFORM CALLBACKS
 * ============================================================ */
static double g_scale_factor = 1.0;
static double g_norm_min = 0.0, g_norm_range = 1.0;

DataType transform_square(DataType val)  { return val * val; }
DataType transform_negate(DataType val)  { return -val; }

DataType transform_sqrt(DataType val) {
    if (val < 0) {
        printf("  [WARN] sqrt of negative %.4g set to 0.\n", val);
        return 0.0;
    }
    return sqrt(val);
}

DataType transform_scale(DataType val) {
    static int set = 0;
    if (!set) {
        g_scale_factor = read_double("  Enter scale factor: ");
        set = 1;
    }
    return val * g_scale_factor;
}

/*
 * transform_normalize: maps values to [0,1] range.
 * Min and range are computed once by apply_transform before
 * this callback is invoked per element.
 */
DataType transform_normalize(DataType val) {
    if (g_norm_range == 0) return 0.0;
    return (val - g_norm_min) / g_norm_range;
}

/* ============================================================
 *  COMPARISON CALLBACKS  (for qsort)
 * ============================================================ */
int compare_asc(const void* a, const void* b) {
    DataType x = *(DataType*)a;
    DataType y = *(DataType*)b;
    return (x > y) - (x < y);
}

int compare_desc(const void* a, const void* b) {
    return compare_asc(b, a);
}

/* ============================================================
 *  GENERIC APPLY FUNCTIONS
 * ============================================================ */
/*
 * apply_filter: iterates dataset, calls FilterFunc callback
 * on each element, copies matching values to output dataset.
 */
void apply_filter(Dataset* d, FilterFunc fn, Dataset* out) {
    check_null_fn((void*)fn, "filter");
    if (!fn) return;
    for (int i = 0; i < d->size; i++) {
        if (fn(d->data[i])) {
            dataset_push(out, d->data[i]);
        }
    }
}

/*
 * apply_transform: iterates dataset, calls TransformFunc
 * callback on each element in-place.
 * Special case: normalize needs min/range pre-computed.
 */
void apply_transform(Dataset* d, TransformFunc fn) {
    check_null_fn((void*)fn, "transform");
    if (!fn) return;

    /* Pre-compute normalization params if needed */
    if (fn == transform_normalize) {
        DataType mn = d->data[0], mx = d->data[0];
        for (int i = 1; i < d->size; i++) {
            if (d->data[i] < mn) mn = d->data[i];
            if (d->data[i] > mx) mx = d->data[i];
        }
        g_norm_min   = mn;
        g_norm_range = mx - mn;
    }

    for (int i = 0; i < d->size; i++) {
        d->data[i] = fn(d->data[i]);
    }
}

/* ============================================================
 *  INPUT HELPERS
 * ============================================================ */
int read_int(const char* prompt, int min, int max) {
    int val;
    while (1) {
        if (strlen(prompt) > 0) printf("%s", prompt);
        if (scanf("%d", &val) == 1 && val >= min && val <= max) {
            /* consume rest of line */
            int c; while ((c = getchar()) != '\n' && c != EOF);
            return val;
        }
        int c; while ((c = getchar()) != '\n' && c != EOF);
        printf("  [ERROR] Enter a number between %d and %d: ", min, max);
    }
}

DataType read_double(const char* prompt) {
    DataType val;
    while (1) {
        printf("%s", prompt);
        if (scanf("%lf", &val) == 1) {
            int c; while ((c = getchar()) != '\n' && c != EOF);
            return val;
        }
        int c; while ((c = getchar()) != '\n' && c != EOF);
        printf("  [ERROR] Enter a valid number: ");
    }
}

void check_null_fn(void* fn, const char* name) {
    if (fn == NULL) {
        printf("[ERROR] NULL function pointer detected for '%s'.\n", name);
    }
}

void print_separator(void) {
    printf("  ------------------------------------------------\n");
}
