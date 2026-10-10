/* Parking lot in C: ticket on arrival if a fitting slot is free, reject otherwise,
 * free the slot on departure.
 * Build: gcc -std=c11 -Wall -Wextra -g -fsanitize=address,undefined otopark.c -o otopark_c */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

enum { SMALL = 0, MEDIUM = 1, LARGE = 2, NSIZES = 3 };

typedef struct {
    int total;            /* number of slots */
    int *slot_size;       /* [slot] size of the slot */
    int *gen;             /* [slot] how many times it has been used (for ticket numbers) */
    char *busy;           /* [slot] 1 if a car is parked */
    int *stack[NSIZES];   /* free slot ids per size (stack) */
    int top[NSIZES];      /* stack heights */
} ParkingLot;

int lot_init(ParkingLot *p, const int counts[NSIZES]) {
    p->total = counts[0] + counts[1] + counts[2];
    p->slot_size = malloc(p->total * sizeof *p->slot_size);
    p->gen = calloc(p->total, sizeof *p->gen);
    p->busy = calloc(p->total, 1);
    if (!p->slot_size || !p->gen || !p->busy) return -1;
    int id = 0;
    for (int s = 0; s < NSIZES; ++s) {
        p->stack[s] = malloc((counts[s] ? counts[s] : 1) * sizeof(int));
        if (!p->stack[s]) return -1;
        p->top[s] = 0;
        for (int i = 0; i < counts[s]; ++i) {
            p->slot_size[id] = s;
            p->stack[s][p->top[s]++] = id++;
        }
    }
    return 0;
}

void lot_free(ParkingLot *p) {
    free(p->slot_size); free(p->gen); free(p->busy);
    for (int s = 0; s < NSIZES; ++s) free(p->stack[s]);
}

/* Returns ticket number (> 0), or 0 if rejected.
 * ticket = slot + total * gen[slot]: tells us the slot and is never repeated,
 * because gen grows every time the slot is used. */
long lot_arrive(ParkingLot *p, int car_size) {
    for (int s = car_size; s < NSIZES; ++s) {       /* smallest fitting size first */
        if (p->top[s] == 0) continue;
        int slot = p->stack[s][--p->top[s]];
        p->busy[slot] = 1;
        p->gen[slot]++;                              /* first use -> gen 1 -> ticket > 0 */
        return (long)slot + (long)p->total * p->gen[slot];
    }
    return 0;
}

/* Returns 1 on success, 0 for an unknown / already used ticket. */
int lot_depart(ParkingLot *p, long ticket) {
    if (ticket <= 0) return 0;
    int slot = (int)(ticket % p->total);
    long gen = ticket / p->total;
    if (!p->busy[slot] || gen != p->gen[slot]) return 0;   /* old or forged ticket */
    p->busy[slot] = 0;
    int s = p->slot_size[slot];
    p->stack[s][p->top[s]++] = slot;                 /* slot is vacant again */
    return 1;
}

int main(void) {
    int counts[NSIZES] = {1, 1, 1};
    ParkingLot lot;
    assert(lot_init(&lot, counts) == 0);

    long t1 = lot_arrive(&lot, SMALL);   /* small slot  */
    long t2 = lot_arrive(&lot, SMALL);   /* medium slot */
    long t3 = lot_arrive(&lot, MEDIUM);  /* large slot  */
    long t4 = lot_arrive(&lot, SMALL);   /* full -> rejected */
    assert(t1 > 0 && t2 > 0 && t3 > 0 && t4 == 0);

    assert(lot_arrive(&lot, LARGE) == 0);
    assert(lot_depart(&lot, t3) == 1);
    assert(lot_depart(&lot, t3) == 0);   /* same ticket twice */
    assert(lot_depart(&lot, 999) == 0);  /* unknown ticket */
    long t5 = lot_arrive(&lot, LARGE);
    assert(t5 > 0 && t5 != t3);          /* same slot, new ticket number */
    assert(lot_depart(&lot, t3) == 0);   /* old ticket stays invalid */
    assert(lot_arrive(&lot, MEDIUM) == 0);

    lot_free(&lot);
    puts("tum testler gecti");
    return 0;
}
