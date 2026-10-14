/* Event-driven FSM + ISR-safe event queue + bit-level UART receiver (8N1, LSB first).
 * Host build/test (simulated ISRs):  gcc -std=c11 -Wall -Wextra -g -fsanitize=address,undefined uart_rx.c -o uart_rx && ./uart_rx
 * On target (Cortex-M): drop the simulation block and provide the hal_* functions. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ------------------------------------------------------------------ FSM (as given) */
typedef void (*fsm_action_t)(void *ctx);

typedef struct {
    fsm_action_t on_entry;     /* may be NULL */
    fsm_action_t on_exit;      /* may be NULL */
} fsm_state_t;

typedef struct {
    int from;
    int event;
    int to;
} fsm_transition_t;

typedef struct {
    const fsm_state_t      *states;
    size_t                  state_count;
    const fsm_transition_t *table;
    size_t                  table_len;
    int                     current;
    void                   *ctx;
} fsm_t;

bool fsm_init(fsm_t *fsm,
              const fsm_state_t *states, size_t state_count,
              const fsm_transition_t *table, size_t table_len,
              int initial, void *ctx)
{
    if (fsm == NULL || states == NULL || state_count == 0) return false;
    if (table == NULL || table_len == 0) return false;
    if (initial < 0 || (size_t)initial >= state_count) return false;
    for (size_t i = 0; i < table_len; i++) {
        if (table[i].from < 0 || (size_t)table[i].from >= state_count ||
            table[i].to   < 0 || (size_t)table[i].to   >= state_count) {
            return false;
        }
    }
    fsm->states      = states;
    fsm->state_count = state_count;
    fsm->table       = table;
    fsm->table_len   = table_len;
    fsm->current     = initial;
    fsm->ctx         = ctx;
    if (states[initial].on_entry != NULL) states[initial].on_entry(ctx);
    return true;
}

bool fsm_dispatch(fsm_t *fsm, int event)
{
    if (fsm == NULL || fsm->states == NULL || fsm->table == NULL) return false;
    for (size_t i = 0; i < fsm->table_len; i++) {
        const fsm_transition_t *t = &fsm->table[i];
        if (t->from != fsm->current || t->event != event) continue;
        const fsm_state_t *from = &fsm->states[t->from];
        const fsm_state_t *to   = &fsm->states[t->to];
        if (from->on_exit != NULL)  from->on_exit(fsm->ctx);
        fsm->current = t->to;
        if (to->on_entry != NULL)   to->on_entry(fsm->ctx);
        return true;
    }
    return false;
}

/* ------------------------------------------------------------------ platform glue */
#if defined(__arm__)
  #define COMPILER_BARRIER()   __asm__ volatile("" ::: "memory")
  #define IRQ_DISABLE()        __asm__ volatile("cpsid i" ::: "memory")
  #define IRQ_ENABLE()         __asm__ volatile("cpsie i" ::: "memory")
  #define WAIT_FOR_INTERRUPT() __asm__ volatile("wfi")
#else   /* host build: no real interrupts */
  #define COMPILER_BARRIER()   __asm__ volatile("" ::: "memory")
  #define IRQ_DISABLE()        ((void)0)
  #define IRQ_ENABLE()         ((void)0)
  #define WAIT_FOR_INTERRUPT() ((void)0)
#endif

/* ------------------------------------------------------------------ event queue (single producer ISR, single consumer loop) */
#define EVQ_SIZE 16u                       /* must be a power of two */

typedef struct {
    int               events[EVQ_SIZE];
    volatile uint32_t head;                /* written ONLY by the ISR (producer)  */
    volatile uint32_t tail;                /* written ONLY by the loop (consumer) */
    volatile uint32_t dropped;             /* events lost because the queue was full (ISR writes) */
} fsm_event_queue_t;

/* head and tail are free-running counters: they only count up and wrap at 2^32.
 * Slot = counter & (EVQ_SIZE-1).  Full: head - tail == EVQ_SIZE.  Empty: head == tail.
 * That way all EVQ_SIZE slots are usable and no "one slot left empty" trick is needed. */
static bool evq_put(fsm_event_queue_t *q, int event)
{
    uint32_t head = q->head;
    if ((uint32_t)(head - q->tail) >= EVQ_SIZE) {      /* 1. full: count the drop */
        q->dropped++;
        return false;
    }
    q->events[head & (EVQ_SIZE - 1u)] = event;         /* 2. write the event       */
    COMPILER_BARRIER();                                /* 3. data must land BEFORE head moves */
    q->head = head + 1u;                               /* 4. publish               */
    return true;
}

static bool evq_get(fsm_event_queue_t *q, int *event)
{
    uint32_t tail = q->tail;
    if (q->head == tail) return false;                 /* 1. empty                 */
    *event = q->events[tail & (EVQ_SIZE - 1u)];        /* 2. read the event        */
    COMPILER_BARRIER();                                /* 3. finish reading BEFORE the slot is given back */
    q->tail = tail + 1u;                               /* free the slot            */
    return true;
}

static fsm_event_queue_t evq;                          /* events for the FSM          */
static fsm_event_queue_t rxq;                          /* received bytes (payload)    */

/* Producer: called from an ISR. Returns false if the queue is full. */
bool fsm_post_from_isr(int event) { return evq_put(&evq, event); }

/* Consumer: called only from the dispatch loop. Returns false if the queue is empty. */
static bool fsm_queue_get(int *event) { return evq_get(&evq, event); }

/* Drain every queued event, each one dispatched to completion before the next. */
void fsm_drain(fsm_t *fsm)
{
    int ev;
    while (fsm_queue_get(&ev)) fsm_dispatch(fsm, ev);
}

/* The dispatch loop: never returns. */
void fsm_run(fsm_t *fsm)
{
    for (;;) {
        fsm_drain(fsm);

        /* Race-free sleep: with interrupts masked, an ISR cannot slip in between the
         * emptiness check and WFI.  If one fires meanwhile it stays pending, and WFI
         * returns immediately when an interrupt is pending, even while masked.
         * Then we unmask and the ISR runs; the next loop pass finds its event. */
        IRQ_DISABLE();
        if (evq.head == evq.tail) WAIT_FOR_INTERRUPT();
        IRQ_ENABLE();
    }
}

/* ------------------------------------------------------------------ UART receiver (ISR side) */
/* HAL: implement these for the real chip (edge interrupt on the RX pin + one timer). */
extern int  hal_rx_read(void);                      /* current level of the RX pin: 1 = idle/mark */
extern void hal_edge_irq_disable(void);
extern void hal_edge_irq_enable(void);              /* must clear any stale pending flag first    */
extern void hal_timer_start(uint32_t first, uint32_t period);   /* first expiry, then every period */
extern void hal_timer_stop(void);

#define BIT_TICKS 100u        /* timer ticks per bit = timer_clock / baud (use a fast timer: 16 MHz / 115200 = 138.9) */

enum { EV_BYTE = 1, EV_FRAME_ERR = 2 };

typedef enum { RX_IDLE, RX_START, RX_DATA, RX_STOP } rx_phase_t;
static struct { rx_phase_t phase; uint8_t shift; uint8_t nbits; } rx;

static void rx_finish(void)                          /* back to waiting for the next start bit */
{
    hal_timer_stop();
    rx.phase = RX_IDLE;
    hal_edge_irq_enable();
}

/* ISR 1: falling edge on RX = possible start bit. */
void uart_edge_isr(void)
{
    hal_edge_irq_disable();                          /* no more edges while the frame is in progress */
    rx.phase = RX_START; rx.shift = 0; rx.nbits = 0;
    hal_timer_start(BIT_TICKS / 2u, BIT_TICKS);      /* first sample in the MIDDLE of the start bit, then every bit */
}

/* ISR 2: timer, fires in the middle of each bit. */
void uart_timer_isr(void)
{
    int level = hal_rx_read();
    switch (rx.phase) {
    case RX_START:
        if (level) { rx_finish(); return; }          /* line already high: it was a glitch, ignore it */
        rx.phase = RX_DATA;
        break;
    case RX_DATA:
        rx.shift = (uint8_t)((rx.shift >> 1) | (level ? 0x80u : 0u));   /* LSB arrives first */
        if (++rx.nbits == 8u) rx.phase = RX_STOP;
        break;
    case RX_STOP:
        if (level) {                                 /* valid stop bit = high */
            if (evq_put(&rxq, rx.shift)) fsm_post_from_isr(EV_BYTE);
        } else {
            fsm_post_from_isr(EV_FRAME_ERR);         /* stop bit low: framing error (or break) */
        }
        rx_finish();                                 /* we are mid stop bit, line is high: a new start edge still works */
        break;
    default:
        rx_finish();
        break;
    }
}

/* ------------------------------------------------------------------ application FSM */
enum { ST_IDLE, ST_RECEIVING, ST_ERROR, ST_COUNT };

typedef struct { uint8_t buf[64]; size_t len; uint32_t errors; } app_t;

static void on_enter_receiving(void *ctx)           /* move every received byte into the app buffer */
{
    app_t *a = ctx;
    int b;
    while (evq_get(&rxq, &b) && a->len < sizeof a->buf) a->buf[a->len++] = (uint8_t)b;
}
static void on_enter_error(void *ctx) { ((app_t *)ctx)->errors++; }

static const fsm_state_t states[ST_COUNT] = {
    [ST_IDLE]      = { NULL, NULL },
    [ST_RECEIVING] = { on_enter_receiving, NULL },
    [ST_ERROR]     = { on_enter_error, NULL },
};
static const fsm_transition_t table[] = {
    { ST_IDLE,      EV_BYTE,      ST_RECEIVING },
    { ST_RECEIVING, EV_BYTE,      ST_RECEIVING },   /* self-transition: entry action runs again */
    { ST_ERROR,     EV_BYTE,      ST_RECEIVING },
    { ST_IDLE,      EV_FRAME_ERR, ST_ERROR },
    { ST_RECEIVING, EV_FRAME_ERR, ST_ERROR },
    { ST_ERROR,     EV_FRAME_ERR, ST_ERROR },
};

/* ------------------------------------------------------------------ host simulation + tests */
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int  sim_line = 1, sim_last = 1, sim_edge_en = 1, sim_run, sim_cnt, sim_period;
int  hal_rx_read(void) { return sim_line; }
void hal_edge_irq_disable(void) { sim_edge_en = 0; }
void hal_edge_irq_enable(void)  { sim_edge_en = 1; }
void hal_timer_start(uint32_t first, uint32_t period) { sim_run = 1; sim_cnt = (int)first; sim_period = (int)period; }
void hal_timer_stop(void) { sim_run = 0; }

static uint8_t wave[100000]; static size_t wlen;     /* line level per timer tick */
static void put(int level, int ticks) { while (ticks-- > 0) wave[wlen++] = (uint8_t)level; }
/* one 8N1 frame; bit length is BIT_TICKS scaled by clock error in percent (sender's clock is off) */
static void put_frame(uint8_t byte, int stop_level, int err_pct)
{
    int bit = (int)BIT_TICKS * (100 + err_pct) / 100;
    put(0, bit);
    for (int i = 0; i < 8; i++) put((byte >> i) & 1, bit);
    put(stop_level, bit);
}

static void reset_sim(void)
{
    memset(&evq, 0, sizeof evq); memset(&rxq, 0, sizeof rxq); memset(&rx, 0, sizeof rx);
    sim_line = sim_last = 1; sim_edge_en = 1; sim_run = 0; wlen = 0;
}
static void run_sim(fsm_t *f)                        /* one loop pass per tick = the ISRs interleave with the loop */
{
    for (size_t t = 0; t < wlen; t++) {
        sim_line = wave[t];
        if (sim_edge_en && sim_last == 1 && sim_line == 0) uart_edge_isr();
        sim_last = sim_line;
        if (sim_run && --sim_cnt == 0) { sim_cnt = sim_period; uart_timer_isr(); }
        fsm_drain(f);
    }
}

static app_t app;
static fsm_t fsm;
static void setup(void)
{
    reset_sim(); memset(&app, 0, sizeof app);
    assert(fsm_init(&fsm, states, ST_COUNT, table, sizeof table / sizeof table[0], ST_IDLE, &app));
}

int main(void)
{
    /* 1. back-to-back frames (no idle gap), including 0x00 and 0xFF */
    setup();
    const uint8_t msg[] = { 0x55, 0xA3, 0x00, 0xFF, 0x01 };
    put(1, 300);
    for (size_t i = 0; i < sizeof msg; i++) put_frame(msg[i], 1, 0);
    put(1, 300);
    run_sim(&fsm);
    assert(app.len == sizeof msg && memcmp(app.buf, msg, sizeof msg) == 0 && app.errors == 0);
    puts("1 back-to-back bytes: ok");

    /* 2. sender clock 3% off still works; 7% off breaks (limit is about +-5%) */
    setup(); put(1, 300);
    for (size_t i = 0; i < sizeof msg; i++) put_frame(msg[i], 1, 3);
    put(1, 300); run_sim(&fsm);
    assert(app.len == sizeof msg && memcmp(app.buf, msg, sizeof msg) == 0);
    setup(); put(1, 300);
    for (size_t i = 0; i < sizeof msg; i++) put_frame(msg[i], 1, 7);
    put(1, 300); run_sim(&fsm);
    assert(!(app.len == sizeof msg && memcmp(app.buf, msg, sizeof msg) == 0));
    puts("2 clock tolerance: 3% ok, 7% fails as expected");

    /* 3. short glitch (20 ticks low, less than half a bit) is ignored, next real byte still received */
    setup(); put(1, 300); put(0, 20); put(1, 300); put_frame(0x42, 1, 0); put(1, 300);
    run_sim(&fsm);
    assert(app.len == 1 && app.buf[0] == 0x42 && app.errors == 0);
    puts("3 glitch rejected: ok");

    /* 4. framing error: stop bit low -> error event, no byte; the line later recovers */
    setup(); put(1, 300); put_frame(0x11, 0, 0); put(1, 300); put_frame(0x22, 1, 0); put(1, 300);
    run_sim(&fsm);
    assert(app.errors == 1 && app.len == 1 && app.buf[0] == 0x22);
    puts("4 framing error then recovery: ok");

    /* 5. queue: wrap-around and overflow */
    reset_sim();
    int v;
    for (int i = 0; i < 100; i++) { assert(fsm_post_from_isr(i)); assert(fsm_queue_get(&v) && v == i); }
    for (int i = 0; i < 20; i++) fsm_post_from_isr(i);
    assert(evq.dropped == 4);                                   /* 16 fit, 4 lost */
    for (int i = 0; i < 16; i++) assert(fsm_queue_get(&v) && v == i);
    assert(!fsm_queue_get(&v));
    puts("5 queue wrap + overflow: ok");
    return 0;
}
