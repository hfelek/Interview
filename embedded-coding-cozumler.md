# Embedded coding mülakatı: 12 sorunun çözümü ve kısa mantığı

Her çözümü derleyip kontrol ettim (kenar durumlar dahil). Derleyici olmayan mülakatta bunları **kâğıda aynen yazabilmen** için kodları kısa tuttum. Yaklaşım her soruda aynı:

1. Sorunun **ne istediğini** kendi cümlenle tekrarla, belirsizlikleri sor (sınır dahil mi, kapasite, tek iş parçacığı mı).
2. **Basit çözümü** söyle, sonra neden yetmediğini ve daha iyisini.
3. Kodu yaz, **kenar durumları** (boş, dolu, taşma, sarma) sesli say.
4. Karmaşıklığı söyle.

| # | Konu | Ana fikir |
|---|---|---|
| 1 | Bit işlemleri | Maske ile alanı temizle, sonra yerleştir |
| 2 | Buton debounce | İlk kenarda uyan, 20 ms sonra bir kez bak |
| 3 | Bellek havuzu | Boş bloklar kendi içinde bağlı liste |
| 4 | find kütüphanesi | Ortak arayüz + AND/OR/NOT birleştiriciler |
| 5 | Paket ayrıştırıcı | Durum makinesi + bayt doldurma + CRC |
| 6 | Yazılım zamanlayıcı | Tek tick, işaretli fark ile sarma güvenliği |
| 7 | Sırasız paket birleştirme | Pencere + 16 bit sarma farkı |
| 8 | Aralık birleştirme | Sırala, tek geçişte tara |
| 9 | DMA çift tampon | Yarım/tam kesmesi + "hâlâ işleniyor" bayrağı |
| 10 | atoi / itoa | Negatif biriktir, `INT_MIN` kenar durumu |
| 11 | CRC-8 / CRC-16 | Polinom bölümünün kalanı, tablo ile hız |
| 12 | Endianness ve yapı | Yapıyı ham gönderme, bayt bayt serileştir |

---

## 1. Bit işlemleri

**Mantık**
Alanı değiştirmek için üç adım: alan genişliğinde maske üret, `reg & ~mask` ile alanı sıfırla, yeni değeri kaydırıp `& mask` ile kes ve OR'la. Kesmek (`& mask`) değer alana sığmasa bile komşu alanı bozmaz. `x & (x-1)` en düşük 1 bitini siler: set bit sayımı ve ikinin kuvveti kontrolü buna dayanır.

```c
#include <stdint.h>
#include <stdbool.h>
static inline uint32_t field_mask(unsigned width) { return (width >= 32u) ? 0xFFFFFFFFu : ((1u << width) - 1u); }
uint32_t set_field(uint32_t reg, unsigned shift, unsigned width, uint32_t value)
{ uint32_t mask = field_mask(width) << shift; return (reg & ~mask) | ((value << shift) & mask); }
uint32_t get_field(uint32_t reg, unsigned shift, unsigned width) { return (reg >> shift) & field_mask(width); }
unsigned count_set_bits(uint32_t x) { unsigned n = 0; while (x) { x &= x - 1u; n++; } return n; }
bool is_power_of_two(uint32_t x) { return x != 0u && (x & (x - 1u)) == 0u; }
```

**Dikkat edilecekler**
- `1u << 32` tanımsız davranıştır, genişlik 32 için maskeyi ayrı hesapla.
- İşaretsiz tip (`1u`, `uint32_t`) kullan, işaretli kaydırma tehlikelidir.
- Gerçek register'da okuma-değiştirme-yazma ISR ile yarışabilir (kesmeyi kapat ya da set/clear register kullan).
- `is_power_of_two(0)` yanlış olmalı, `x != 0` kontrolü şart.

**Mülakatta söyle:** I clear the field with the inverted mask, then OR in the new value masked to the field width, and I special-case width 32 to avoid undefined behaviour.

---

## 2. Buton debounce

**Mantık**
Mekanik buton basılınca 5–20 ms titrer, her titreme bir kesme üretir. Çözüm: ilk kenar kesmesinde kenar kesmesini kapat ve 20 ms'lik tek atımlık zamanlayıcı başlat. Süre dolunca pini **bir kez** oku, son bildirdiğin durumdan farklıysa olayı yayınla, kesmeyi yeniden aç. Sürekli yoklama yok, işlemci uyuyabilir.

```c
#include <stdbool.h>
#include <stdint.h>

#define DEBOUNCE_MS 20u                   /* bounce lasts 5-20 ms; wait slightly longer */

typedef enum { BUTTON_PRESSED = 1, BUTTON_RELEASED = 2 } button_event_t;

/* Hardware layer (platform specific). Button wired to a pull-up: pressed = pin low. */
extern bool hal_button_is_pressed(void);
extern void hal_button_irq_disable(void);
extern void hal_button_irq_enable(void);                /* clears a stale pending flag first */
extern void hal_oneshot_timer_start_ms(uint32_t ms);
extern bool post_event_from_isr(int event);             /* e.g. the event queue from the FSM */

static bool stable_pressed;               /* last state we reported, not the raw pin */

void button_init(void)
{
    stable_pressed = hal_button_is_pressed();
    hal_button_irq_enable();               /* interrupt on BOTH edges */
}

/* ISR 1: any edge on the button pin. The first edge of a bounce burst. */
void button_edge_isr(void)
{
    hal_button_irq_disable();              /* ignore the rest of the bounces */
    hal_oneshot_timer_start_ms(DEBOUNCE_MS);
}

/* ISR 2: timer expired, the contact has settled. Look at the pin once. */
void button_timer_isr(void)
{
    bool pressed = hal_button_is_pressed();
    if (pressed != stable_pressed) {       /* real change; same level = it was just a glitch */
        stable_pressed = pressed;
        post_event_from_isr(pressed ? BUTTON_PRESSED : BUTTON_RELEASED);
    }
    hal_button_irq_enable();               /* listen for the next edge */
}
```

**Dikkat edilecekler**
- İki yönlü kenar tetikle (basma ve bırakma).
- Kesmeyi açarken bekleyen bayrağı temizle, yoksa sahte kesme gelir.
- Gecikme yaklaşık 20 ms. Hızlı tepki gerekirse "ilk kenarda bildir, sonra 20 ms yok say" kullan.
- Aynı değeri iki kez yayınlamamak için ham pin yerine `stable_pressed` tut.

**Mülakatta söyle:** I wake on the first edge, mask it, start a one-shot timer a bit longer than the bounce time, and sample the pin once. A glitch gives the same level as before, so no event.

---

## 3. Sabit boyutlu bellek havuzu

**Mantık**
Bloklar zaten boşken içleri kullanılmıyor. Bu yüzden boş listenin "sonraki" işaretçisini **boş bloğun kendi içine** koyarız, ek bellek harcamayız. `alloc` listenin başını alır, `free` başına ekler: ikisi de O(1). Blok boyutu en az bir işaretçi kadar ve hizalı olmalı.

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct pool_node { struct pool_node *next; } pool_node_t;

typedef struct {
    uint8_t     *base;          /* start of the storage */
    size_t       block_size;
    size_t       block_count;
    pool_node_t *free_list;     /* linked list that lives INSIDE the free blocks */
    size_t       free_count;
} pool_t;

bool pool_init(pool_t *p, void *storage, size_t block_size, size_t block_count)
{
    if (p == NULL || storage == NULL || block_count == 0) return false;
    if (block_size < sizeof(pool_node_t)) return false;                 /* a free block must hold a pointer */
    if (block_size % _Alignof(pool_node_t) != 0) return false;          /* every block stays aligned */
    if ((uintptr_t)storage % _Alignof(pool_node_t) != 0) return false;

    p->base = storage;
    p->block_size = block_size;
    p->block_count = block_count;
    p->free_list = NULL;
    for (size_t i = block_count; i > 0; i--) {                          /* chain all blocks, first block on top */
        pool_node_t *node = (pool_node_t *)(p->base + (i - 1) * block_size);
        node->next = p->free_list;
        p->free_list = node;
    }
    p->free_count = block_count;
    return true;
}

void *pool_alloc(pool_t *p)                                             /* O(1): pop the head */
{
    pool_node_t *node = p->free_list;
    if (node == NULL) return NULL;
    p->free_list = node->next;
    p->free_count--;
    return node;
}

bool pool_free(pool_t *p, void *block)                                  /* O(1): push on the head */
{
    uintptr_t addr  = (uintptr_t)block;
    uintptr_t start = (uintptr_t)p->base;
    if (addr < start || addr >= start + p->block_size * p->block_count) return false;   /* not from this pool */
    if ((addr - start) % p->block_size != 0) return false;                              /* not a block start */
    if (p->free_count == p->block_count) return false;                                  /* more frees than allocs */
#ifdef POOL_CHECK_DOUBLE_FREE                                           /* debug only: O(n) walk */
    for (pool_node_t *n = p->free_list; n != NULL; n = n->next)
        if ((void *)n == block) return false;
#endif
    pool_node_t *node = block;
    node->next = p->free_list;
    p->free_list = node;
    p->free_count++;
    return true;
}
```

**Dikkat edilecekler**
- `free` içinde kontroller: havuz aralığında mı, blok başlangıcı mı, serbest sayısı taşıyor mu. Çift `free`'yi O(1) yakalamak mümkün değil; `POOL_CHECK_DOUBLE_FREE` ile hata ayıklamada listeyi gez (O(n)), ya da ek bir bit dizisi tut.
- ISR ve ana döngü aynı havuzu kullanıyorsa `alloc` ve `free` kritik bölge içinde olmalı.
- Parçalanma yok: tüm bloklar aynı boyutta. Bu yüzden gerçek zamanlı sistemlerde `malloc`'a tercih edilir.
- Bloğun içindeki eski veri `free`'den sonra silinmez (ilk işaretçi hariç bozulur).

**Mülakatta söyle:** The free list lives inside the free blocks, so the pool needs no extra memory, and alloc and free are O(1) with no fragmentation.

---

## 4. `find` benzeri kütüphane (C++)

**Mantık**
Her koşulu aynı arayüze sok: "bu dosya uyuyor mu?" (`Criterion`). VE, VEYA, DEĞİL de aynı arayüzü uygulasın ve içinde başka koşullar taşısın (Composite deseni). Böylece koşullar iç içe geçer ve `find` hangi koşulun ne olduğunu bilmez. Yeni koşul eklemek = yeni sınıf, var olan koda dokunmadan. `&`, `|`, `!` operatörleri okunabilir yazım verir.

```cpp
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct FileInfo {
    std::string path;
    std::string extension;      // lower case, without the dot: "pdf"
    std::uint64_t size_bytes;
};

constexpr std::uint64_t kMB = 1024ull * 1024ull;

// ---- 1. One small interface: "does this file match?" -------------------------------------
class Criterion {
public:
    virtual ~Criterion() = default;
    virtual bool matches(const FileInfo& file) const = 0;
};

// ---- 2. Value-type handle, so criteria can be combined with & | ! -------------------------
class Filter {
public:
    explicit Filter(std::shared_ptr<const Criterion> criterion) : criterion_(std::move(criterion)) {}
    bool matches(const FileInfo& file) const { return criterion_->matches(file); }
private:
    std::shared_ptr<const Criterion> criterion_;
};

// ---- 3. Combinators: they only know the Criterion interface, never the concrete criteria --
class AndCriterion : public Criterion {
public:
    AndCriterion(Filter left, Filter right) : left_(std::move(left)), right_(std::move(right)) {}
    bool matches(const FileInfo& f) const override { return left_.matches(f) && right_.matches(f); }
private:
    Filter left_, right_;
};

class OrCriterion : public Criterion {
public:
    OrCriterion(Filter left, Filter right) : left_(std::move(left)), right_(std::move(right)) {}
    bool matches(const FileInfo& f) const override { return left_.matches(f) || right_.matches(f); }
private:
    Filter left_, right_;
};

class NotCriterion : public Criterion {
public:
    explicit NotCriterion(Filter inner) : inner_(std::move(inner)) {}
    bool matches(const FileInfo& f) const override { return !inner_.matches(f); }
private:
    Filter inner_;
};

Filter operator&(Filter a, Filter b) { return Filter(std::make_shared<AndCriterion>(std::move(a), std::move(b))); }
Filter operator|(Filter a, Filter b) { return Filter(std::make_shared<OrCriterion>(std::move(a), std::move(b))); }
Filter operator!(Filter a)           { return Filter(std::make_shared<NotCriterion>(std::move(a))); }

// ---- 4. Leaf criteria: the two seed cases ------------------------------------------------
class SizeGreaterThan : public Criterion {
public:
    explicit SizeGreaterThan(std::uint64_t bytes) : bytes_(bytes) {}
    bool matches(const FileInfo& f) const override { return f.size_bytes > bytes_; }
private:
    std::uint64_t bytes_;
};

class ExtensionIs : public Criterion {
public:
    explicit ExtensionIs(std::string ext) : ext_(to_lower(std::move(ext))) {}
    bool matches(const FileInfo& f) const override { return f.extension == ext_; }
private:
    static std::string to_lower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }
    std::string ext_;
};

// Readable factory functions: this is what the caller writes.
Filter size_greater_than(std::uint64_t bytes) { return Filter(std::make_shared<SizeGreaterThan>(bytes)); }
Filter extension_is(std::string ext)          { return Filter(std::make_shared<ExtensionIs>(std::move(ext))); }

// ---- 5. The search itself knows nothing about criteria types ------------------------------
std::vector<FileInfo> find(const std::vector<FileInfo>& files, const Filter& filter)
{
    std::vector<FileInfo> result;
    std::copy_if(files.begin(), files.end(), std::back_inserter(result),
                 [&filter](const FileInfo& f) { return filter.matches(f); });
    return result;
}
```

Kullanım:
```cpp
auto query = (extension_is("pdf")  & size_greater_than(5 * kMB))
           | (extension_is("docx") & size_greater_than(3 * kMB));
auto result = find(all_files, query);
```

**Dikkat edilecekler**
- Maliyet sırası: uzantı bedava, boyut `stat` ister. `extension_is(...) & size_...` sırası `&&` kısa devresiyle gereksiz `stat`'ı engeller.
- `find` dosya listesini dışarıdan alır (dosya sistemine bağlı değil): birim test edilebilir. Gerçekte `std::filesystem::recursive_directory_iterator` ile `FileInfo` üretilir.
- "5 MB'tan büyük" sınırının dahil mi hariç mi olduğunu sor (burada kesinlikle büyük).
- C'de: `struct criterion` içinde `matches` fonksiyon işaretçisi ve somut koşullar bu yapıyı ilk üye olarak taşır, AND/OR iki işaretçi taşır, bellek sahipliği (`free`) ayrıca konuşulur.

**Mülakatta söyle:** I model every condition behind one `matches(file)` interface, and AND, OR and NOT are composites that only know that interface. Adding a condition means adding one class.

---

## 5. Çerçeveli paket ayrıştırıcı

**Mantık**
Format: `0x7E | uzunluk | veri | CRC8`. Bayt akışı bir **durum makinesi** ile işlenir: bayrağı bekle, uzunluğu al, veriyi al, CRC'yi al. İçerikte `0x7E` ya da `0x7D` varsa gönderen `0x7D` ve `değer ^ 0x20` yollar; alıcı `0x7D` görünce bir sonraki baytı `^ 0x20` ile geri çevirir. En önemli kural: **bayrak (`0x7E`) her zaman yeni çerçeve başlatır.** Böylece bozuk ya da yarım çerçeveden sonra kendiliğinden toparlanırız.

```c
#include <stdbool.h>
#include <stdint.h>

#define FRAME_FLAG          0x7Eu
#define FRAME_ESC           0x7Du
#define FRAME_ESC_XOR       0x20u
#define PARSER_MAX_PAYLOAD  64u

typedef void (*frame_cb_t)(const uint8_t *data, uint8_t len, void *ctx);

typedef enum { PS_WAIT_FLAG, PS_LENGTH, PS_DATA, PS_CRC } parser_state_t;

typedef struct {
    parser_state_t state;
    bool           escaped;                 /* previous byte was 0x7D */
    uint8_t        len, count, crc;
    uint8_t        buf[PARSER_MAX_PAYLOAD];
    frame_cb_t     on_frame;
    void          *ctx;
} parser_t;

static uint8_t crc8_update(uint8_t crc, uint8_t byte)       /* CRC-8, polynomial 0x07 */
{
    crc ^= byte;
    for (int bit = 0; bit < 8; bit++)
        crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ 0x07u) : (uint8_t)(crc << 1);
    return crc;
}

void parser_init(parser_t *p, frame_cb_t cb, void *ctx)
{
    p->state = PS_WAIT_FLAG; p->escaped = false; p->len = p->count = p->crc = 0;
    p->on_frame = cb; p->ctx = ctx;
}

void parser_feed(parser_t *p, uint8_t byte)
{
    if (byte == FRAME_FLAG) {                /* a flag ALWAYS starts a new frame: this is how we resynchronise */
        p->state = PS_LENGTH; p->escaped = false; p->crc = 0; p->count = 0;
        return;
    }
    if (p->state == PS_WAIT_FLAG) return;    /* garbage between frames */
    if (byte == FRAME_ESC) { p->escaped = true; return; }
    if (p->escaped) { byte ^= FRAME_ESC_XOR; p->escaped = false; }   /* 7D 5E -> 7E, 7D 5D -> 7D */

    switch (p->state) {
    case PS_LENGTH:
        if (byte > PARSER_MAX_PAYLOAD) { p->state = PS_WAIT_FLAG; break; }   /* absurd length: drop */
        p->len = byte; p->crc = crc8_update(0, byte);
        p->state = (byte == 0) ? PS_CRC : PS_DATA;
        break;
    case PS_DATA:
        p->buf[p->count++] = byte;
        p->crc = crc8_update(p->crc, byte);
        if (p->count == p->len) p->state = PS_CRC;
        break;
    case PS_CRC:
        if (byte == p->crc && p->on_frame != NULL) p->on_frame(p->buf, p->len, p->ctx);
        p->state = PS_WAIT_FLAG;             /* good or bad, wait for the next flag */
        break;
    default:
        p->state = PS_WAIT_FLAG;
        break;
    }
}
```

**Dikkat edilecekler**
- CRC uzunluk ve veri üzerinden hesaplanır; `crc8_update` önceki CRC sorusuyla aynı.
- Absürt uzunluk (`> 64`) gelirse çerçeveyi at ve bir sonraki bayrağı bekle.
- Kaçış baytından hemen sonra bayrak gelirse bayrak kazanır (kaçış iptal).
- Parser ISR'dan değil, ana döngüdeki bayt kuyruğundan beslenmeli. Callback içinde ağır iş yapma.
- Tampon dışına yazmamak için sayaç ve uzunluk sınır kontrolü var.

**Mülakatta söyle:** A byte-wise state machine, with the flag byte always restarting the frame so the parser resynchronises after any corruption.

---

## 6. Yazılım zamanlayıcı yöneticisi

**Mantık**
Donanım tick'i (1 ms) sadece bir sayacı artırır, başka hiçbir şey yapmaz. Ana döngü `timer_process()` ile süresi dolanları bulur ve callback'leri **kendi bağlamında** çalıştırır, ISR'da değil. Sarma güvenliği için "süre doldu mu?" kontrolü `(int32_t)(now - expiry) >= 0` ile yapılır (iki sayının işaretli farkı, sayaç 2³²'de sarsa bile doğru). Periyodik zamanlayıcıda yeni bitiş eski bitişe periyot eklenerek bulunur (kayma birikmez).

```c
#include <stdbool.h>
#include <stdint.h>

#define MAX_TIMERS 8

typedef uint16_t timer_id_t;                 /* 0 = invalid; high byte = generation, low byte = slot */
typedef void (*timer_cb_t)(void *arg);

typedef struct {
    bool       used, periodic;
    uint8_t    gen;                          /* changes every time the slot is reused: stale ids stop working */
    uint32_t   expiry, period;               /* in ticks (ms) */
    timer_cb_t cb;
    void      *arg;
} timer_slot_t;

static timer_slot_t   slots[MAX_TIMERS];     /* touched only by the main loop */
static volatile uint32_t now_ms;             /* written only by the tick ISR */

void timer_tick(void) { now_ms++; }          /* call from the 1 ms tick ISR: nothing else happens here */

/* "now has reached expiry", correct even when the counter wraps (limit: delays < 2^31 ms). */
static bool is_due(uint32_t expiry, uint32_t now) { return (int32_t)(now - expiry) >= 0; }

timer_id_t timer_start(uint32_t delay_ms, timer_cb_t cb, void *arg, bool periodic)
{
    if (cb == NULL || delay_ms == 0 || delay_ms > (uint32_t)INT32_MAX) return 0;
    for (uint8_t i = 0; i < MAX_TIMERS; i++) {
        timer_slot_t *s = &slots[i];
        if (s->used) continue;
        s->gen = (uint8_t)(s->gen + 1u);
        if (s->gen == 0) s->gen = 1;                     /* keep ids non-zero */
        s->used = true; s->periodic = periodic;
        s->period = delay_ms; s->expiry = now_ms + delay_ms;
        s->cb = cb; s->arg = arg;
        return (timer_id_t)(((unsigned)s->gen << 8) | i);
    }
    return 0;                                            /* no free slot */
}

bool timer_stop(timer_id_t id)
{
    unsigned idx = id & 0xFFu;
    if (idx >= MAX_TIMERS) return false;
    timer_slot_t *s = &slots[idx];
    if (!s->used || s->gen != (id >> 8)) return false;   /* already finished or stale id */
    s->used = false;
    return true;
}

void timer_process(void)                     /* call from the main loop: callbacks run HERE, not in the ISR */
{
    uint32_t now = now_ms;                   /* read once */
    for (unsigned i = 0; i < MAX_TIMERS; i++) {
        timer_slot_t *s = &slots[i];
        if (!s->used || !is_due(s->expiry, now)) continue;
        timer_cb_t cb = s->cb;
        void *arg = s->arg;
        if (s->periodic) s->expiry += s->period;         /* from the old expiry, so no drift */
        else             s->used = false;                /* free the slot BEFORE the callback... */
        cb(arg);                                         /* ...so the callback may start/stop timers */
    }
}
```

**Dikkat edilecekler**
- Gecikme 2³¹ ms'den küçük olmalı (işaretli fark kuralı), kod reddediyor.
- Callback çalışmadan önce slot serbest bırakılır / yeniden kurulur, böylece callback içinden `timer_start` ve `timer_stop` güvenli.
- `id` içinde üretim (generation) sayısı var: biten bir zamanlayıcıya ait eski `id` ile `timer_stop` çağrılırsa başka bir zamanlayıcıyı yanlışlıkla durdurmaz.
- `now_ms` sadece ISR'da yazılır, 32 bit okuma Cortex-M'de atomik. Başka mimaride okuma sırasında kesmeyi kapat.
- Döngü O(N): az sayıda zamanlayıcı için iyi. Çok sayıda için zamanlayıcı çarkı (timer wheel) ya da sıralı liste / yığın kullan.

**Mülakatta söyle:** The tick ISR only increments a counter; callbacks run from the main loop, and I compare with a signed difference so counter wrap-around is harmless.

---

## 7. Sırasız paket birleştirme

**Mantık**
Alıcı "sıradaki beklenen" numarayı (`next_seq`) tutar. Gelen paketin `next_seq`'ten **uzaklığı** `(uint16_t)(seq - next_seq)` ile bulunur: sarma sorun olmaz, çünkü işaretsiz çıkarma 65535→0 geçişini doğru hesaplar. Uzaklık 0..31 ise pencere içinde, o slota koy. Çok büyük uzaklık (≥ 0x8000) = geride kaldı, zaten teslim edilmiş. Teslim: `next_seq` slotu doluysa ver ve ilerle, boşluk varsa dur.

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define RX_WINDOW       32u                  /* power of two that divides 65536, so slot = seq % 32 survives wrap */
#define RX_MAX_PAYLOAD  64u

typedef struct { bool present; uint8_t len; uint8_t data[RX_MAX_PAYLOAD]; } rx_slot_t;
typedef struct { uint16_t next_seq; rx_slot_t slots[RX_WINDOW]; } rx_t;
typedef enum { RX_STORED, RX_DUPLICATE, RX_OUT_OF_WINDOW, RX_TOO_LONG } rx_result_t;
typedef void (*rx_deliver_cb_t)(const uint8_t *data, size_t len, void *ctx);

void rx_init(rx_t *r, uint16_t first_seq)
{
    memset(r, 0, sizeof *r);
    r->next_seq = first_seq;
}

rx_result_t rx_receive(rx_t *r, uint16_t seq, const uint8_t *data, size_t len)
{
    if (len > RX_MAX_PAYLOAD) return RX_TOO_LONG;
    uint16_t offset = (uint16_t)(seq - r->next_seq);     /* wraps naturally: 0 = the packet we wait for */
    if (offset >= 0x8000u) return RX_DUPLICATE;          /* "behind us": already delivered */
    if (offset >= RX_WINDOW) return RX_OUT_OF_WINDOW;    /* too far ahead: no room */
    rx_slot_t *s = &r->slots[seq % RX_WINDOW];
    if (s->present) return RX_DUPLICATE;
    memcpy(s->data, data, len);
    s->len = (uint8_t)len;
    s->present = true;
    return RX_STORED;
}

size_t rx_deliver(rx_t *r, rx_deliver_cb_t cb, void *ctx)   /* hands over everything that is now in order */
{
    size_t delivered = 0;
    for (;;) {
        rx_slot_t *s = &r->slots[r->next_seq % RX_WINDOW];
        if (!s->present) break;                          /* gap: wait for the missing packet */
        cb(s->data, s->len, ctx);
        s->present = false;
        r->next_seq++;                                   /* uint16_t: wraps 65535 -> 0 by itself */
        delivered++;
    }
    return delivered;
}
```

**Dikkat edilecekler**
- Pencere 32 (ikinin kuvveti ve 65536'yı böler): `seq % 32` sarma boyunca tutarlı kalır.
- Tekrarlı paket: slot doluysa ya da geride kaldıysa `RX_DUPLICATE`.
- Eksik paket hiç gelmezse pencere dolar ve her şey durur. Gerçek sistemde zaman aşımı ile o paketi "kayıp say" ve `next_seq`'i ilerlet ya da yeniden iletim iste (ARQ).
- Veri kopyalanır (`memcpy`), kaynak tampon hemen yeniden kullanılabilir.

**Mülakatta söyle:** I compute the distance from the next expected sequence number with unsigned 16-bit subtraction, which is wrap-around safe, and use it to index a fixed window.

---

## 8. Aralık birleştirme ve en çok çakışma

**Mantık**
**a) Birleştirme:** Aralıkları başlangıca göre sırala. Sonra tek geçişte: yeni aralık son birleşenle çakışıyorsa bitişi uzat, çakışmıyorsa yeni birleşen başlat. Sıralama baskın: **O(n log n)**, geçiş O(n).

**b) En çok çakışma (toplantı odası sayısı):** Başlangıçları ve bitişleri **ayrı ayrı** sırala. Zamanda ilerle: bir başlangıç, sıradaki bitişten önceyse sayaç artar; değilse bir şey bitmiştir, sayaç azalır. Sayacın en büyük değeri cevaptır. Aralıklar `[başlangıç, bitiş)` kabul edildi: 5'te biten ile 5'te başlayan çakışmaz.

```c
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct { int32_t start, end; } interval_t;

static int cmp_start(const void *a, const void *b)
{
    const interval_t *x = a, *y = b;
    return (x->start > y->start) - (x->start < y->start);    /* no subtraction: cannot overflow */
}
static int cmp_i32(const void *a, const void *b)
{
    int32_t x = *(const int32_t *)a, y = *(const int32_t *)b;
    return (x > y) - (x < y);
}

/* a) Merge overlapping or touching intervals, in place. Returns the new count. O(n log n). */
size_t merge_intervals(interval_t *a, size_t n)
{
    if (n == 0) return 0;
    qsort(a, n, sizeof *a, cmp_start);                       /* sort by start */
    size_t last = 0;                                         /* a[0..last] is the merged result */
    for (size_t i = 1; i < n; i++) {
        if (a[i].start <= a[last].end) {                     /* overlaps the last merged one */
            if (a[i].end > a[last].end) a[last].end = a[i].end;
        } else {
            a[++last] = a[i];                                /* gap: start a new merged interval */
        }
    }
    return last + 1;
}

/* b) Maximum number of intervals active at the same moment. Intervals are [start, end):
 *    one that ends at 5 and one that starts at 5 do not overlap. Returns 0 if memory runs out. */
size_t max_overlap(const interval_t *a, size_t n)
{
    if (n == 0) return 0;
    int32_t *starts = malloc(2 * n * sizeof *starts);
    if (starts == NULL) return 0;
    int32_t *ends = starts + n;
    for (size_t i = 0; i < n; i++) { starts[i] = a[i].start; ends[i] = a[i].end; }
    qsort(starts, n, sizeof *starts, cmp_i32);
    qsort(ends, n, sizeof *ends, cmp_i32);

    size_t i = 0, j = 0, current = 0, best = 0;
    while (i < n) {                                          /* sweep through time */
        if (starts[i] < ends[j]) { current++; i++; if (current > best) best = current; }
        else                     { current--; j++; }         /* something ended first (or at the same time) */
    }
    free(starts);
    return best;
}
```

**Dikkat edilecekler**
- "Dokunan aralıklar birleşir mi?" diye sor. Burada `[1,2]` ve `[2,3]` birleşir (`<=`), çakışma sayımında çakışmaz (yarı açık).
- Karşılaştırmada çıkarma kullanma (`a - b` taşabilir), `(a > b) - (a < b)` kullan.
- Alt sınır kanıtı: sıralama olmadan "hangisi hangisiyle çakışıyor" sorusu için karşılaştırma tabanlı çözümde O(n log n)'den iyisi yok.
- Boş girdi ve tek eleman kontrolü var.

**Mülakatta söyle:** Sort by start and sweep once, extending the last merged interval; for the maximum overlap, sort starts and ends separately and sweep through time.

---

## 9. DMA çift tampon

**Mantık**
ADC, DMA ile 256 örneklik tamponu dönerek doldurur. DMA iki kesme verir: "yarısı doldu" ve "tamamı doldu". İlk yarı dolunca DMA ikinci yarıya geçer, biz ilk yarıyı işleriz. Tamamı dolunca DMA başa sarar, biz ikinci yarıyı işleriz. Kesmeler sadece `ready` bitini kurar, işleme ana döngüde yapılır. **Taşma tespiti:** Bir yarı dolduğunda DMA diğer yarıya geçer. Diğer yarının biti hâlâ kuruluysa (işlenmemiş), DMA işlenmemiş veriyi eziyor demektir, `overruns++`. Bit, işleme **bittikten sonra** temizlenir; böylece "hâlâ işleniyor" durumu da görünür.

```c
#include <stddef.h>
#include <stdint.h>

#define DMA_HALF 128u                            /* samples per half buffer */

extern void irq_disable(void);                   /* platform: cpsid i / cpsie i */
extern void irq_enable(void);

static int16_t dma_buf[2 * DMA_HALF];            /* the DMA fills this circularly: [first half | second half] */
static volatile uint8_t  ready;                  /* bit h set = half h is full and NOT yet processed */
static volatile uint32_t overruns;               /* times the DMA reached a half we had not finished */

/* Called by both ISRs. Half h just became full, so the DMA moves on to the OTHER half.
 * If the other half is still waiting for the loop, the DMA is about to overwrite unprocessed data. */
static void half_full(unsigned h)
{
    if (ready & (1u << (1u - h))) overruns++;
    ready |= (uint8_t)(1u << h);
}

void dma_half_transfer_isr(void)     { half_full(0); }   /* first half full, DMA now writes the second */
void dma_transfer_complete_isr(void) { half_full(1); }   /* second half full, DMA wraps to the first   */

/* Main loop. The bit stays set WHILE we process, so the ISR can see that this half is still busy. */
void adc_process_pending(void (*process)(const int16_t *samples, size_t count))
{
    for (unsigned h = 0; h < 2; h++) {
        if (!(ready & (1u << h))) continue;
        process(&dma_buf[h * DMA_HALF], DMA_HALF);
        irq_disable();                           /* read-modify-write must not interleave with the ISR */
        ready &= (uint8_t)~(1u << h);
        irq_enable();
    }
}
```

**Dikkat edilecekler**
- İşleme süresi, bir yarının dolma süresinden kısa olmalı (128 örnek / örnekleme hızı). Bunu gerçek zamanlı bütçe olarak hesapla.
- `ready` bitinin temizlenmesi (okuma-değiştirme-yazma) kesmeyi kapatarak korunur, çünkü ISR aynı değişkeni kurar.
- Cortex-M7 gibi veri önbelleği olan çiplerde DMA tamponunu okumadan önce önbelleği geçersiz kıl (invalidate) ya da tamponu önbelleksiz bölgeye koy.
- Tampon `int16_t`: DMA'nın veri genişliğiyle (yarım kelime) uyuşmalı ve hizalı olmalı.
- Alternatifler: üç tampon (üçlü tamponlama) ya da yarım kesmesinde işlemi doğrudan kesmede kısa filtreyle yapmak.

**Mülakatta söyle:** The ISRs only mark a half-buffer ready; the loop clears the flag after processing, so a still-set flag on the other half at interrupt time means the DMA is overwriting unprocessed data.

---

## 10. `atoi` ve `itoa`

**Mantık**
`parse_int`: boşlukları atla, işareti oku, en az bir rakam iste, rakamları biriktir. Taşmayı yakalamak için sayıyı **negatif** biriktiririz: negatif aralık bir fazla geniş (`INT_MIN`'in pozitif eşi yok), bu sayede `-2147483648` doğru okunur. Her rakamdan önce `value < (INT_MIN + digit) / 10` ise bir sonraki adım taşar. `int_to_str`: rakamlar ters çıkar, geçici diziye yaz sonra ters kopyala. İşaretsiz çalışarak `INT_MIN`'i güvenle işle.

```c
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum { PARSE_OK, PARSE_INVALID, PARSE_OVERFLOW } parse_result_t;

parse_result_t parse_int(const char *s, int *out)
{
    if (s == NULL || out == NULL) return PARSE_INVALID;
    while (isspace((unsigned char)*s)) s++;                  /* leading whitespace */

    bool negative = false;
    if (*s == '-')      { negative = true; s++; }
    else if (*s == '+') { s++; }
    if (!isdigit((unsigned char)*s)) return PARSE_INVALID;   /* need at least one digit */

    /* Accumulate as a NEGATIVE number: the negative range is one larger (INT_MIN has no positive twin). */
    int value = 0;
    for (; isdigit((unsigned char)*s); s++) {
        int digit = *s - '0';
        if (value < (INT_MIN + digit) / 10) return PARSE_OVERFLOW;   /* value*10 - digit would go below INT_MIN */
        value = value * 10 - digit;
    }
    while (isspace((unsigned char)*s)) s++;                  /* trailing whitespace is fine... */
    if (*s != '\0') return PARSE_INVALID;                    /* ...other trailing characters are not */

    if (!negative) {
        if (value == INT_MIN) return PARSE_OVERFLOW;         /* +2147483648 does not fit */
        value = -value;
    }
    *out = value;
    return PARSE_OK;
}

/* Writes v as text with a terminating '\0'. Returns the length, or 0 if the buffer is too small. */
size_t int_to_str(int v, char *buf, size_t cap)
{
    char tmp[12];                                            /* "-2147483648" is 11 chars */
    size_t n = 0;
    unsigned int u = (v < 0) ? 0u - (unsigned int)v : (unsigned int)v;   /* safe for INT_MIN */
    do { tmp[n++] = (char)('0' + u % 10u); u /= 10u; } while (u != 0u);  /* digits come out reversed */
    if (v < 0) tmp[n++] = '-';

    if (cap < n + 1) return 0;
    for (size_t i = 0; i < n; i++) buf[i] = tmp[n - 1 - i];
    buf[n] = '\0';
    return n;
}
```

**Dikkat edilecekler**
- Kenar durumlar: boş dize, sadece `-`, `12a`, baştaki sıfırlar, `+` işareti, sondaki boşluk.
- `-v` işlemi `INT_MIN` için tanımsız davranış; bu yüzden işaretsiz çıkarma (`0u - (unsigned)v`).
- Çıktı tamponu yetmezse `0` döner, taşırmaz.
- `isdigit` ve `isspace` için `unsigned char`'a dönüştür.
- Dönüş değeri `enum`: geçersiz girdi ile taşmayı ayırır.

**Mülakatta söyle:** I accumulate negatively so INT_MIN is representable, and check overflow before each multiply-add instead of after.

---

## 11. CRC-8 / CRC-16

**Mantık**
CRC, bayt dizisini çok uzun bir ikili sayı gibi bir polinoma bölmenin **kalanıdır** (bölme XOR ile yapılır). Bit bit sürüm: bayt XOR'lanır, sonra 8 kez: üst bit 1 ise kaydır ve polinomla XOR, değilse sadece kaydır. Tablo sürümü: bir baytın 8 adım sonucu sadece o bayta bağlı, 256 sonucu önceden hesapla ve bayt başına tek bakış yap.

```c
#include <stdint.h>
#include <stddef.h>
#define CRC8_POLY 0x07u
uint8_t crc8_bitwise(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++)
            crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ CRC8_POLY) : (uint8_t)(crc << 1);
    }
    return crc;
}
static uint8_t crc8_table[256];
void crc8_table_init(void)
{
    for (unsigned b = 0; b < 256; b++) {
        uint8_t crc = (uint8_t)b;
        for (int bit = 0; bit < 8; bit++)
            crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ CRC8_POLY) : (uint8_t)(crc << 1);
        crc8_table[b] = crc;
    }
}
uint8_t crc8_table_driven(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) crc = crc8_table[crc ^ data[i]];
    return crc;
}
uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (int bit = 0; bit < 8; bit++)
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
    }
    return crc;
}
```

**Dikkat edilecekler**
- Bit bit: bellek yok, yavaş (bayt başına 8 adım). Tablo: 256 bayt (CRC-8) ya da 512 bayt (CRC-16), `const` ile **flash**'ta. Ara çözüm: 16 girişlik tablo (yarım bayt).
- Doğrulama değerleri: `"123456789"` için CRC-8 = `0xF4`, CRC-16/CCITT-FALSE = `0x29B1`.
- Parametreleri karşı tarafla netleştir: polinom, başlangıç değeri, bit yansıtma, son XOR.
- CRC'yi veriye ekleyip tüm paketin CRC'sini alırsan sonuç `0` çıkar.
- CRC rastgele hatayı yakalar, kötü niyeti yakalamaz (güvenlik için HMAC/imza).
- STM32'de donanım CRC birimi var, büyük veri için kullan.

**Mülakatta söyle:** I'd start bitwise since it needs no memory, move to a const table in flash if throughput matters, and agree on polynomial, initial value, reflection and final XOR with the other side.

---

## 12. Endianness ve yapı paketleme

**Mantık**
Bir `struct`'ı doğrudan ağa göndermek üç sebeple tehlikelidir: **dolgu** (derleyici alanlar arasına boşluk koyar, burada 7 yerine 8 bayt), **hizalama** (bazı çipler hizasız erişimde hata verir) ve **bayt sırası** (iki taraf farklı endian olabilir). Çözüm: kablodaki formatı kendin tanımla (burada big endian, 7 bayt) ve alanları bayt bayt yaz/oku. Kaydırma ve maskeleme CPU'nun endian'ından bağımsızdır.

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* In-memory form: the compiler may add padding and uses the CPU's byte order. NEVER send this raw. */
typedef struct {
    uint8_t  type;
    uint16_t length;
    uint32_t timestamp;
} header_t;

/* Wire form (big endian, no padding): type(1) | length(2) | timestamp(4) */
#define HEADER_WIRE_SIZE 7u

size_t header_serialize(const header_t *h, uint8_t *buf, size_t cap)
{
    if (cap < HEADER_WIRE_SIZE) return 0;
    buf[0] = h->type;
    buf[1] = (uint8_t)(h->length >> 8);
    buf[2] = (uint8_t)(h->length);
    buf[3] = (uint8_t)(h->timestamp >> 24);
    buf[4] = (uint8_t)(h->timestamp >> 16);
    buf[5] = (uint8_t)(h->timestamp >> 8);
    buf[6] = (uint8_t)(h->timestamp);
    return HEADER_WIRE_SIZE;
}

bool header_parse(const uint8_t *buf, size_t len, header_t *h)
{
    if (len < HEADER_WIRE_SIZE) return false;
    h->type      = buf[0];
    h->length    = (uint16_t)(((uint16_t)buf[1] << 8) | buf[2]);
    h->timestamp = ((uint32_t)buf[3] << 24) | ((uint32_t)buf[4] << 16) |
                   ((uint32_t)buf[5] << 8)  |  (uint32_t)buf[6];
    return true;
}
```

**Dikkat edilecekler**
- `__attribute__((packed))` hızlı çözüm gibi görünür ama hizasız erişim (Cortex-M0'da hata) ve işaretçi alma sorunları getirir. Bayt bayt serileştirme taşınabilir.
- `memcpy(&h, buf, 7)` de endian sorununu çözmez.
- `parse` önce uzunluğu kontrol eder: kısa tampon ile taşma olmaz.
- Ağ standardı big endian ("network byte order"), ama protokol karar verir, belirt.
- Derleme zamanı kontrolü: `_Static_assert(HEADER_WIRE_SIZE == 7, ...)`.

**Mülakatta söyle:** I define the wire format explicitly and serialise field by field with shifts, so the result doesn't depend on padding, alignment or the CPU's endianness.

---

## Genel mülakat ipuçları

- **Önce sor:** kapasite, tek/çok iş parçacığı, ISR'dan çağrılacak mı, bellek tahsisi var mı, sınırlar dahil mi.
- **Sesli düşün:** basit çözüm → sorunu → daha iyi çözüm.
- **Kenar durumları say:** boş, dolu, tek eleman, taşma, sarma, geçersiz girdi.
- **İsimlendirme:** `crc`, `next_seq`, `free_count` gibi anlamlı isimler. Sihirli sayıları `#define` / `enum` yap.
- **Karmaşıklığı söyle:** zaman ve bellek.
- **Kesme bağlamı:** ISR'da kısa iş, paylaşılan veri için `volatile` + bariyer / kritik bölge, tek yazar kuralı.
- **Derleyici yok:** noktalı virgülleri, `uint32_t` dönüşümlerini, `size_t` / işaretli karşılaştırmaları elle kontrol et.
