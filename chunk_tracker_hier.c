/* Simple chunk tracker with two bitmaps.
 * bits[]    : bit i = chunk i received.
 * summary[] : bit w = word w of bits[] is completely full, so next_missing can skip it.
 * Build: gcc -std=c11 -Wall -Wextra -g -fsanitize=address,undefined chunk_tracker_hier.c -o chunk_tracker_hier */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define FULL (~(uint64_t)0)

typedef struct {
    uint64_t *bits;
    uint64_t *summary;
    uint64_t  nwords;       /* words in bits[] */
    uint64_t  nsummary;     /* words in summary[] */
    uint64_t  total;        /* chunks in the file */
    uint64_t  received;     /* distinct chunks received */
} ChunkTracker;

int ct_init(ChunkTracker *t, uint64_t total) {
    t->total = total;
    t->received = 0;
    t->nwords = (total + 63) / 64;
    t->nsummary = (t->nwords + 63) / 64;
    t->bits = calloc(t->nwords + 1, sizeof(uint64_t));         /* +1 so size is never 0 */
    t->summary = calloc(t->nsummary + 1, sizeof(uint64_t));
    if (!t->bits || !t->summary) { free(t->bits); free(t->summary); return -1; }
    return 0;
}

void ct_free(ChunkTracker *t) { free(t->bits); free(t->summary); }

/* Returns 1 if the chunk is new, 0 if duplicate, -1 if out of range. */
int ct_mark(ChunkTracker *t, uint64_t i) {
    if (i >= t->total) return -1;
    uint64_t w = i / 64, mask = (uint64_t)1 << (i % 64);
    if (t->bits[w] & mask) return 0;
    t->bits[w] |= mask;
    t->received++;
    if (t->bits[w] == FULL)                                     /* word filled up: record it in summary */
        t->summary[w / 64] |= (uint64_t)1 << (w % 64);
    return 1;
}

int ct_complete(const ChunkTracker *t) { return t->received == t->total; }

/* Lowest missing chunk, or -1 if complete. */
int64_t ct_next_missing(const ChunkTracker *t) {
    if (ct_complete(t)) return -1;
    for (uint64_t s = 0; s < t->nsummary; s++) {
        if (t->summary[s] == FULL) continue;                    /* 64 full words, skip 4096 chunks */
        uint64_t w = s * 64 + (uint64_t)__builtin_ctzll(~t->summary[s]);   /* first word that is not full */
        return (int64_t)(w * 64 + (uint64_t)__builtin_ctzll(~t->bits[w])); /* first zero bit in it */
    }
    return -1;   /* not reached when not complete */
}

int main(void) {
    ChunkTracker t;
    ct_init(&t, 5);
    ct_mark(&t, 0); ct_mark(&t, 1); ct_mark(&t, 3);
    int dup = ct_mark(&t, 3), bad = ct_mark(&t, 9);
    printf("dup=%d out_of_range=%d complete=%d next_missing=%lld\n",
           dup, bad, ct_complete(&t), (long long)ct_next_missing(&t));      /* 0 -1 0 2 */
    ct_mark(&t, 2); ct_mark(&t, 4);
    printf("complete=%d next_missing=%lld\n", ct_complete(&t), (long long)ct_next_missing(&t)); /* 1 -1 */
    ct_free(&t);

    /* random order vs. brute force, sizes around the 64 / 4096 boundaries */
    uint64_t sizes[] = {0, 1, 63, 64, 65, 4095, 4096, 4097, 9000};
    for (size_t k = 0; k < sizeof sizes / sizeof *sizes; k++) {
        uint64_t n = sizes[k];
        uint8_t *ref = calloc(n + 1, 1);
        ct_init(&t, n);
        for (uint64_t step = 0; step < n; step++) {
            uint64_t i = (uint64_t)rand() % n;
            ct_mark(&t, i);
            ref[i] = 1;
            int64_t want = -1;
            for (uint64_t j = 0; j < n; j++) if (!ref[j]) { want = (int64_t)j; break; }
            if (ct_next_missing(&t) != want) { printf("FAIL n=%llu\n", (unsigned long long)n); return 1; }
        }
        for (uint64_t i = 0; i < n; i++) ct_mark(&t, i);
        if (!ct_complete(&t) || ct_next_missing(&t) != -1) { printf("FAIL complete n=%llu\n", (unsigned long long)n); return 1; }
        ct_free(&t);
        free(ref);
    }
    printf("random tests passed\n");
    return 0;
}
