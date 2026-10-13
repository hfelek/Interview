/* Chunk tracker with a multi-level bitmap so next_missing is O(log64 N).
 * Level 0: bit i = chunk i received.
 * Level k: bit j = "word j of level k-1 is completely full".
 * Unused bits past the end of every level are set to 1, so they never look "missing".
 * Build: gcc -std=c11 -Wall -Wextra -g -fsanitize=address,undefined chunk_tracker_hier.c -o chunk_tracker_hier */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_LEVELS 11                 /* 64^11 > 2^64, enough for any uint64_t count */
#define FULL (~(uint64_t)0)

typedef struct {
    uint64_t *level[MAX_LEVELS];      /* level[0] is the real bitmap, level[top] is a single word */
    int       top;                    /* index of the highest level */
    uint64_t  total;                  /* number of chunks in the file */
    uint64_t  received;               /* distinct chunks received so far */
} ChunkTracker;

static void set_bit(uint64_t *words, uint64_t i) { words[i / 64] |= (uint64_t)1 << (i % 64); }

/* Allocates one level with `nbits` meaningful bits; the padding bits are set to 1. */
static uint64_t *make_level(uint64_t nbits, uint64_t *nwords) {
    uint64_t n = (nbits + 63) / 64;
    if (n == 0) n = 1;                              /* empty file still gets one (all-padding) word */
    uint64_t *w = calloc(n, sizeof(uint64_t));
    if (!w) return NULL;
    for (uint64_t b = nbits; b < n * 64; b++) set_bit(w, b);
    *nwords = n;
    return w;
}

void ct_free(ChunkTracker *t) {
    for (int l = 0; l < MAX_LEVELS; l++) { free(t->level[l]); t->level[l] = NULL; }
}

int ct_init(ChunkTracker *t, uint64_t total_chunks) {
    t->total = total_chunks;
    t->received = 0;
    t->top = 0;
    for (int l = 0; l < MAX_LEVELS; l++) t->level[l] = NULL;

    uint64_t nbits = total_chunks, nwords = 0;
    for (int l = 0; l < MAX_LEVELS; l++) {
        t->level[l] = make_level(nbits, &nwords);
        if (!t->level[l]) { ct_free(t); return -1; }
        t->top = l;
        if (nwords == 1) break;                     /* single word = top of the tree */
        nbits = nwords;                             /* next level has one bit per word here */
    }
    return 0;
}

/* Called after level[l] word w changed: if it just became full, tell the level above. */
static void propagate_full(ChunkTracker *t, int l, uint64_t w) {
    while (l < t->top && t->level[l][w] == FULL) {
        set_bit(t->level[l + 1], w);
        w /= 64;
        l++;
    }
}

/* Returns 1 if the chunk is new, 0 if it was a duplicate, -1 if the index is out of range. */
int ct_mark(ChunkTracker *t, uint64_t i) {
    if (i >= t->total) return -1;
    uint64_t mask = (uint64_t)1 << (i % 64);
    if (t->level[0][i / 64] & mask) return 0;       /* duplicate: don't count twice */
    t->level[0][i / 64] |= mask;
    t->received++;
    propagate_full(t, 0, i / 64);
    return 1;
}

int ct_is_received(const ChunkTracker *t, uint64_t i) {
    return i < t->total && ((t->level[0][i / 64] >> (i % 64)) & 1);
}

int ct_complete(const ChunkTracker *t) { return t->received == t->total; }

/* Lowest missing chunk, or -1 if the file is complete. Walks from the top level down. */
int64_t ct_next_missing(const ChunkTracker *t) {
    if (ct_complete(t)) return -1;
    uint64_t idx = 0;                               /* word index at the current level */
    for (int l = t->top; l >= 0; l--) {
        uint64_t holes = ~t->level[l][idx];         /* 1 bits = not full / not received */
        idx = idx * 64 + (uint64_t)__builtin_ctzll(holes);
    }
    return (int64_t)idx;
}

/* ---- tests ---- */

static void check(int ok, const char *what) {
    if (!ok) { printf("FAIL: %s\n", what); exit(1); }
}

static void test_basic(void) {
    ChunkTracker t;
    ct_init(&t, 5);
    ct_mark(&t, 0); ct_mark(&t, 1); ct_mark(&t, 3);
    check(ct_mark(&t, 3) == 0, "duplicate returns 0");
    check(ct_mark(&t, 9) == -1, "out of range returns -1");
    check(!ct_complete(&t), "not complete yet");
    check(ct_next_missing(&t) == 2, "next missing is 2");
    ct_mark(&t, 2); ct_mark(&t, 4);
    check(ct_complete(&t), "complete");
    check(ct_next_missing(&t) == -1, "nothing missing");
    ct_free(&t);
}

static void test_empty(void) {
    ChunkTracker t;
    ct_init(&t, 0);
    check(ct_complete(&t), "empty file is complete");
    check(ct_next_missing(&t) == -1, "empty file has no missing chunk");
    ct_free(&t);
}

/* Compares against a plain byte-per-chunk scan, marking in random order. */
static void test_random(uint64_t total) {
    ChunkTracker t;
    ct_init(&t, total);
    uint8_t *ref = calloc(total ? total : 1, 1);
    for (uint64_t n = 0; n < total; n++) {
        uint64_t i = (uint64_t)rand() % total;
        ct_mark(&t, i);
        ref[i] = 1;
        int64_t want = -1;
        for (uint64_t j = 0; j < total; j++) if (!ref[j]) { want = (int64_t)j; break; }
        check(ct_next_missing(&t) == want, "next_missing matches brute force");
    }
    for (uint64_t i = 0; i < total; i++) ct_mark(&t, i);   /* fill the rest */
    check(ct_complete(&t) && ct_next_missing(&t) == -1, "complete after filling");
    free(ref);
    ct_free(&t);
}

int main(void) {
    test_basic();
    test_empty();
    uint64_t sizes[] = {1, 63, 64, 65, 4095, 4096, 4097, 5000};   /* around the 64 / 64^2 boundaries */
    for (size_t k = 0; k < sizeof sizes / sizeof *sizes; k++) test_random(sizes[k]);
    printf("all tests passed\n");
    return 0;
}
