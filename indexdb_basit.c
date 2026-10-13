/* Simple version: one bit per index + a "hint" for the lowest index that might be free.
 * Indices 1 .. UINT32_MAX.  Build: gcc -std=c11 -O2 -Wall -Wextra indexdb_basit.c -o indexdb_basit */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define NBITS  ((uint64_t)1 << 32)            /* bit v = index v; bit 0 is never used */
#define NWORDS (NBITS / 64)

typedef struct {
    uint64_t *bits;     /* 512 MB; calloc'd, so the OS only commits pages that get touched */
    uint32_t hint;      /* every index below hint is in use */
} IndexDB;

IndexDB *idx_create(void) {
    IndexDB *db = malloc(sizeof *db);
    if (!db) return NULL;
    db->bits = calloc(NWORDS, sizeof(uint64_t));
    if (!db->bits) { free(db); return NULL; }
    db->bits[0] = 1;                           /* index 0 is reserved */
    db->hint = 1;
    return db;
}

void idx_destroy(IndexDB *db) { if (db) { free(db->bits); free(db); } }

/* Lowest unused index, or 0 if everything is taken. */
uint32_t idx_alloc(IndexDB *db) {
    for (uint64_t w = db->hint / 64; w < NWORDS; ++w) {
        if (db->bits[w] == ~(uint64_t)0) continue;                 /* word full, skip 64 at once */
        uint32_t idx = (uint32_t)(w * 64 + __builtin_ctzll(~db->bits[w]));
        db->bits[w] |= (uint64_t)1 << (idx & 63);
        db->hint = idx + 1;                                         /* idx == UINT32_MAX wraps to 0 (= full) */
        return idx;
    }
    return 0;
}

/* 1 on success, 0 if the index is 0 or not in use. */
int idx_free(IndexDB *db, uint32_t idx) {
    if (idx == 0 || !((db->bits[idx / 64] >> (idx & 63)) & 1)) return 0;
    db->bits[idx / 64] &= ~((uint64_t)1 << (idx & 63));
    if (idx < db->hint) db->hint = idx;                             /* a lower hole appeared */
    return 1;
}

int main(void) {
    IndexDB *db = idx_create();
    uint32_t a = idx_alloc(db), b = idx_alloc(db), c = idx_alloc(db);
    printf("%u %u %u\n", a, b, c);                                      /* 1 2 3 */
    int f1 = idx_free(db, 2), f2 = idx_free(db, 2), f3 = idx_free(db, 0);
    printf("%d %d %d\n", f1, f2, f3);                                   /* 1 0 0 */
    a = idx_alloc(db); b = idx_alloc(db);
    printf("%u %u\n", a, b);                                            /* 2 4 */
    idx_destroy(db);
    return 0;
}
