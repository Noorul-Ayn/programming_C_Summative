/*
 * ============================================================
 *  Course Performance and Academic Records Analyzer v1.0
 * ============================================================
 *  Features:
 *    - Dynamic memory management (malloc/realloc/free)
 *    - CRUD operations on student records
 *    - Search by ID and name
 *    - Sort by GPA, name, ID
 *    - Performance reports and statistics
 *    - File persistence (binary format)
 *    - Full input validation and error handling
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================
 *  CONSTANTS
 * ============================================================ */
#define MAX_NAME_LEN     64
#define MAX_COURSE_LEN   64
#define MAX_SUBJECTS     10
#define DATA_FILE        "records.dat"

/* ============================================================
 *  STRUCTURE
 * ============================================================ */
typedef struct {
    int    id;
    char   name[MAX_NAME_LEN];
    char   course[MAX_COURSE_LEN];
    int    age;
    int    num_subjects;
    float  grades[MAX_SUBJECTS];
    float  gpa;
} Student;

/* ============================================================
 *  GLOBALS
 * ============================================================ */
Student* students = NULL;   /* Heap-allocated array */
int      count    = 0;      /* Current number of records */
int      capacity = 0;      /* Allocated capacity */

/* ============================================================
 *  FUNCTION PROTOTYPES
 * ============================================================ */
/* Memory */
int  ensure_capacity(void);
void free_all(void);

/* GPA */
float compute_gpa(float* grades, int n);

/* CRUD */
void add_student(void);
void display_all(void);
void display_one(int idx);
void update_student(void);
void delete_student(void);

/* Search */
int  search_by_id(int id);
void search_menu(void);

/* Sort */
void sort_by_gpa(void);
void sort_by_name(void);
void sort_by_id(void);
void sort_menu(void);

/* Reports */
void report_class_average(void);
void report_highest_lowest(void);
void report_median(void);
void report_top_n(void);
void report_best_per_course(void);
void report_course_average(void);
void reports_menu(void);

/* File I/O */
void save_records(void);
void load_records(void);

/* Input helpers */
int   read_int(const char* prompt, int min, int max);
float read_float(const char* prompt, float min, float max);
void  read_string(const char* prompt, char* buf, int max_len);
void  clear_input(void);

/* UI */
void print_separator(void);
void main_menu(void);

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(void) {
    load_records();
    main_menu();
    save_records();
    free_all();
    return 0;
}

/* ============================================================
 *  MEMORY MANAGEMENT
 * ============================================================ */
int ensure_capacity(void) {
    if (count < capacity) return 1;

    int new_cap = (capacity == 0) ? 4 : capacity * 2;
    Student* tmp = (Student*) realloc(students, new_cap * sizeof(Student));
    if (tmp == NULL) {
        printf("[ERROR] Memory allocation failed.\n");
        return 0;
    }
    students = tmp;
    capacity = new_cap;
    return 1;
}

void free_all(void) {
    free(students);
    students = NULL;
    count = capacity = 0;
}

/* ============================================================
 *  GPA CALCULATION
 * ============================================================ */
float compute_gpa(float* grades, int n) {
    if (n <= 0) return 0.0f;
    float sum = 0.0f;
    for (int i = 0; i < n; i++) sum += grades[i];
    return sum / n;
}

/* ============================================================
 *  ADD STUDENT
 * ============================================================ */
void add_student(void) {
    if (!ensure_capacity()) return;

    Student s;
    memset(&s, 0, sizeof(Student));

    /* ID — must be unique */
    while (1) {
        s.id = read_int("  Enter Student ID: ", 1, 999999);
        if (search_by_id(s.id) == -1) break;
        printf("  [ERROR] Student ID %d already exists. Try again.\n", s.id);
    }

    read_string("  Enter Name: ", s.name, MAX_NAME_LEN);
    read_string("  Enter Course: ", s.course, MAX_COURSE_LEN);
    s.age = read_int("  Enter Age: ", 10, 100);

    s.num_subjects = read_int("  Number of subjects (1-10): ", 1, MAX_SUBJECTS);
    for (int i = 0; i < s.num_subjects; i++) {
        char prompt[64];
        snprintf(prompt, sizeof(prompt), "  Grade for subject %d (0-100): ", i + 1);
        s.grades[i] = read_float(prompt, 0.0f, 100.0f);
    }

    s.gpa = compute_gpa(s.grades, s.num_subjects);
    students[count++] = s;

    printf("  [OK] Student added. GPA: %.2f\n", s.gpa);
}

/* ============================================================
 *  DISPLAY
 * ============================================================ */
void display_one(int idx) {
    Student* s = &students[idx];
    print_separator();
    printf("  ID      : %d\n", s->id);
    printf("  Name    : %s\n", s->name);
    printf("  Course  : %s\n", s->course);
    printf("  Age     : %d\n", s->age);
    printf("  Grades  : ");
    for (int i = 0; i < s->num_subjects; i++) {
        printf("%.1f ", s->grades[i]);
    }
    printf("\n");
    printf("  GPA     : %.2f\n", s->gpa);
    print_separator();
}

void display_all(void) {
    if (count == 0) {
        printf("  No records found.\n");
        return;
    }
    printf("\n  %-6s %-20s %-20s %-4s %-6s\n",
           "ID", "Name", "Course", "Age", "GPA");
    print_separator();
    for (int i = 0; i < count; i++) {
        printf("  %-6d %-20s %-20s %-4d %.2f\n",
               students[i].id,
               students[i].name,
               students[i].course,
               students[i].age,
               students[i].gpa);
    }
    print_separator();
    printf("  Total records: %d\n", count);
}

/* ============================================================
 *  UPDATE
 * ============================================================ */
void update_student(void) {
    if (count == 0) { printf("  No records.\n"); return; }

    int id = read_int("  Enter Student ID to update: ", 1, 999999);
    int idx = search_by_id(id);
    if (idx == -1) { printf("  [ERROR] Student ID %d not found.\n", id); return; }

    Student* s = &students[idx];
    printf("  Updating record for %s\n", s->name);
    printf("  (Press Enter to keep current value)\n\n");

    char buf[MAX_NAME_LEN];

    printf("  Current Name: %s\n", s->name);
    read_string("  New Name (or Enter to skip): ", buf, MAX_NAME_LEN);
    if (strlen(buf) > 0) strncpy(s->name, buf, MAX_NAME_LEN);

    printf("  Current Course: %s\n", s->course);
    read_string("  New Course (or Enter to skip): ", buf, MAX_COURSE_LEN);
    if (strlen(buf) > 0) strncpy(s->course, buf, MAX_COURSE_LEN);

    printf("  Current Age: %d\n", s->age);
    int new_age = read_int("  New Age (0 to skip): ", 0, 100);
    if (new_age > 0) s->age = new_age;

    printf("  Re-enter grades? (1=Yes, 0=No): ");
    int regrade = read_int("", 0, 1);
    if (regrade) {
        s->num_subjects = read_int("  Number of subjects (1-10): ", 1, MAX_SUBJECTS);
        for (int i = 0; i < s->num_subjects; i++) {
            char prompt[64];
            snprintf(prompt, sizeof(prompt), "  Grade for subject %d (0-100): ", i + 1);
            s->grades[i] = read_float(prompt, 0.0f, 100.0f);
        }
        s->gpa = compute_gpa(s->grades, s->num_subjects);
        printf("  New GPA: %.2f\n", s->gpa);
    }

    printf("  [OK] Record updated.\n");
}

/* ============================================================
 *  DELETE
 * ============================================================ */
void delete_student(void) {
    if (count == 0) { printf("  No records.\n"); return; }

    int id = read_int("  Enter Student ID to delete: ", 1, 999999);
    int idx = search_by_id(id);
    if (idx == -1) { printf("  [ERROR] ID %d not found.\n", id); return; }

    printf("  Delete %s? (1=Yes, 0=No): ", students[idx].name);
    int confirm = read_int("", 0, 1);
    if (!confirm) { printf("  Cancelled.\n"); return; }

    /* Shift records left */
    for (int i = idx; i < count - 1; i++) {
        students[i] = students[i + 1];
    }
    count--;
    printf("  [OK] Record deleted.\n");
}

/* ============================================================
 *  SEARCH
 * ============================================================ */
int search_by_id(int id) {
    for (int i = 0; i < count; i++) {
        if (students[i].id == id) return i;
    }
    return -1;
}

void search_menu(void) {
    printf("\n  Search by:\n");
    printf("  1. Student ID\n");
    printf("  2. Student Name\n");
    int choice = read_int("  Choice: ", 1, 2);

    if (choice == 1) {
        int id = read_int("  Enter ID: ", 1, 999999);
        int idx = search_by_id(id);
        if (idx == -1) printf("  Not found.\n");
        else display_one(idx);
    } else {
        char name[MAX_NAME_LEN];
        read_string("  Enter name (partial match): ", name, MAX_NAME_LEN);
        int found = 0;
        for (int i = 0; i < count; i++) {
            /* Case-insensitive partial match */
            char lower_rec[MAX_NAME_LEN], lower_in[MAX_NAME_LEN];
            strncpy(lower_rec, students[i].name, MAX_NAME_LEN);
            strncpy(lower_in, name, MAX_NAME_LEN);
            for (int j = 0; lower_rec[j]; j++) lower_rec[j] = tolower(lower_rec[j]);
            for (int j = 0; lower_in[j]; j++) lower_in[j] = tolower(lower_in[j]);
            if (strstr(lower_rec, lower_in)) {
                display_one(i);
                found++;
            }
        }
        if (!found) printf("  No matching records.\n");
    }
}

/* ============================================================
 *  SORTING  (bubble sort)
 * ============================================================ */
void sort_by_gpa(void) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (students[j].gpa < students[j + 1].gpa) {
                Student tmp = students[j];
                students[j] = students[j + 1];
                students[j + 1] = tmp;
            }
        }
    }
    printf("  [OK] Sorted by GPA (descending).\n");
    display_all();
}

void sort_by_name(void) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (strcmp(students[j].name, students[j + 1].name) > 0) {
                Student tmp = students[j];
                students[j] = students[j + 1];
                students[j + 1] = tmp;
            }
        }
    }
    printf("  [OK] Sorted by name (A-Z).\n");
    display_all();
}

void sort_by_id(void) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (students[j].id > students[j + 1].id) {
                Student tmp = students[j];
                students[j] = students[j + 1];
                students[j + 1] = tmp;
            }
        }
    }
    printf("  [OK] Sorted by ID (ascending).\n");
    display_all();
}

void sort_menu(void) {
    printf("\n  Sort by:\n");
    printf("  1. GPA\n");
    printf("  2. Name\n");
    printf("  3. ID\n");
    int choice = read_int("  Choice: ", 1, 3);
    if (count == 0) { printf("  No records.\n"); return; }
    switch (choice) {
        case 1: sort_by_gpa();  break;
        case 2: sort_by_name(); break;
        case 3: sort_by_id();   break;
    }
}

/* ============================================================
 *  REPORTS
 * ============================================================ */
void report_class_average(void) {
    if (count == 0) { printf("  No records.\n"); return; }
    float sum = 0.0f;
    for (int i = 0; i < count; i++) sum += students[i].gpa;
    printf("  Class Average GPA: %.2f\n", sum / count);
}

void report_highest_lowest(void) {
    if (count == 0) { printf("  No records.\n"); return; }
    int hi = 0, lo = 0;
    for (int i = 1; i < count; i++) {
        if (students[i].gpa > students[hi].gpa) hi = i;
        if (students[i].gpa < students[lo].gpa) lo = i;
    }
    printf("  Highest GPA: %s (%.2f)\n", students[hi].name, students[hi].gpa);
    printf("  Lowest  GPA: %s (%.2f)\n", students[lo].name, students[lo].gpa);
}

void report_median(void) {
    if (count == 0) { printf("  No records.\n"); return; }

    /* Copy GPAs, sort, find median */
    float* gpas = (float*) malloc(count * sizeof(float));
    if (!gpas) { printf("  [ERROR] Memory error.\n"); return; }

    for (int i = 0; i < count; i++) gpas[i] = students[i].gpa;

    /* Bubble sort copy */
    for (int i = 0; i < count - 1; i++)
        for (int j = 0; j < count - i - 1; j++)
            if (gpas[j] > gpas[j + 1]) {
                float t = gpas[j]; gpas[j] = gpas[j + 1]; gpas[j + 1] = t;
            }

    float median;
    if (count % 2 == 0)
        median = (gpas[count / 2 - 1] + gpas[count / 2]) / 2.0f;
    else
        median = gpas[count / 2];

    printf("  Median GPA: %.2f\n", median);
    free(gpas);
}

void report_top_n(void) {
    if (count == 0) { printf("  No records.\n"); return; }
    int n = read_int("  Show top N students: ", 1, count);

    /* Temp sorted copy */
    Student* tmp = (Student*) malloc(count * sizeof(Student));
    if (!tmp) { printf("  [ERROR] Memory error.\n"); return; }
    memcpy(tmp, students, count * sizeof(Student));

    for (int i = 0; i < count - 1; i++)
        for (int j = 0; j < count - i - 1; j++)
            if (tmp[j].gpa < tmp[j + 1].gpa) {
                Student t = tmp[j]; tmp[j] = tmp[j + 1]; tmp[j + 1] = t;
            }

    printf("\n  Top %d Students:\n", n);
    printf("  %-4s %-20s %-20s %-6s\n", "Rank", "Name", "Course", "GPA");
    print_separator();
    for (int i = 0; i < n; i++) {
        printf("  %-4d %-20s %-20s %.2f\n",
               i + 1, tmp[i].name, tmp[i].course, tmp[i].gpa);
    }
    free(tmp);
}

void report_best_per_course(void) {
    if (count == 0) { printf("  No records.\n"); return; }

    printf("\n  Best Student Per Course:\n");
    printf("  %-20s %-20s %-6s\n", "Course", "Student", "GPA");
    print_separator();

    char seen[100][MAX_COURSE_LEN];
    int  seen_count = 0;

    for (int i = 0; i < count; i++) {
        /* Check if course already processed */
        int already = 0;
        for (int s = 0; s < seen_count; s++) {
            if (strcmp(seen[s], students[i].course) == 0) { already = 1; break; }
        }
        if (already) continue;

        /* Find best in this course */
        int best = i;
        for (int j = i + 1; j < count; j++) {
            if (strcmp(students[j].course, students[i].course) == 0) {
                if (students[j].gpa > students[best].gpa) best = j;
            }
        }

        printf("  %-20s %-20s %.2f\n",
               students[i].course, students[best].name, students[best].gpa);

        strncpy(seen[seen_count++], students[i].course, MAX_COURSE_LEN);
    }
}

void report_course_average(void) {
    if (count == 0) { printf("  No records.\n"); return; }

    printf("\n  Course Average GPA:\n");
    printf("  %-20s %-10s %-10s\n", "Course", "Students", "Avg GPA");
    print_separator();

    char seen[100][MAX_COURSE_LEN];
    int  seen_count = 0;

    for (int i = 0; i < count; i++) {
        int already = 0;
        for (int s = 0; s < seen_count; s++) {
            if (strcmp(seen[s], students[i].course) == 0) { already = 1; break; }
        }
        if (already) continue;

        float sum = 0.0f;
        int   n   = 0;
        for (int j = 0; j < count; j++) {
            if (strcmp(students[j].course, students[i].course) == 0) {
                sum += students[j].gpa;
                n++;
            }
        }

        printf("  %-20s %-10d %.2f\n", students[i].course, n, sum / n);
        strncpy(seen[seen_count++], students[i].course, MAX_COURSE_LEN);
    }
}

void reports_menu(void) {
    printf("\n  Reports:\n");
    printf("  1. Class average GPA\n");
    printf("  2. Highest and lowest GPA\n");
    printf("  3. Median GPA\n");
    printf("  4. Top N students\n");
    printf("  5. Best student per course\n");
    printf("  6. Course average performance\n");
    int choice = read_int("  Choice: ", 1, 6);
    printf("\n");
    switch (choice) {
        case 1: report_class_average();  break;
        case 2: report_highest_lowest(); break;
        case 3: report_median();         break;
        case 4: report_top_n();          break;
        case 5: report_best_per_course();break;
        case 6: report_course_average(); break;
    }
}

/* ============================================================
 *  FILE I/O  (binary format)
 * ============================================================ */
void save_records(void) {
    FILE* fp = fopen(DATA_FILE, "wb");
    if (!fp) {
        printf("  [ERROR] Could not open %s for writing.\n", DATA_FILE);
        return;
    }
    fwrite(&count, sizeof(int), 1, fp);
    fwrite(students, sizeof(Student), count, fp);
    fclose(fp);
    printf("  [OK] %d record(s) saved to %s\n", count, DATA_FILE);
}

void load_records(void) {
    FILE* fp = fopen(DATA_FILE, "rb");
    if (!fp) return;   /* No file yet — first run */

    int saved_count = 0;
    if (fread(&saved_count, sizeof(int), 1, fp) != 1 || saved_count <= 0) {
        fclose(fp);
        return;
    }

    Student* tmp = (Student*) malloc(saved_count * sizeof(Student));
    if (!tmp) {
        printf("  [ERROR] Memory error loading records.\n");
        fclose(fp);
        return;
    }

    size_t read = fread(tmp, sizeof(Student), saved_count, fp);
    fclose(fp);

    if ((int)read != saved_count) {
        printf("  [WARN] File may be corrupted. Loaded %zu/%d records.\n",
               read, saved_count);
    }

    free(students);
    students = tmp;
    count    = (int)read;
    capacity = saved_count;

    printf("  [OK] Loaded %d record(s) from %s\n", count, DATA_FILE);
}

/* ============================================================
 *  INPUT HELPERS
 * ============================================================ */
void clear_input(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int read_int(const char* prompt, int min, int max) {
    int val;
    while (1) {
        if (strlen(prompt) > 0) printf("%s", prompt);
        if (scanf("%d", &val) == 1 && val >= min && val <= max) {
            clear_input();
            return val;
        }
        clear_input();
        if (min == 0 && max == 1)
            printf("  [ERROR] Enter 0 or 1: ");
        else
            printf("  [ERROR] Enter a number between %d and %d: ", min, max);
    }
}

float read_float(const char* prompt, float min, float max) {
    float val;
    while (1) {
        printf("%s", prompt);
        if (scanf("%f", &val) == 1 && val >= min && val <= max) {
            clear_input();
            return val;
        }
        clear_input();
        printf("  [ERROR] Enter a value between %.1f and %.1f: ", min, max);
    }
}

void read_string(const char* prompt, char* buf, int max_len) {
    printf("%s", prompt);
    if (fgets(buf, max_len, stdin)) {
        buf[strcspn(buf, "\n")] = '\0';   /* Strip newline */
    }
}

/* ============================================================
 *  UI
 * ============================================================ */
void print_separator(void) {
    printf("  ------------------------------------------------\n");
}

void main_menu(void) {
    int choice;
    while (1) {
        printf("\n");
        print_separator();
        printf("   ACADEMIC RECORDS ANALYZER\n");
        print_separator();
        printf("  1.  Add student record\n");
        printf("  2.  Display all records\n");
        printf("  3.  Update record\n");
        printf("  4.  Delete record\n");
        printf("  5.  Search records\n");
        printf("  6.  Sort records\n");
        printf("  7.  Performance reports\n");
        printf("  8.  Save records to file\n");
        printf("  9.  Load records from file\n");
        printf("  10. Exit\n");
        print_separator();
        printf("  Total records loaded: %d\n\n", count);

        choice = read_int("  Enter choice [1-10]: ", 1, 10);

        switch (choice) {
            case 1:  add_student();    break;
            case 2:  display_all();    break;
            case 3:  update_student(); break;
            case 4:  delete_student(); break;
            case 5:  search_menu();    break;
            case 6:  sort_menu();      break;
            case 7:  reports_menu();   break;
            case 8:  save_records();   break;
            case 9:  load_records();   break;
            case 10:
                save_records();
                printf("  Goodbye!\n");
                return;
        }
    }
}
