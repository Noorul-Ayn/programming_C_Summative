/*
 * ============================================================
 *  Multi-threaded Web Scraper v1.0
 *  Uses POSIX threads (pthreads) + libcurl
 * ============================================================
 *  Architecture:
 *    - One thread per URL, all launched concurrently
 *    - Each thread fetches its URL and saves to a .txt file
 *    - No shared mutable state — no mutex needed
 *    - Basic error handling for unreachable URLs
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>
#include <time.h>
#include <sys/stat.h>

/* ============================================================
 *  CONSTANTS
 * ============================================================ */
#define MAX_URL_LEN     512
#define MAX_FILENAME    256
#define OUTPUT_DIR      "scraped_output"
#define CONNECT_TIMEOUT 10L   /* seconds */
#define TRANSFER_TIMEOUT 30L  /* seconds */

/* ============================================================
 *  STRUCTURES
 * ============================================================ */

/* Passed to each thread — all read-only after creation */
typedef struct {
    int   thread_id;
    char  url[MAX_URL_LEN];
    char  output_file[MAX_FILENAME];
} ThreadArgs;

/* Result returned from each thread */
typedef struct {
    int    thread_id;
    char   url[MAX_URL_LEN];
    int    success;       /* 1 = ok, 0 = failed */
    long   http_code;
    size_t bytes_written;
    double elapsed_ms;
    char   error_msg[256];
} ThreadResult;

/* Buffer used by curl write callback */
typedef struct {
    FILE*  fp;
    size_t total_written;
} WriteState;

/* ============================================================
 *  CURL WRITE CALLBACK
 *  Called by curl whenever data arrives — writes to file.
 *  No shared state — each thread has its own WriteState.
 * ============================================================ */
static size_t write_callback(void* ptr, size_t size,
                              size_t nmemb, void* userdata) {
    WriteState* ws = (WriteState*) userdata;
    size_t bytes = size * nmemb;
    size_t written = fwrite(ptr, 1, bytes, ws->fp);
    ws->total_written += written;
    return written;
}

/* ============================================================
 *  THREAD FUNCTION
 *  Each thread:
 *    1. Opens its output file
 *    2. Initialises a private CURL handle
 *    3. Fetches the URL
 *    4. Writes HTML to file
 *    5. Returns a heap-allocated ThreadResult
 * ============================================================ */
void* fetch_url(void* arg) {
    ThreadArgs*   args   = (ThreadArgs*) arg;
    ThreadResult* result = (ThreadResult*) malloc(sizeof(ThreadResult));
    if (!result) pthread_exit(NULL);

    /* Initialise result */
    memset(result, 0, sizeof(ThreadResult));
    result->thread_id = args->thread_id;
    strncpy(result->url, args->url, MAX_URL_LEN);

    /* Start timer */
    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    /* Open output file */
    FILE* fp = fopen(args->output_file, "w");
    if (!fp) {
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Cannot open output file: %s", args->output_file);
        result->success = 0;
        pthread_exit(result);
    }

    /* Write header comment into file */
    fprintf(fp, "# URL: %s\n", args->url);
    fprintf(fp, "# Thread ID: %d\n", args->thread_id);
    fprintf(fp, "# ----------------------------------------\n\n");

    WriteState ws = { fp, 0 };

    /* Init private curl handle */
    CURL* curl = curl_easy_init();
    if (!curl) {
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "curl_easy_init() failed.");
        result->success = 0;
        fclose(fp);
        pthread_exit(result);
    }

    /* Configure curl */
    curl_easy_setopt(curl, CURLOPT_URL,            args->url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &ws);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        TRANSFER_TIMEOUT);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  /* follow redirects */
    curl_easy_setopt(curl, CURLOPT_USERAGENT,
                     "Mozilla/5.0 (C-WebScraper/1.0)");
    /* Disable SSL verification for simplicity */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    /* Perform request */
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "curl error: %s", curl_easy_strerror(res));
        result->success = 0;
    } else {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result->http_code);
        result->success       = (result->http_code >= 200 &&
                                  result->http_code < 400) ? 1 : 0;
        result->bytes_written = ws.total_written;
        if (!result->success) {
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "HTTP %ld", result->http_code);
        }
    }

    curl_easy_cleanup(curl);
    fclose(fp);

    /* Stop timer */
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    result->elapsed_ms = (t_end.tv_sec  - t_start.tv_sec)  * 1000.0 +
                         (t_end.tv_nsec - t_start.tv_nsec) / 1e6;

    pthread_exit(result);
}

/* ============================================================
 *  HELPERS
 * ============================================================ */

/* Sanitise a URL into a safe filename */
void url_to_filename(const char* url, char* out, int max_len) {
    snprintf(out, max_len, "%s/", OUTPUT_DIR);
    int pos = (int)strlen(out);

    /* Skip protocol */
    const char* start = strstr(url, "://");
    start = start ? start + 3 : url;

    for (int i = 0; start[i] && pos < max_len - 5; i++) {
        char c = start[i];
        if (c == '/' || c == '?' || c == '&' ||
            c == ':' || c == '=' || c == '#') c = '_';
        out[pos++] = c;
    }
    /* Trim trailing underscores */
    while (pos > 1 && out[pos - 1] == '_') pos--;
    out[pos] = '\0';
    strncat(out, ".txt", max_len - strlen(out) - 1);
}

void print_separator(void) {
    printf("  ------------------------------------------------\n");
}

/* Load URLs from a file (one per line, # = comment) */
int load_urls_from_file(const char* filename,
                         char urls[][MAX_URL_LEN], int max_urls) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("[ERROR] Cannot open '%s'.\n", filename);
        return 0;
    }
    int count = 0;
    char line[MAX_URL_LEN];
    while (fgets(line, sizeof(line), fp) && count < max_urls) {
        /* Strip newline */
        line[strcspn(line, "\n")] = '\0';
        /* Skip empty lines and comments */
        if (strlen(line) == 0 || line[0] == '#') continue;
        strncpy(urls[count++], line, MAX_URL_LEN);
    }
    fclose(fp);
    return count;
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(int argc, char* argv[]) {

    printf("\n");
    print_separator();
    printf("   MULTI-THREADED WEB SCRAPER v1.0\n");
    printf("   POSIX Threads + libcurl\n");
    print_separator();

    /* --- Collect URLs ------------------------------------ */
    static char urls[64][MAX_URL_LEN];
    int url_count = 0;

    if (argc >= 2) {
        /* Mode 1: URLs passed as command-line arguments */
        url_count = argc - 1;
        if (url_count > 64) url_count = 64;
        for (int i = 0; i < url_count; i++) {
            strncpy(urls[i], argv[i + 1], MAX_URL_LEN);
        }
    } else {
        /* Mode 2: Interactive — type URLs or load from file */
        printf("\n  Options:\n");
        printf("  1. Enter URLs manually\n");
        printf("  2. Load URLs from file (urls.txt)\n");
        printf("  Choice [1-2]: ");

        int choice;
        scanf("%d", &choice);
        while (getchar() != '\n');

        if (choice == 2) {
            url_count = load_urls_from_file("urls.txt", urls, 64);
            if (url_count == 0) {
                printf("[ERROR] No URLs loaded. Exiting.\n");
                return 1;
            }
            printf("  [OK] Loaded %d URL(s) from urls.txt\n", url_count);
        } else {
            printf("  How many URLs to scrape? ");
            scanf("%d", &url_count);
            while (getchar() != '\n');
            if (url_count < 1 || url_count > 64) url_count = 1;

            for (int i = 0; i < url_count; i++) {
                printf("  URL %d: ", i + 1);
                if (fgets(urls[i], MAX_URL_LEN, stdin)) {
                    urls[i][strcspn(urls[i], "\n")] = '\0';
                }
            }
        }
    }

    /* --- Create output directory ------------------------- */
    mkdir(OUTPUT_DIR, 0755);

    /* --- Initialise curl globally (once, before threads) - */
    curl_global_init(CURL_GLOBAL_ALL);

    /* --- Build thread args ------------------------------- */
    pthread_t*   threads = (pthread_t*)   malloc(url_count * sizeof(pthread_t));
    ThreadArgs*  args    = (ThreadArgs*)  malloc(url_count * sizeof(ThreadArgs));

    if (!threads || !args) {
        printf("[FATAL] Memory allocation failed.\n");
        return 1;
    }

    printf("\n  Launching %d thread(s)...\n\n", url_count);

    for (int i = 0; i < url_count; i++) {
        args[i].thread_id = i + 1;
        strncpy(args[i].url, urls[i], MAX_URL_LEN);
        url_to_filename(urls[i], args[i].output_file, MAX_FILENAME);

        printf("  [Thread %d] Fetching: %s\n", i + 1, urls[i]);
        printf("             Output:   %s\n", args[i].output_file);
    }

    print_separator();

    /* --- Launch all threads concurrently ----------------- */
    for (int i = 0; i < url_count; i++) {
        int rc = pthread_create(&threads[i], NULL, fetch_url, &args[i]);
        if (rc != 0) {
            printf("[ERROR] Failed to create thread %d (rc=%d)\n", i + 1, rc);
            threads[i] = 0;
        }
    }

    /* --- Wait for all threads and collect results -------- */
    int success_count = 0;
    int fail_count    = 0;

    printf("\n  Results:\n");
    print_separator();

    for (int i = 0; i < url_count; i++) {
        if (threads[i] == 0) {
            printf("  [Thread %d] SKIPPED (creation failed)\n", i + 1);
            fail_count++;
            continue;
        }

        ThreadResult* result = NULL;
        pthread_join(threads[i], (void**) &result);

        if (!result) {
            printf("  [Thread %d] ERROR: no result returned\n", i + 1);
            fail_count++;
            continue;
        }

        if (result->success) {
            printf("  [Thread %d] OK    | HTTP %ld | %zu bytes | %.0f ms\n"
                   "             URL: %s\n"
                   "             File: %s\n",
                   result->thread_id,
                   result->http_code,
                   result->bytes_written,
                   result->elapsed_ms,
                   result->url,
                   args[i].output_file);
            success_count++;
        } else {
            printf("  [Thread %d] FAIL  | %s\n"
                   "             URL: %s\n",
                   result->thread_id,
                   result->error_msg,
                   result->url);
            fail_count++;
        }

        free(result);
    }

    /* --- Summary ----------------------------------------- */
    print_separator();
    printf("  Summary: %d succeeded, %d failed\n",
           success_count, fail_count);
    printf("  Output directory: ./%s/\n", OUTPUT_DIR);
    print_separator();

    /* --- Cleanup ----------------------------------------- */
    free(threads);
    free(args);
    curl_global_cleanup();

    return (fail_count > 0) ? 1 : 0;
}
