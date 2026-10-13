/* Index allocator: hands out the lowest unused index starting at 1, supports delete by index.
 * Capacity: UINT32_MAX indices (1 .. 4294967295).
 * Build: gcc -std=c11 -O2 -Wall -Wextra -g -fsanitize=address,undefined indexdb.c -o indexdb
 * Uses __builtin_ctzll (GCC/Clang). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define PAGE_BITS   4096u                    /* one page = 4096 bits = 64 words = 512 bytes */
#define PAGE_WORDS  (PAGE_BITS / 64)
#define NPAGES      (1u << 20)               /* 2^20 pages * 2^12 bits = 2^32 positions */
#define S1_WORDS    (NPAGES / 64)            /* bit p of S1 = "page p is full" */
#define S2_WORDS    (S1_WORDS / 64)          /* bit j of S2 = "S1 word j is full" */
#define S3_WORDS    (S2_WORDS / 64)          /* bit k of S3 = "S2 word k is full" (4 words) */
#define ALL         (~(uint64_t)0)

/* position v = index - 1 (0-based). Position 2^32-1 would be index 0, so it is
 * pre-marked used and never handed out. */
typedef struct IndexDB {
    uint64_t *pages[NPAGES];                 /* allocated lazily (sparse use stays cheap) */
    uint64_t s1[S1_WORDS];
    uint64_t s2[S2_WORDS];
    uint64_t s3[S3_WORDS];
    uint32_t count;                          /* indices currently in use */
} IndexDB;

static int get_bit(const IndexDB *db, uint32_t v) {
    const uint64_t *pg = db->pages[v >> 12];
    return pg && ((pg[(v >> 6) & 63] >> (v & 63)) & 1);
}

/* Marks position v used. Returns 0 on allocation failure. */
static int set_bit(IndexDB *db, uint32_t v) {
    uint32_t p = v >> 12;
    if (!db->pages[p]) {
        db->pages[p] = calloc(PAGE_WORDS, sizeof(uint64_t));
        if (!db->pages[p]) return 0;
    }
    uint64_t *pg = db->pages[p];
    uint64_t *w = &pg[(v >> 6) & 63];
    *w |= (uint64_t)1 << (v & 63);
    if (*w != ALL) return 1;                 /* word not full -> page not newly full */
    for (unsigned i = 0; i < PAGE_WORDS; ++i) if (pg[i] != ALL) return 1;
    /* page became full: propagate up while the parent word becomes full */
    db->s1[p >> 6] |= (uint64_t)1 << (p & 63);
    if (db->s1[p >> 6] != ALL) return 1;
    uint32_t j = p >> 6;
    db->s2[j >> 6] |= (uint64_t)1 << (j & 63);
    if (db->s2[j >> 6] != ALL) return 1;
    uint32_t k = j >> 6;
    db->s3[k >> 6] |= (uint64_t)1 << (k & 63);
    return 1;
}

static void clear_bit(IndexDB *db, uint32_t v) {
    uint32_t p = v >> 12, j = p >> 6, k = j >> 6;
    db->pages[p][(v >> 6) & 63] &= ~((uint64_t)1 << (v & 63));
    /* everything above this page is no longer full */
    db->s1[j] &= ~((uint64_t)1 << (p & 63));
    db->s2[k] &= ~((uint64_t)1 << (j & 63));
    db->s3[k >> 6] &= ~((uint64_t)1 << (k & 63));
}

IndexDB *idx_create(void) {
    IndexDB *db = calloc(1, sizeof *db);
    if (!db) return NULL;
    if (!set_bit(db, UINT32_MAX)) { free(db); return NULL; }   /* reserve "index 0" slot */
    return db;
}

void idx_destroy(IndexDB *db) {
    if (!db) return;
    for (uint32_t p = 0; p < NPAGES; ++p) free(db->pages[p]);
    free(db);
}

/* Returns the lowest unused index (>= 1), or 0 if full / out of memory. */
uint32_t idx_alloc(IndexDB *db) {
    unsigned i = 0;
    while (i < S3_WORDS && db->s3[i] == ALL) ++i;
    if (i == S3_WORDS) return 0;                                   /* all full */
    uint32_t k = i * 64 + (uint32_t)__builtin_ctzll(~db->s3[i]);   /* first non-full S2 word */
    uint32_t j = k * 64 + (uint32_t)__builtin_ctzll(~db->s2[k]);   /* first non-full S1 word */
    uint32_t p = j * 64 + (uint32_t)__builtin_ctzll(~db->s1[j]);   /* first non-full page */
    const uint64_t *pg = db->pages[p];
    uint32_t v = p << 12;
    if (pg) {                                                      /* page never touched: bit 0 is free */
        unsigned w = 0;
        while (pg[w] == ALL) ++w;
        v += w * 64 + (uint32_t)__builtin_ctzll(~pg[w]);
    }
    if (!set_bit(db, v)) return 0;
    db->count++;
    return v + 1;
}

/* Frees an index. Returns 1 on success, 0 if the index is invalid or not in use. */
int idx_free(IndexDB *db, uint32_t index) {
    if (index == 0) return 0;
    uint32_t v = index - 1;
    if (!get_bit(db, v)) return 0;
    clear_bit(db, v);
    db->count--;
    return 1;
}

int idx_in_use(const IndexDB *db, uint32_t index) { return index != 0 && get_bit(db, index - 1); }
uint32_t idx_count(const IndexDB *db) { return db->count; }

/* ---------------- tests ---------------- */
#define MODEL 20000
static unsigned char used[MODEL + 2];

static void random_test(void) {
    IndexDB *db = idx_create();
    assert(db);
    srand(12345);
    uint32_t live = 0;
    for (int step = 0; step < 300000; ++step) {
        if (rand() % 100 < 55 || live == 0) {
            uint32_t want = 1;
            while (used[want]) ++want;                    /* brute-force lowest unused */
            if (want > MODEL) continue;
            uint32_t got = idx_alloc(db);
            assert(got == want);
            used[got] = 1; live++;
        } else {
            uint32_t x = (uint32_t)(rand() % MODEL) + 1;
            int expect = used[x];
            assert(idx_free(db, x) == expect);
            if (expect) { used[x] = 0; live--; }
        }
        assert(idx_count(db) == live);
    }
    idx_destroy(db);
}

static void edge_test(void) {
    IndexDB *db = idx_create();
    assert(idx_alloc(db) == 1 && idx_alloc(db) == 2 && idx_alloc(db) == 3);
    assert(idx_free(db, 2) == 1);
    assert(idx_free(db, 2) == 0);          /* double delete */
    assert(idx_free(db, 0) == 0);          /* invalid */
    assert(idx_free(db, 4000000000u) == 0);/* never allocated, page not even present */
    assert(idx_alloc(db) == 2);            /* lowest hole is reused first */
    assert(idx_alloc(db) == 4);
    idx_destroy(db);
}

/* Fills the whole 2^32-1 range: ~512 MB, takes a while. */
static void full_test(void) {
    IndexDB *db = idx_create();
    for (uint64_t i = 1; i <= UINT32_MAX; ++i) {
        uint32_t got = idx_alloc(db);
        if (got != i) { printf("mismatch at %llu got %u\n", (unsigned long long)i, got); abort(); }
    }
    assert(idx_count(db) == UINT32_MAX);
    assert(idx_alloc(db) == 0);            /* full -> rejected */
    assert(idx_free(db, UINT32_MAX) == 1);
    assert(idx_free(db, 1) == 1);
    assert(idx_alloc(db) == 1);            /* lowest first */
    assert(idx_alloc(db) == UINT32_MAX);
    assert(idx_alloc(db) == 0);
    idx_destroy(db);
}

int main(int argc, char **argv) {
    edge_test();
    random_test();
    puts("edge + random tests passed");
    if (argc > 1 && strcmp(argv[1], "full") == 0) { full_test(); puts("full-range test passed"); }
    return 0;
}
