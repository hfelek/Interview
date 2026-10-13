/* Tracks which chunks of a file have been received. One bit per chunk.
 * Build: gcc -std=c11 -Wall -Wextra -g -fsanitize=address,undefined chunk_tracker.c -o chunk_tracker */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    uint8_t *bits;       /* bit i = chunk i received */
    uint32_t total;      /* number of chunks in the file */
    uint32_t received;   /* how many distinct chunks we have */
    uint32_t first_missing;  /* every chunk below this one is received (scan starts here) */
} ChunkTracker;

int ct_init(ChunkTracker *t, uint32_t total_chunks) {
    t->bits = calloc((total_chunks + 7) / 8 + 1, 1);
    if (!t->bits) return -1;
    t->total = total_chunks;
    t->received = 0;
    t->first_missing = 0;
    return 0;
}

void ct_free(ChunkTracker *t) { free(t->bits); t->bits = NULL; }

/* Returns 1 if the chunk is new, 0 if it was a duplicate, -1 if the index is out of range. */
int ct_mark(ChunkTracker *t, uint32_t i) {
    if (i >= t->total) return -1;
    if (t->bits[i / 8] & (1u << (i % 8))) return 0;   /* duplicate: don't count twice */
    t->bits[i / 8] |= (uint8_t)(1u << (i % 8));
    t->received++;
    return 1;
}

int ct_complete(const ChunkTracker *t) { return t->received == t->total; }

/* Lowest missing chunk, or -1 if the file is complete. */
int64_t ct_next_missing(ChunkTracker *t) {
    while (t->first_missing < t->total &&
           (t->bits[t->first_missing / 8] & (1u << (t->first_missing % 8))))
        t->first_missing++;                            /* skip received chunks, remember where we got to */
    return t->first_missing < t->total ? (int64_t)t->first_missing : -1;
}

int main(void) {
    ChunkTracker t;
    ct_init(&t, 5);
    ct_mark(&t, 0); ct_mark(&t, 1); ct_mark(&t, 3);
    int dup = ct_mark(&t, 3), bad = ct_mark(&t, 9);
    printf("dup=%d out_of_range=%d complete=%d next_missing=%lld\n",
           dup, bad, ct_complete(&t), (long long)ct_next_missing(&t));   /* 0 -1 0 2 */
    ct_mark(&t, 2); ct_mark(&t, 4);
    printf("complete=%d next_missing=%lld\n", ct_complete(&t), (long long)ct_next_missing(&t)); /* 1 -1 */
    ct_free(&t);
    return 0;
}
