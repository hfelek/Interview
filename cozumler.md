# Çözümler: Önce Basit Açıklama, Sonra Cevap

Bu dosya, birlikte çalıştığımız soruların hepsini tek yerde toplar. Her soruda sıra aynı:

1. **Soru** (kısa hatırlatma)
2. **Basit açıklama** (kod yok, benzetme ve örnekle)
3. **Cevap** (C kodu)
4. **Karmaşıklık ve tuzaklar**

Kodların hepsi derlendi ve rastgele testlerle kaba çözümle karşılaştırıldı (ASan/UBSan açık). Kodlar farklı dosyalardan geldiği için, birbirinden bağımsız parçalardır. Aynı isimler (`cell_t`, `sift_down`, `Node`) farklı bölümlerde farklı tanımlanmış olabilir, hepsini tek dosyada birleştirmeyin.

Bu dosyayla birlikte: `veri-yapilari-calisma-notu.md` (yapılar), `hangi-yapiyi-secmeli.md` (kalıplar), `bst-calisma.md` (BST), `sonra-calisilacaklar.md` (bekleyenler).

**İçindekiler**

*A. Temel sorular:* A1 Two Sum · A2 Geçerli parantez · A3 İlk tekil ID · A4 UART tamponu · A5 Ağaç derinliği ve seviye sırası · A6 K sıralı log birleştirme

*B. Alıştırma serisi (12 soru):* B1 Aralık toplamı (BST) · B2 Kaç gün sonra daha sıcak · B3 Tekrar eden paketler · B4 En sık K hata kodu · B5 Akıştan medyan · B6 Sıfırları sona taşı · B7 Zamanlayıcı yöneticisi · B8 Toplamı K olan alt diziler · B9 Bağlantı tablosu (LRU) · B10 Toplantı odası · B11 Sürücü başlatma sırası · B12 Son N örneğin tepe değeri

---

## Ortak parça: sayaçlı hash tablosu

Birçok çözüm bunu kullanır (A3, B3, B4, B8). Açık adreslemeli, `malloc`'suz çalışabilir (tabloyu siz ayırırsınız).

```c
typedef struct { int key; int count; bool isFilled; } cell_t;

static uint32_t hash_slot(int key, uint32_t mask, uint32_t iter)
{
    uint32_t h = (uint32_t)key * 2654435761u;    /* Knuth çarpımsal */
    h ^= h >> 16;                                /* üst bitleri aşağı karıştır */
    return (h + iter) & mask;                    /* doğrusal yoklama */
}

/* key varsa count++, yoksa yeni hücre (count = 1) */
static bool hash_add(cell_t *table, int key, uint32_t mask)
{
    for (uint32_t i = 0; i <= mask; i++) {
        cell_t *c = &table[hash_slot(key, mask, i)];
        if (!c->isFilled) { c->isFilled = true; c->key = key; c->count = 1; return true; }
        if (c->key == key) { c->count++; return true; }
    }
    return false;                                /* tablo dolu (olmamalı) */
}

/* key'in sayacı, yoksa 0 */
static int hash_get(const cell_t *table, int key, uint32_t mask)
{
    for (uint32_t i = 0; i <= mask; i++) {
        const cell_t *c = &table[hash_slot(key, mask, i)];
        if (!c->isFilled) return 0;              /* boş slot: zincir bitti, yok */
        if (c->key == key) return c->count;
    }
    return 0;
}
```

**Tablo boyutu:** kapasite 2'nin kuvveti ve en az `2n` (yük faktörü ≤ 0.5). `calloc` ile ayırın (tüm `isFilled` sıfır olur), `mask = cap - 1`.

```c
uint32_t cap = 2;
while (cap < 2u * (uint32_t)n) cap <<= 1;
uint32_t mask = cap - 1;
cell_t *table = calloc(cap, sizeof *table);
```

---

# A. Temel Sorular

## A1. Two Sum

**Soru:** Bir dizide toplamı `target` olan **iki farklı elemanın indekslerini** bulun (tam bir çözüm var, aynı elemanı iki kez kullanamazsınız). `O(n²)`'den iyi olsun.

### Basit açıklama
Her sayıya bakarken kendinize sorun: **"Bana tamamlayıcı olacak sayı (target − ben) daha önce geldi mi?"**

- Geldiyse bitti: iki indeksi bulduk.
- Gelmediyse kendimi bir deftere yazarım (değer → indeks) ve sonraki sayıya geçerim.

Deftere hızlı bakabilmek için **hash map** kullanırız.

```
nums = [2, 7, 11, 15], target = 9
i=0: 2 için aranan 7. Defterde yok. Deftere yaz: {2:0}
i=1: 7 için aranan 2. Defterde var (indeks 0)! → cevap [0, 1]
```

**Sıra önemli:** önce defterde ara, **sonra** kendini ekle. Böylece bir eleman kendisiyle eşleşmez (`[3, 3]`, hedef 6 → ilk 3 deftere girer, ikinci 3 onu bulur).

### Cevap
```c
typedef struct { int value; int index; bool isFilled; } tcell_t;

static bool ts_insert(tcell_t *t, int num, int index, uint32_t mask)
{
    for (uint32_t i = 0; i <= mask; i++) {
        tcell_t *c = &t[hash_slot(num, mask, i)];
        if (!c->isFilled) { c->isFilled = true; c->value = num; c->index = index; return true; }
    }
    return false;
}

static bool ts_get(const tcell_t *t, int num, uint32_t mask, int *index)
{
    for (uint32_t i = 0; i <= mask; i++) {
        const tcell_t *c = &t[hash_slot(num, mask, i)];
        if (!c->isFilled) return false;
        if (c->value == num) { *index = c->index; return true; }
    }
    return false;
}

void twoSum(const int *nums, int numsSize, int target, int out[2])
{
    uint32_t cap = 2;
    while (cap < 2u * (uint32_t)numsSize) cap <<= 1;
    uint32_t mask = cap - 1;

    tcell_t *t = calloc(cap, sizeof *t);
    if (t == NULL) return;

    for (int i = 0; i < numsSize; i++) {
        long long diff = (long long)target - nums[i];       /* taşmayı önle */
        int j;
        if (diff >= INT_MIN && diff <= INT_MAX && ts_get(t, (int)diff, mask, &j)) {
            out[0] = j;                                     /* önce bulunan */
            out[1] = i;
            free(t);
            return;
        }
        if (!ts_insert(t, nums[i], i, mask)) break;         /* sonra kendini ekle */
    }
    free(t);
}
```

### Karmaşıklık ve tuzaklar
- **O(n) zaman** (ortalama), **O(n) alan**. Kaba çözüm O(n²).
- `target - nums[i]` `int` aralığını aşabilir, bu yüzden `long long` kullandık.
- Hash fonksiyonunda işaretli taşmadan kaçınmak için `uint32_t`'ye çevirdik.
- Kapasite `≥ 2n`, yoklama döngüsü `i <= mask`.

---

## A2. Geçerli parantez

**Soru:** `()[]{}` karakterlerinden oluşan bir string verilmiş. Her açılan işaret **doğru türde ve doğru sırada** kapanmışsa `true`.

### Basit açıklama
Bir **tabak yığını** gibi düşünün. Bir parantez açtığınızda yığına, **kapanışta bekleyeceğiniz karakteri** koyarsınız. Bir kapanış gelince yığının en üstündeki tabağa bakarsınız: bu o olmalı. Değilse hata.

```
"([{}])"
(  → yığına ')' koy        yığın: )
[  → yığına ']' koy        yığın: ) ]
{  → yığına '}' koy        yığın: ) ] }
}  → tepe '}' ✔ çek        yığın: ) ]
]  → tepe ']' ✔ çek        yığın: )
)  → tepe ')' ✔ çek        yığın: (boş)
sonda yığın boş → geçerli
```

**Sonda yığın boş olmalı** ve döngü boyunca hata olmamalı, ikisi **birlikte** (`"(("` hatasız biter ama yığın boş değildir).

### Cevap
```c
#define PAREN_MAX 100000

bool isValid(const char *s)
{
    static char stack[PAREN_MAX];                   /* malloc yok; yeniden girişli değil */
    size_t top = 0;

    for (; *s; s++) {
        switch (*s) {
        case '(': if (top >= PAREN_MAX) return false; stack[top++] = ')'; break;
        case '[': if (top >= PAREN_MAX) return false; stack[top++] = ']'; break;
        case '{': if (top >= PAREN_MAX) return false; stack[top++] = '}'; break;
        case ')': case ']': case '}':
            if (top == 0 || stack[--top] != *s) return false;   /* boş yığından çekme! */
            break;
        default:
            return false;                           /* geçersiz karakter */
        }
    }
    return top == 0;                                /* hepsi kapanmış olmalı */
}
```

### Karmaşıklık ve tuzaklar
- **O(n) zaman, O(n) alan** (en kötü `((((...`).
- Boş yığından `pop` yapmayın (`"())"`).
- Geçersiz karakterlere karar verin (burada `false`).
- Yığın taşması için sabit üst sınır.

---

## A3. İlk tekil ID (ve en az geçen değer)

**Soru:** Bir ID listesinde **tam bir kez** geçen **ilk** ID'yi döndürün (yoksa `-1`).

### Basit açıklama
İki geçiş:

1. **Sayım:** Her ID'nin kaç kez geçtiğini bir deftere yaz.
2. **Arama:** Orijinal listeyi **baştan** yine gez. Defterde sayısı 1 olan **ilk** ID'yi döndür.

İkinci geçişte **tabloyu değil diziyi** geziyoruz, çünkü "ilk" bilgisini sadece dizinin sırası taşır (hash tablosu sırayı saklamaz).

```
ids = [4, 7, 4, 9, 7, 2]
sayım:   {4:2, 7:2, 9:1, 2:1}
gezi:    4(2) 7(2) 9(1) → dur, cevap 9
```

### Cevap
```c
int firstUnique(const int *ids, int n)
{
    uint32_t cap = 2;
    while (cap < 2u * (uint32_t)n) cap <<= 1;
    uint32_t mask = cap - 1;

    cell_t *table = calloc(cap, sizeof *table);
    if (table == NULL) return -1;

    for (int i = 0; i < n; i++) {                         /* Geçiş 1: say */
        if (!hash_add(table, ids[i], mask)) { free(table); return -1; }
    }
    for (int i = 0; i < n; i++) {                         /* Geçiş 2: diziyi gez */
        if (hash_get(table, ids[i], mask) == 1) {
            int r = ids[i];
            free(table);
            return r;
        }
    }
    free(table);
    return -1;
}
```

### Varyant: en az geçen değer
Sayımdan **sonra** dizide gezip en küçük sayacı tutarsınız. **Sayım sırasında** karşılaştırmayın (sayaçlar henüz son değerinde değil).
```c
int leastFrequent(const int *ids, int n)        /* n >= 1; beraberlikte dizide ilk gelen */
{
    uint32_t cap = 2;
    while (cap < 2u * (uint32_t)n) cap <<= 1;
    uint32_t mask = cap - 1;

    cell_t *table = calloc(cap, sizeof *table);
    if (table == NULL) return 0;

    for (int i = 0; i < n; i++) hash_add(table, ids[i], mask);

    int best = ids[0], bestCount = hash_get(table, ids[0], mask);
    for (int i = 1; i < n; i++) {
        int c = hash_get(table, ids[i], mask);
        if (c < bestCount) { bestCount = c; best = ids[i]; }    /* kesin küçük: ilk gelen kazanır */
    }
    free(table);
    return best;
}
```

### Karmaşıklık ve tuzaklar
- **O(n) zaman, O(n) alan.**
- Min heap burada gereksiz (heap "ilk"i bilmez).
- ID `-1` geçerliyse `-1` dönüş değeri belirsizleşir; mülakatta söyleyin.
- Beraberlik kuralını önce sorun.

---

## A4. UART alım tamponu (ring buffer)

**Soru:** Kesme servisi (ISR) byte'ları yazıyor, ana döngü geliş sırasıyla okuyor. Bellek sabit, `malloc` yok. Doluyken gelen byte **atılır** (`false`), boşken okuma `false`.

### Basit açıklama
Bir **daire** gibi düşünün: dizinin sonuna gelince başa dönersiniz. İki sayaç:

- `head` = toplam kaç byte **yazıldı**
- `tail` = toplam kaç byte **okundu**

```
içeride kaç byte var = head - tail          (0 ile N arası)
boş  : head == tail
dolu : head - tail == N
yazılacak yer: buf[head & (N-1)]            (N 2'nin kuvveti → bölmesiz)
okunacak yer : buf[tail & (N-1)]
```

Sayaçlar hiç sıfırlanmaz, **sürekli artar** (işaretsiz taşma tanımlı). Bu yüzden "dolu" ile "boş" karışmaz ve dizinin tamamını kullanırız. N'nin 2'nin kuvveti olması **şart**: sayaç 2³²'de sarılırken `sayaç & (N-1)` indeksi atlamasın.

**Neden kilit gerekmez?** `head`'i **sadece üretici** (ISR), `tail`'i **sadece tüketici** (ana döngü) yazar. Ortak yazılan alan yok.

### Cevap
```c
#define ARRAY_SIZE 64u                         /* 2'nin kuvveti */
#define MASK       (ARRAY_SIZE - 1u)

typedef char size_must_be_pow2[((ARRAY_SIZE & MASK) == 0) ? 1 : -1];   /* C99: derleme zamanı kontrol */

#define BARRIER() __asm volatile("" ::: "memory")   /* derleyici bariyeri; çok çekirdekte dmb ekleyin */

typedef struct {
    uint8_t           buf[ARRAY_SIZE];
    volatile uint32_t head;                    /* toplam yazılan: sadece ISR yazar */
    volatile uint32_t tail;                    /* toplam okunan : sadece ana döngü yazar */
} buf_t;

void buf_init(buf_t *b)
{
    if (b == NULL) return;
    b->head = 0;
    b->tail = 0;
}

bool buf_put(buf_t *b, uint8_t byte)           /* ISR'dan */
{
    if (b == NULL) return false;

    uint32_t h = b->head;                      /* kendi sayacım */
    uint32_t t = b->tail;                      /* diğer tarafın, bir kez oku */

    if ((uint32_t)(h - t) >= ARRAY_SIZE) return false;   /* dolu: ezme! */

    b->buf[h & MASK] = byte;
    BARRIER();                                 /* veri, head'den ÖNCE yazılsın */
    b->head = h + 1u;                          /* şimdi tüketici görebilir */
    return true;
}

bool buf_get(buf_t *b, uint8_t *byte)          /* ana döngüden */
{
    if (b == NULL || byte == NULL) return false;

    uint32_t t = b->tail;
    uint32_t h = b->head;

    if (h == t) return false;                  /* boş: *byte'a dokunma */

    BARRIER();                                 /* head okundu, SONRA veriyi oku */
    *byte = b->buf[t & MASK];
    BARRIER();                                 /* okuma bitti, SONRA slotu serbest bırak */
    b->tail = t + 1u;
    return true;
}
```

### Karmaşıklık ve tuzaklar
- `put` ve `get` **O(1)**, bellek sabit.
- **İlk yazdığınız kodda yapılan hatalar:** `get`'te `tail--` (doğrusu `tail++`), `get`'te maskeyi unutmak (dizi dışı okuma), `sizeof buf_t` (doğrusu `sizeof(buf_t)`), doluyken yazıp eski veriyi ezmek.
- **`volatile` yetmez:** atomiklik ve sıra garantisi vermez. 8/16 bit MCU'da sayaç tipini kelime genişliğine indirin. Tek çekirdekte derleyici bariyeri yeter, çok çekirdekte donanım bariyeri (`dmb`) ya da C11 `<stdatomic.h>` (release/acquire).
- **Mutex ISR'da kullanılamaz** (kilitlenir). Birden fazla üretici/tüketici varsa kısa kesme-kapatma bölümü gerekir.
- Mülakat cümlesi: *"Her sayacın tek yazarı var, o yüzden kilit gerekmiyor; veriyi yazdıktan sonra head'i yayınlıyorum."*

---

## A5. Ağaç derinliği ve seviye sırası

**Soru:** İkili ağaç için (1) `maxDepth`, (2) `levelOrder` (seviye seviye, soldan sağa, `out`'a yaz).

### Basit açıklama
**Derinlik:** Bir ağacın derinliği = `1 + (sol alt ağacın derinliği ile sağ alt ağacın derinliğinden büyüğü)`. Boş ağaç 0. Ağaç zaten "alt ağaçlardan oluşan" bir yapı, bu yüzden özyineleme doğal.

**Seviye sırası:** Önce kök, sonra köke bağlı çocuklar, sonra onların çocukları... Yani **geliş sırasıyla işle**, bu bir **kuyruk** (FIFO) işi: bir düğümü kuyruktan çıkar, değerini yaz, çocuklarını (sol, sağ) kuyruğun sonuna ekle.

```
      3                     kuyruk: [3]
     / \                    3 çıktı → ekle 9, 20 → [9, 20]
    9   20                  9 çıktı → [20]
       /  \                 20 çıktı → ekle 15, 7 → [15, 7]
      15   7                15, 7 çıktı → bitti
                            çıktı: 3 9 20 15 7
```

### Cevap
```c
typedef struct Node { int val; struct Node *left, *right; } Node;

int maxDepth(const Node *root)
{
    if (root == NULL) return 0;
    int l = maxDepth(root->left);
    int r = maxDepth(root->right);
    return 1 + (l > r ? l : r);
}

int levelOrder(const Node *root, int *out, int outCap)
{
    if (root == NULL || out == NULL || outCap <= 0) return 0;

    const Node **queue = malloc((size_t)outCap * sizeof *queue);
    if (queue == NULL) return 0;

    int qHead = 0, qTail = 0, n = 0;
    queue[qTail++] = root;

    while (qHead < qTail) {
        const Node *cur = queue[qHead++];
        out[n++] = cur->val;
        if (cur->left  && qTail < outCap) queue[qTail++] = cur->left;
        if (cur->right && qTail < outCap) queue[qTail++] = cur->right;
    }
    free(queue);
    return n;
}
```

**`malloc`'suz seçenek:** Her seviye için ağacı baştan gezip sadece o seviyedeki düğümleri yazın (ek bellek O(h), süre O(n·h)):
```c
static bool writeLevel(const Node *n, int level, int *out, int outCap, int *cnt)
{
    if (n == NULL) return false;
    if (level == 1) {
        if (*cnt < outCap) out[(*cnt)++] = n->val;
        return true;
    }
    bool l = writeLevel(n->left,  level - 1, out, outCap, cnt);
    bool r = writeLevel(n->right, level - 1, out, outCap, cnt);
    return l || r;
}

int levelOrderNoAlloc(const Node *root, int *out, int outCap)
{
    int cnt = 0;
    if (out == NULL || outCap <= 0) return 0;
    for (int level = 1; cnt < outCap && writeLevel(root, level, out, outCap, &cnt); level++)
        ;
    return cnt;
}
```

### Karmaşıklık ve tuzaklar
- `maxDepth`: **O(n)** zaman, **O(h)** yığın (dengeli: log n, dejenere: n).
- `levelOrder`: **O(n)** zaman, **O(w)** kuyruk (w: en geniş seviye).
- Kuyruk için ring buffer gerekmez: her düğüm bir kez girer, `qTail` toplam girişi sayar, düz dizi yeter.
- **Dejenere ağaçta (zincir) özyineleme yığını taşırır.** Gömülüde açık yığınla döngü yazın ve derinlik sınırı koyun.
- `outCap` aşılırsa ilk `outCap` düğüm (BFS sırasıyla) yazılır.

---

## A6. K sıralı log'u birleştirme (bellek O(K))

**Soru:** K sensörün her biri kayıtlarını **zamana göre sıralı** üretiyor. Hepsini, **belleğe yüklemeden**, tek kronolojik akışta birleştirin.

### Basit açıklama
K tane **sıralı kart destesi**. Her destenin sadece **en üstteki kartına** bakarsınız, bu K kartın en küçüğünü alıp çıktıya yazarsınız, o destenin yeni üst kartını açarsınız. Tekrar.

**Tek bir heap** var ve içinde her sensörden **sadece 1 kayıt** (sıradaki). "K kartın en küçüğü hangisi?" sorusunu heap hızlıca cevaplar. Heap hiçbir zaman K elemandan fazla tutmaz, bu yüzden bellek O(K).

```
A: 1 → 4 → 9      B: 2 → 4 → 5 → 10      C: 3
heap {A1,B2,C3} → 1 yaz, A'dan 4 gelir → {A4,B2,C3} → 2 yaz, B'den 4 → {A4,B4,C3} → 3 yaz ...
çıktı: 1 2 3 4 4 5 9 10
```

### Cevap
```c
#define MAX_SOURCES 16

typedef struct { uint64_t ts; uint32_t value; } log_rec_t;

typedef struct {
    bool (*next)(void *ctx, log_rec_t *out);   /* false: kayıt kalmadı */
    void *ctx;
} log_source_t;

typedef bool (*log_sink_fn)(void *ctx, const log_rec_t *rec, int src);   /* false: dur */

typedef struct { log_rec_t rec; int src; } heap_item_t;

static bool item_before(const heap_item_t *a, const heap_item_t *b)
{
    if (a->rec.ts != b->rec.ts) return a->rec.ts < b->rec.ts;
    return a->src < b->src;                    /* eşit zaman: küçük kaynak önce */
}

static void sift_up(heap_item_t *h, int i)
{
    while (i > 0) {
        int p = (i - 1) / 2;
        if (!item_before(&h[i], &h[p])) break;
        heap_item_t t = h[i]; h[i] = h[p]; h[p] = t;
        i = p;
    }
}

static void sift_down(heap_item_t *h, int size, int i)
{
    for (;;) {
        int l = 2 * i + 1, r = 2 * i + 2, m = i;
        if (l < size && item_before(&h[l], &h[m])) m = l;
        if (r < size && item_before(&h[r], &h[m])) m = r;
        if (m == i) return;
        heap_item_t t = h[i]; h[i] = h[m]; h[m] = t;
        i = m;
    }
}

long merge_logs(const log_source_t *src, int k, log_sink_fn sink, void *sink_ctx)
{
    if (src == NULL || sink == NULL || k < 0 || k > MAX_SOURCES) return -1;

    heap_item_t heap[MAX_SOURCES];
    int size = 0;
    long written = 0;

    for (int i = 0; i < k; i++) {              /* 1) her kaynaktan ilk kayıt */
        heap_item_t it;
        if (src[i].next(src[i].ctx, &it.rec)) {
            it.src = i;
            heap[size] = it;
            sift_up(heap, size);
            size++;
        }
    }

    while (size > 0) {                         /* 2) en küçük çıkar, aynı kaynaktan yenisi gelir */
        heap_item_t top = heap[0];
        if (!sink(sink_ctx, &top.rec, top.src)) return written;
        written++;

        if (src[top.src].next(src[top.src].ctx, &heap[0].rec)) {
            heap[0].src = top.src;             /* kökü yenisiyle değiştir */
            sift_down(heap, size, 0);
        } else {
            heap[0] = heap[--size];            /* kaynak bitti: heap küçülür */
            sift_down(heap, size, 0);
        }
    }
    return written;
}
```

### Karmaşıklık ve tuzaklar
- **O(N log K)** zaman (N toplam kayıt), **O(K)** bellek.
- Her kaynak **önceden sıralı** olmalı.
- Tick 32 bit sarılıyorsa karşılaştırmayı işaretli farkla yapın: `(int32_t)(a - b) < 0`.
- Kökü değiştirip `sift_down` yapmak, `pop` + `push`'tan ucuzdur.

---

# B. Alıştırma Serisi (12 soru)

## B1. Aralık toplamı (BST) — Kolay

**Soru:** BST'de değeri `[low, high]` aralığında olan düğümlerin toplamı.

### Basit açıklama
BST kuralı: sol alt ağaç < düğüm < sağ alt ağaç. Bu kural sayesinde **bazı dalları hiç gezmeyiz**. Karar **bulunduğunuz düğümün değerine** göre verilir (çocuğuna göre değil):

| Durum | Anlamı | Ne yapılır |
|---|---|---|
| `düğüm < low` | düğüm ve **sol alt ağacın tamamı** low'dan küçük | sayma, sadece **sağa** git |
| `düğüm > high` | düğüm ve **sağ alt ağacın tamamı** high'dan büyük | sayma, sadece **sola** git |
| `low ≤ düğüm ≤ high` | aralıkta | **say**, iki yana da git |

```
        10                 low=7, high=15
       /  \
      5    15              10 aralıkta, say, iki yana git
     / \     \             5 < 7: say(ma), sadece sağa git → 7 say
    3   7     18           15 say; 18 > 15: sayma, sola git (yok)
                           toplam 7+10+15 = 32
```
Çocuğa bakarak "5 < low, sola gitmem" demek **7'yi kaçırtır**: 5 aralığın dışında ama sağ çocuğu 7 aralıkta.

### Cevap
```c
int rangeSumBST(const Node *root, int low, int high)
{
    if (root == NULL) return 0;
    if (root->val < low)  return rangeSumBST(root->right, low, high);
    if (root->val > high) return rangeSumBST(root->left,  low, high);
    return root->val
         + rangeSumBST(root->left,  low, high)
         + rangeSumBST(root->right, low, high);
}
```

**Özyinelemesiz sürüm** (açık yığın, taşarsa `-1`):
```c
#define MAX_DEPTH 64
int rangeSumBST_iter(const Node *root, int low, int high)
{
    const Node *stack[MAX_DEPTH];
    int top = 0, sum = 0;
    if (root) stack[top++] = root;

    while (top > 0) {
        const Node *n = stack[--top];
        if (n->val < low) {
            if (n->right) { if (top >= MAX_DEPTH) return -1; stack[top++] = n->right; }
        } else if (n->val > high) {
            if (n->left)  { if (top >= MAX_DEPTH) return -1; stack[top++] = n->left; }
        } else {
            sum += n->val;
            if (n->left)  { if (top >= MAX_DEPTH) return -1; stack[top++] = n->left; }
            if (n->right) { if (top >= MAX_DEPTH) return -1; stack[top++] = n->right; }
        }
    }
    return sum;
}
```

### Karmaşıklık ve tuzaklar
- Zaman **O(h + m)** (h: yükseklik, m: aralıktaki düğüm sayısı). En kötü **O(n)**.
- Alan **O(h)**: dengeli ağaçta log n, dejenerede n.
- Dejenere ağaçta özyineleme yığını taşabilir → açık yığın + sınır.
- En çok 2·10⁴ düğüm, değerler ≤ 10⁵ → toplam ≈ 1.8·10⁹, `int`'e sığar.

---

## B2. Kaç gün sonra daha sıcak — Orta

**Soru:** Her gün için, ondan sonra **daha yüksek** bir sıcaklığın kaç gün sonra geldiğini bulun (yoksa 0).

```
t   = [73, 74, 75, 71, 69, 72, 76, 73]
out = [ 1,  1,  4,  2,  1,  1,  0,  0]
```

### Basit açıklama (yığın çözümü, soldan sağa)
Henüz "cevabını bulamamış günleri" bir **bekleme yığınına** koyarız. Yeni bir gün gelince, yığının **en üstündeki** (en yeni) bekleyen günlerden **bugünden daha soğuk olanların** cevabı bugündür.

Neden yığın? Bekleyen günlerin sıcaklıkları **kendiliğinden büyükten küçüğe sıralı** kalır (bir gün sonra daha sıcak biri gelseydi o zaten çözülüp çıkardı). Yani çözülecek günler hep **yığının üstünde**, sıralamaya gerek yok.

```
t = 73 74 75 71 69 72 76 73
72 geldi: bekleyenler [75, 71, 69] → 69 ve 71 çözülür (cevap 1 ve 2), 75 kalır
```

Min heap ile de çözülür (O(n log n)) ama bekleyenler zaten sıralı olduğu için gereksizdir.

### Cevap 1: yığın (O(n) zaman, O(n) alan)
```c
void waitDays_stack(const int *t, int n, int *out)
{
    int *st = malloc((size_t)n * sizeof *st);       /* bekleyen günlerin İNDEKSLERİ */
    int top = 0;
    if (st == NULL) return;
    memset(out, 0, (size_t)n * sizeof *out);

    for (int i = 0; i < n; i++) {
        while (top > 0 && t[st[top - 1]] < t[i]) {  /* bugün, sondaki bekleyenden sıcak */
            int j = st[--top];
            out[j] = i - j;
        }
        st[top++] = i;                              /* bugün de bekleyene girer */
    }
    free(st);
}
```

### Cevap 2: ek bellek yok (sağdan sola, "komşuya sor")
**Basit açıklama:** Sağdan sola gidin. Sağdaki her günün `out` değeri hazır: "benden daha sıcak ilk gün şu kadar uzakta". Bir günün cevabını bulurken sağ komşuya bakın. Komşu benden **sıcak değilse**, ona sorun: "senden sıcak ilk gün nerede?" ve oraya **zıplayın** (aradaki günler komşudan soğuk, o da benden sıcak değil, onlara bakmaya gerek yok). Komşu "bende yok" derse bende de yok.

```
t = 73 71 69 72 76
gün 1 (71): komşu gün 2 (69) sıcak değil. Gün 2'ye sor: 1 gün sonra → gün 3 (72). Sıcak. cevap 3-1 = 2
```
```c
void waitDays_nomem(const int *t, int n, int *out)
{
    for (int i = n - 1; i >= 0; i--) {
        int j = i + 1;

        while (j < n && t[j] <= t[i]) {             /* aday j benden sıcak değil */
            if (out[j] == 0) {                      /* j'den sonra j'den sıcak gün yok → bende de yok */
                j = n;
                break;
            }
            j += out[j];                            /* j'nin daha sıcak gününe zıpla */
        }

        out[i] = (j < n) ? j - i : 0;
    }
}
```
`out`'u önceden sıfırlamaya gerek yok: sadece sağdaki (zaten yazılmış) değerler okunuyor.

### Karmaşıklık ve tuzaklar
- İkisi de **O(n)** zaman. Birincisi O(n) ek bellek, ikincisi **O(1)**.
- "Daha sıcak" **kesin** büyük demek (`<`, `<=` farkına dikkat).
- Kalıp: **"sonraki daha büyük/küçük eleman"** → monotonic stack.

---

## B3. Tekrar eden sensör paketlerini ele — Kolay

**Soru:** ID akışında her ID'nin **sadece ilk görünümünü** koruyun, sırayı bozmayın.

```
ids = [7, 3, 7, 9, 3, 3, 1]  →  [7, 3, 9, 1]
```

### Basit açıklama
**Girdiyi baştan sona, sırayla gezin.** Her ID için hash set'e bakın: daha önce **görülmemişse** hem sete ekleyin hem **hemen çıktıya yazın**. Görülmüşse atlayın. Sırayı hash tablosu değil **girdi dizisinin kendisi** korur (tabloyu gezmeye gerek yok, zaten gezilemez, çünkü tablo sırayı tutmaz).

### Cevap
```c
/* key tabloda yoksa ekler ve true döner (yeni). Varsa false. Tek yoklamada hem arar hem ekler. */
static bool insert_if_absent(cell_t *table, int key, uint32_t mask)
{
    for (uint32_t i = 0; i <= mask; i++) {
        cell_t *c = &table[hash_slot(key, mask, i)];
        if (!c->isFilled) { c->isFilled = true; c->key = key; return true; }
        if (c->key == key) return false;
    }
    return false;
}

int dedupePackets(const int *ids, int n, int *out)
{
    if (ids == NULL || out == NULL || n <= 0) return 0;

    uint32_t cap = 2;
    while (cap < 2u * (uint32_t)n) cap <<= 1;
    uint32_t mask = cap - 1;

    cell_t *table = calloc(cap, sizeof *table);
    if (table == NULL) return -1;

    int m = 0;
    for (int i = 0; i < n; i++) {
        if (insert_if_absent(table, ids[i], mask))
            out[m++] = ids[i];                    /* ilk görünüm */
    }
    free(table);
    return m;
}
```

### Karmaşıklık ve tuzaklar
- **O(n)** ortalama zaman, O(n) alan. Kaba çözüm (her yeni ID için `out` içinde ara) O(n²) ama **ek bellek O(1)** (`out` zaten gerekli). Bellek çok kısıtlı ve `n` küçükse mantıklı.
- ID `0` geçerli: "boş" işareti olarak `key == 0` kullanmayın, ayrı `isFilled` bayrağı kullanın.
- Hash O(1), dengeli ağaç O(log n): ikisini karıştırmayın.

---

## B4. En sık görülen K hata kodu — Orta

**Soru:** Hata kodları listesinden **en sık görülen K kodu** döndürün (O(n log K), sıralama yok).

```
codes = [1, 1, 1, 2, 2, 3], k = 2  →  [1, 2]
```

### Basit açıklama
İki aşama, **sırası önemli**:

1. **Önce hepsini say** (hash map: kod → kaç kez).
2. **Sonra** her farklı kodun (kod, sayaç) çiftine bak ve **K boyutlu bir min heap** tut: heap'te en sık görülen K kod var, **kökte bunların en zayıfı**. Yeni gelenin sayacı kökten büyükse kökü atıp yenisini koy.

**Neden sayımı bitirip sonra heap?** Heap'e her gelişte yazarsanız aynı kod heap'e **birden çok kez** girer (eski sayaçla) ve güncelleyemezsiniz. Önce sayıp sonra eklerseniz her kod heap'e **en fazla bir kez** girer.

### Cevap
```c
typedef struct { int count; int code; } hitem_t;     /* min heap: anahtar = count */

static void sift_up(hitem_t *h, int i)
{
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h[p].count <= h[i].count) break;
        hitem_t t = h[p]; h[p] = h[i]; h[i] = t;
        i = p;
    }
}

static void sift_down(hitem_t *h, int size, int i)
{
    for (;;) {
        int l = 2 * i + 1, r = 2 * i + 2, m = i;
        if (l < size && h[l].count < h[m].count) m = l;
        if (r < size && h[r].count < h[m].count) m = r;
        if (m == i) return;
        hitem_t t = h[i]; h[i] = h[m]; h[m] = t;
        i = m;
    }
}

int topKFrequent(const int *codes, int n, int k, int *out)
{
    if (codes == NULL || out == NULL || n <= 0 || k <= 0) return 0;

    uint32_t cap = 2;
    while (cap < 2u * (uint32_t)n) cap <<= 1;
    uint32_t mask = cap - 1;

    cell_t *table = calloc(cap, sizeof *table);
    hitem_t *heap = malloc((size_t)k * sizeof *heap);
    if (table == NULL || heap == NULL) { free(table); free(heap); return -1; }

    for (int i = 0; i < n; i++)                          /* Geçiş 1: sayımlar bitsin */
        if (!hash_add(table, codes[i], mask)) { free(table); free(heap); return -1; }

    int size = 0;
    for (uint32_t s = 0; s < cap; s++) {                 /* Geçiş 2: farklı her kod */
        if (!table[s].isFilled) continue;
        if (size < k) {
            heap[size] = (hitem_t){table[s].count, table[s].key};
            sift_up(heap, size);
            size++;
        } else if (table[s].count > heap[0].count) {     /* kökten (en zayıf) daha sık */
            heap[0] = (hitem_t){table[s].count, table[s].key};
            sift_down(heap, size, 0);
        }
    }

    for (int i = 0; i < size; i++) out[i] = heap[i].code;
    free(table); free(heap);
    return size;
}
```

### Karmaşıklık ve tuzaklar
- **O(n log K)** zaman, bellek: tablo O(D) + heap O(K).
- "En büyük K" için **min** heap kullanılır (kök = en zayıf, atması kolay).
- Sayım bitmeden heap'e yazmayın.
- Gerçek bir **akış** olsaydı: heap'te kodun konumunu hash map'te tutup sayaç artınca `sift_down` yapmak ya da yaklaşık algoritmalar (Misra-Gries). Alternatif: kova sıralama O(n) zaman, O(n) bellek.

---

## B5. Akıştan medyan — Zor

**Soru:** Her yeni ölçümden sonra, o ana kadarki tüm ölçümlerin **medyanını** verin (`add` O(log n), `median` O(1)).

```
add(5) → 5 · add(15) → 10 · add(1) → 5 · add(3) → 4
```

### Basit açıklama
Sıralı listenin **ortasına** hızlı erişmek istiyoruz. Listeyi **iki yarıya** bölün:

- **Küçük yarı:** sayıların küçük yarısı. Burada **en büyüğü** hızlıca bilmek istiyoruz → **max heap**.
- **Büyük yarı:** büyük yarısı. Burada **en küçüğü** bilmek istiyoruz → **min heap**.

Medyan ikisinin **tepelerinde**: eleman sayısı tekse boyutu büyük olan heap'in tepesi; çiftse iki tepenin ortalaması.

**Ekleme:** Yeni değer `küçük yarının tepesinden küçük/eşitse` küçük yarıya, değilse büyük yarıya. Sonra boyutlar 1'den fazla ayrılmışsa, fazla olanın tepesini diğerine taşı.

```
5 gelir → küçük:[5]                         medyan 5
15 gelir → küçük:[5] büyük:[15]            medyan (5+15)/2 = 10
1 gelir → küçük:[5,1] büyük:[15]           medyan = küçüğün tepesi = 5
3 gelir → küçük:[3,1] büyük:[5,15]*         (5 taşındı) medyan (3+5)/2 = 4
```
Eşit değerler için **sayaç veya özel durum gerekmez**: heap aynı değeri birden çok kez tutabilir.

### Cevap (iki heap)
```c
#define MAX_N    100000
#define HEAP_CAP (MAX_N / 2 + 1)

typedef struct {
    int  a[HEAP_CAP];
    int  size;
    bool is_max;                          /* true: max heap, false: min heap */
} heap_t;

static bool before(const heap_t *h, int x, int y)       /* x, y'den önce (daha yukarıda) mı? */
{
    return h->is_max ? x > y : x < y;
}

static void sift_up(heap_t *h, int i)
{
    while (i > 0) {
        int p = (i - 1) / 2;
        if (!before(h, h->a[i], h->a[p])) break;
        int t = h->a[i]; h->a[i] = h->a[p]; h->a[p] = t;
        i = p;
    }
}

static void sift_down(heap_t *h, int i)
{
    for (;;) {
        int l = 2 * i + 1, r = 2 * i + 2, m = i;
        if (l < h->size && before(h, h->a[l], h->a[m])) m = l;
        if (r < h->size && before(h, h->a[r], h->a[m])) m = r;
        if (m == i) return;
        int t = h->a[i]; h->a[i] = h->a[m]; h->a[m] = t;
        i = m;
    }
}

static void heap_push(heap_t *h, int v)
{
    h->a[h->size] = v;
    sift_up(h, h->size);
    h->size++;
}

static int heap_pop(heap_t *h)            /* size > 0 olmalı */
{
    int top = h->a[0];
    h->a[0] = h->a[--h->size];
    sift_down(h, 0);
    return top;
}

static int heap_top(const heap_t *h) { return h->a[0]; }

static heap_t lo = { .is_max = true  };   /* küçük yarı: tepesi en büyük */
static heap_t hi = { .is_max = false };   /* büyük yarı: tepesi en küçük */

void med_init(void)
{
    lo.size = 0; lo.is_max = true;
    hi.size = 0; hi.is_max = false;
}

void med_add(int x)
{
    if (lo.size + hi.size >= MAX_N) return;

    if (lo.size == 0 || x <= heap_top(&lo)) heap_push(&lo, x);
    else                                    heap_push(&hi, x);

    if (lo.size > hi.size + 1)      heap_push(&hi, heap_pop(&lo));    /* dengele */
    else if (hi.size > lo.size + 1) heap_push(&lo, heap_pop(&hi));
}

double med_median(void)
{
    if (lo.size == 0 && hi.size == 0) return 0.0;
    if (lo.size > hi.size) return (double)heap_top(&lo);
    if (hi.size > lo.size) return (double)heap_top(&hi);
    return ((double)heap_top(&lo) + (double)heap_top(&hi)) / 2.0;      /* int taşması yok */
}
```

### Karmaşıklık ve tuzaklar
- `med_add`: **O(log n)**, `med_median`: **O(1)**, bellek O(n) (iki statik dizi ≈ 400 KB).
- Değişmez kural: küçük yarının her değeri ≤ büyük yarının her değeri, boyut farkı ≤ 1.
- Ortalamayı `double` ile hesaplayın (`INT_MAX + INT_MAX` taşar).
- Max heap'i `-x` ile min heap'ten türetmeyin (`-INT_MIN` taşar). Burada tek `before` fonksiyonuyla yapıldı.
- Heap bir değeri **arayamaz**: eşit değerler için "heap'te var mı" diye bakmak gerekmiyor.

### Bonus: değerler küçük bir aralıktaysa (örn. 12 bit ADC, 0..4095), histogram
**Basit açıklama:** Her değerden kaç tane geldiğini say (`count[değer]`). Medyan = sayaçları küçükten büyüğe toplarken ortadaki sıraya ulaştığınız değer. Heap yok, bellek sabit (16 KB).
```c
#define RANGE 4096u

typedef struct { uint32_t count[RANGE]; uint32_t total; } hist_t;

void hist_init(hist_t *h) { memset(h, 0, sizeof *h); }

bool hist_add(hist_t *h, uint32_t x)
{
    if (x >= RANGE) return false;
    h->count[x]++;
    h->total++;
    return true;
}

static uint32_t value_at_rank(const hist_t *h, uint32_t k)    /* sıralıda k. (0 tabanlı) elemanın değeri */
{
    uint32_t seen = 0;
    for (uint32_t v = 0; v < RANGE; v++) {
        seen += h->count[v];
        if (seen > k) return v;
    }
    return RANGE - 1;
}

double hist_median(const hist_t *h)
{
    if (h->total == 0) return 0.0;
    uint32_t lo = value_at_rank(h, (h->total - 1) / 2);       /* alt orta */
    uint32_t hi = value_at_rank(h, h->total / 2);             /* üst orta (tekte aynı) */
    return ((double)lo + (double)hi) / 2.0;
}
```
Ekleme O(1), medyan O(RANGE). Medyanı da O(1) yapan işaretçili sürüm var ama en kötü durumda ekleme O(RANGE) olur (uç değerler arasında gidip gelen veride).

---

## B6. Sıfırları sona taşı (yerinde) — Kolay

**Soru:** Sıfır olmayanların sırasını koruyarak sıfırları dizinin sonuna taşıyın, **yeni dizi açmadan**.

```
[0, 1, 0, 3, 12]  →  [1, 3, 12, 0, 0]
```

### Basit açıklama
İki işaretçi: `i` diziyi **okur**, `write` sıradaki sıfır olmayanın **yazılacağı yeri** gösterir. Sıfır olmayanı gördükçe `write` konumuna yazıp `write`'ı ilerletirsiniz. Döngü bitince `write`'tan sona kadar olan kısım sıfırdır, onları sıfırlarsınız.

`write ≤ i` her zaman olduğu için henüz okunmamış veri **ezilmez**. Sıfır sayacına gerek yok: sıfır sayısı `n - write`.

### Cevap
```c
void moveZeroes(int *a, int n)
{
    if (a == NULL || n <= 0) return;

    int write = 0;                                  /* sıradaki sıfır olmayanın yazılacağı yer */
    for (int i = 0; i < n; i++) {
        if (a[i] != 0) {
            a[write++] = a[i];
        }
    }
    memset(a + write, 0, (size_t)(n - write) * sizeof *a);     /* kalan yer = sıfırlar */
}
```

### Karmaşıklık ve tuzaklar
- **O(n)** zaman, **O(1)** ek bellek.
- Erken çıkış yok: kalan elemanların hepsinin sıfır olduğunu bilmeden döngüyü bitiremezsiniz.
- `memset` boyutu **byte** cinsinden (`sizeof` ile çarpın).
- Kalıp: **filtreleyip öne toplamak** = okuma + yazma işaretçisi (silme, tekrarları kaldırma da aynı).

---

## B7. Zamanlayıcı yöneticisi — Orta

**Soru:** Tek donanım zamanlayıcısı, çok sayıda yazılım zamanlayıcısı. `add(id, bitiş_tick)`, `next()` (en erken bitiş, O(1)), `pop_expired(now, &id)` (süresi dolmuş birini çıkar). En çok 1000 zamanlayıcı, `malloc` yok.

### Basit açıklama
"Sıradaki en erken olay" = **min heap**. Anahtar bitiş zamanı, ama hangi ID dolduğunu söylemek için heap'te **(bitiş, ID)** çifti tutulur. Kök her zaman en erken biten.

- `next`: sadece köke bak → O(1).
- `pop_expired(now)`: kökün bitişi `now`'dan **ilerideyse** hiçbiri dolmadı → `false`. Kök dolmuşsa (`bitiş ≤ now`, **eşitlik dahil**) çıkar. Çıkarınca heap hemen yeniden düzenlenir (bekleme yok), çağıran `false` dönene kadar tekrar çağırır.

**Gömülünün altın kuralı:** 32 bit tick sayacı bir gün 0'a döner. Düz `bitiş <= now` yanlış çalışır. **İşaretli farkla** karşılaştırın: `(int32_t)(now - bitiş) >= 0`.

### Cevap
```c
#define MAX_TIMERS 1000

typedef struct { uint32_t expiry; uint32_t id; } timer_t_;

static timer_t_ heap[MAX_TIMERS];
static int      size;

/* a, b'den önce mi? Tick sarılabilir: işaretli FARK ile karşılaştır. */
static bool before(const timer_t_ *a, const timer_t_ *b)
{
    int32_t d = (int32_t)(a->expiry - b->expiry);
    if (d != 0) return d < 0;
    return a->id < b->id;                    /* eşit zamanda küçük id önce */
}

static void sift_up(int i)
{
    while (i > 0) {
        int p = (i - 1) / 2;
        if (!before(&heap[i], &heap[p])) break;
        timer_t_ t = heap[i]; heap[i] = heap[p]; heap[p] = t;
        i = p;
    }
}

static void sift_down(int i)
{
    for (;;) {
        int l = 2 * i + 1, r = 2 * i + 2, m = i;
        if (l < size && before(&heap[l], &heap[m])) m = l;
        if (r < size && before(&heap[r], &heap[m])) m = r;
        if (m == i) return;
        timer_t_ t = heap[i]; heap[i] = heap[m]; heap[m] = t;
        i = m;
    }
}

void timers_init(void) { size = 0; }

bool timer_add(uint32_t id, uint32_t expiry_tick)
{
    if (size >= MAX_TIMERS) return false;    /* dolu */
    heap[size] = (timer_t_){expiry_tick, id};
    sift_up(size);
    size++;
    return true;
}

bool timer_next(uint32_t *expiry_tick)       /* O(1) */
{
    if (size == 0 || expiry_tick == NULL) return false;
    *expiry_tick = heap[0].expiry;
    return true;
}

bool timer_pop_expired(uint32_t now, uint32_t *id)
{
    if (size == 0 || id == NULL) return false;
    if ((int32_t)(now - heap[0].expiry) < 0) return false;    /* en erken bile dolmadı */
    *id = heap[0].id;
    heap[0] = heap[--size];                  /* son elemanı köke al */
    sift_down(0);
    return true;
}
```

### Karmaşıklık ve tuzaklar
- `add`, `pop_expired`: **O(log n)**. `next`: **O(1)**. Bellek sabit.
- **Tick sarılması** için işaretli fark (RTOS'larda standart). Mülakat cümlesi: *"Tick sarılabileceği için karşılaştırmayı işaretli farkla yapıyorum."*
- Eşitlik (`bitiş == now`) dolmuş sayılır.
- Heap içinden ID ile **iptal** zordur (arama O(n)). İptal gerekirse konum tablosu ya da "iptal edildi" işaretleyip pop sırasında atlamak (lazy deletion).

---

## B8. Toplamı K olan alt dizi sayısı — Orta

**Soru:** Dizide **ardışık** elemanlardan oluşan ve toplamı tam `K` olan kaç alt dizi var? Negatif sayılar olabilir.

```
[1, 2, 3], k = 5  →  1   ([2, 3])
[1, -1, 0], k = 0 →  3   ([1,-1], [0], [1,-1,0])
```

### Basit açıklama
Banka hesabı benzetmesi: günlük para hareketleri dizinin elemanları, **bakiye** de kümülatif toplam. Bir dönemde toplam değişim `K` ise, o dönemin **sonundaki bakiye, başındaki bakiyeden tam K fazladır.**

Her gün sonunda sorun: **"Daha önce kaç günde bakiyem tam (bugünkü bakiye − K) idi?"** Her eşleşme bir alt dizidir. Geçmiş bakiyeleri bir **hash map**'te tutarız: `bakiye → kaç kez görüldü`.

**Kurallar:**
1. Başlangıçta bakiye 0, tabloda `{0: 1}`.
2. Her elemanda: bakiyeyi güncelle → **önce sorgula** (`bakiye − K` tabloda kaç tane) → cevaba ekle → **sonra** bakiyeyi tabloya ekle.

Neden sliding window olmaz? Negatif sayı varsa pencereyi daraltmak toplamı artırabilir, yön belli olmaz.

Tablo gösterimi: `{0:2, 1:1}` demek "bakiye 0'ı 2 kez, bakiye 1'i 1 kez gördüm".

```
a = [1, -1, 0], k = 0
başla     p=0                      tablo {0:1}
a=1       p=1  ara 1  → 0 tane     tablo {0:1, 1:1}
a=-1      p=0  ara 0  → 1 tane     tablo {0:2, 1:1}     cevap 1
a=0       p=0  ara 0  → 2 tane     tablo {0:3, 1:1}     cevap 3
```
Sayacın neden gerekli olduğuna dikkat: son adımda iki ayrı eski gün eşleşti, iki alt dizi bulundu.

### Cevap
(Ortak hash tablosu parçası kullanılır.)
```c
int countSubarraysWithSum(const int *a, int n, int k)
{
    if (a == NULL || n <= 0) return 0;

    uint32_t cap = 2;
    while (cap < 2u * ((uint32_t)n + 1u)) cap <<= 1;         /* n+1 farklı bakiye olabilir */
    uint32_t mask = cap - 1;

    cell_t *t = calloc(cap, sizeof *t);
    if (t == NULL) return -1;

    hash_add(t, 0, mask);                                    /* boş önek: bakiye 0, 1 kez */

    int p = 0, ans = 0;
    for (int i = 0; i < n; i++) {
        p += a[i];                                           /* |p| ≤ 2·10⁷, int'e sığar */
        ans += hash_get(t, p - k, mask);                     /* ÖNCE sorgula */
        hash_add(t, p, mask);                                /* SONRA ekle */
    }
    free(t);
    return ans;
}
```

### Karmaşıklık ve tuzaklar
- **O(n)** ortalama zaman, **O(n)** alan.
- Sorgu → ekleme sırası: tersi `k = 0` iken boş alt diziyi de sayar.
- Bakiye aralığı büyük (±2·10⁷), doğrudan dizi indeksi olmaz → hash map.
- Fonksiyon alt dizilerin **sayısını** döndürür, kendilerini değil.

---

## B9. Bağlantı tablosu ve çıkarma kuralı (LRU) — Zor

**Soru:** En çok `C` bağlantı. `get(handle)` ve `put(handle, value)` O(1). Tablo doluyken yeni bağlantı gelirse **en uzun süredir kullanılmayan** atılır. `malloc` yok.

### Basit açıklama
**Masa benzetmesi:** Kullandığınız kağıdı hep **en üste** koyarsınız. Masa dolunca **en alttakini** atarsınız.

İki ihtiyaç var ve **tek yapı yetmez:**

| İhtiyaç | Çözen yapı |
|---|---|
| Handle'ı O(1) bul | **Hash map** (handle → düğüm) |
| Sırayı tut, ortadan çıkarıp en öne koy O(1), en eskiyi at O(1) | **Çift yönlü bağlı liste** (bir düğümü çıkarmak için hem önceki hem sonrakini bilmek gerek) |

- **Dizi** sırayı tutar ama taşımak O(n). **Hash map** hızlı bulur ama sırası yok. **Tekli liste** sırayı tutar ama bulmak O(n) ve ortadan çıkarmak için öncekini bilmek lazım.

```
EN YENİ  [3] ⇄ [1] ⇄ [2]  EN ESKİ
```

**`get(h)`:** map'te yoksa −1. Varsa düğümü en öne al, değerini döndür.
**`put(h, v)`:** varsa güncelle ve en öne al. Yoksa: doluysa **en eskiyi hem listeden hem map'ten at**, yeni düğümü en öne koy, map'e ekle.

`malloc` yerine: sabit düğüm havuzu, bağlantılar **dizi indeksi**, boş düğümler için bir **boş liste**. Silme olduğu için hash'te **zincirleme** (düğümde `hnext`) kullanıldı: açık adreslemede silme için "tombstone" gerekir, zincirlemede bağ kopar.

### Cevap
```c
#define MAX_CAP   1000
#define NBUCKETS  2048                 /* 2'nin kuvveti: 2^11 */
#define NIL       (-1)

typedef struct {
    int handle, value;
    int prev, next;                    /* yakınlık listesi: prev = daha yeni, next = daha eski */
    int hnext;                         /* hash zinciri: aynı kovadaki sonraki düğüm */
} node_t;

static node_t nodes[MAX_CAP];          /* düğüm havuzu (malloc yok) */
static int buckets[NBUCKETS];          /* kova -> zincirin ilk düğümü */
static int head, tail;                 /* head: en yeni, tail: en eski */
static int free_head;                  /* boş düğümler listesi (next ile bağlı) */
static int cap, count;

static int bucket_of(int handle)
{
    return (int)(((uint32_t)handle * 2654435761u) >> 21);    /* üst 11 bit: 0..2047 */
}

void conn_init(int capacity)
{
    cap = capacity < 1 ? 1 : (capacity > MAX_CAP ? MAX_CAP : capacity);
    count = 0;
    head = tail = NIL;
    for (int b = 0; b < NBUCKETS; b++) buckets[b] = NIL;
    for (int i = 0; i < MAX_CAP; i++) nodes[i].next = (i + 1 < MAX_CAP) ? i + 1 : NIL;
    free_head = 0;
}

static int find(int handle)
{
    int i = buckets[bucket_of(handle)];
    while (i != NIL && nodes[i].handle != handle) i = nodes[i].hnext;
    return i;                                                /* NIL: yok */
}

static void list_unlink(int i)         /* düğümü listeden çıkar: komşularını birbirine bağla */
{
    int p = nodes[i].prev, n = nodes[i].next;
    if (p != NIL) nodes[p].next = n; else head = n;
    if (n != NIL) nodes[n].prev = p; else tail = p;
}

static void list_push_front(int i)     /* düğümü en öne (en yeni) koy */
{
    nodes[i].prev = NIL;
    nodes[i].next = head;
    if (head != NIL) nodes[head].prev = i; else tail = i;
    head = i;
}

static void hash_remove(int i)         /* düğümü hash zincirinden çıkar */
{
    int *pp = &buckets[bucket_of(nodes[i].handle)];
    while (*pp != i) pp = &nodes[*pp].hnext;                 /* i zincirde mutlaka var */
    *pp = nodes[i].hnext;
}

static void evict_oldest(void)         /* en eskiyi at: listeden, hash'ten, havuza geri ver */
{
    int i = tail;
    list_unlink(i);
    hash_remove(i);
    nodes[i].next = free_head;
    free_head = i;
    count--;
}

int conn_get(int handle)
{
    int i = find(handle);
    if (i == NIL) return -1;
    if (i != head) {                                         /* en öne al */
        list_unlink(i);
        list_push_front(i);
    }
    return nodes[i].value;
}

void conn_put(int handle, int value)
{
    int i = find(handle);
    if (i != NIL) {                                          /* var: güncelle, en öne al */
        nodes[i].value = value;
        if (i != head) { list_unlink(i); list_push_front(i); }
        return;
    }
    if (count == cap) evict_oldest();                        /* dolu: en eskiyi at */

    i = free_head;                                           /* boş düğüm al */
    free_head = nodes[i].next;
    nodes[i].handle = handle;
    nodes[i].value  = value;
    int b = bucket_of(handle);
    nodes[i].hnext = buckets[b];                             /* zincirin başına ekle */
    buckets[b] = i;
    list_push_front(i);
    count++;
}
```

### Karmaşıklık ve tuzaklar
- `get`, `put`: **O(1)** ortalama. Bellek O(C), statik.
- **Eviction sırası:** en eskiyi önce listeden ve **hash'ten de** çıkarın, sonra yenisini ekleyin. Hash'ten silmeyi unutmak en sık hata.
- Var olan handle'a `put`: eviction olmaz, değer ve sıra güncellenir.
- Kapasite 1 ve olmayan handle'a `get` (sıra değişmez) kenar durumları.

---

## B10. Toplantı odası ihtiyacı — Orta

**Soru:** Zaman aralıkları `[start, end)` veriliyor. Hepsini çalıştırmak için **en az kaç kaynak** (oda) gerekir? (`end == start` çakışma sayılmaz.)

```
[[0,30],[5,10],[15,20]]  →  2
[[1,5],[5,8]]            →  1
```

### Basit açıklama
Cevap = **aynı anda en çok kaç toplantı sürüyor.** İki yöntem:

**Yöntem 1 (heap):** Toplantıları **başlangıç saatine göre sırala**. Her yeni toplantıda sor: *"Şu an boşalmış bir oda var mı?"* Bakılacak tek oda **en erken bitecek** olandır (min heap, anahtar = bitiş saati). `en erken bitiş <= yeni başlangıç` ise o odayı **yeniden kullan** (kökü yeni bitişle değiştir). Değilse **yeni oda aç**. Sonda heap boyutu = oda sayısı.

**Yöntem 2 (iki dizi, heap yok):** Başlangıçları ve bitişleri **ayrı ayrı sırala**. Her başlangıç için: o ana kadar **biten** toplantı varsa (`start >= ends[e]`) bir oda boşalmıştır (`e++`), yoksa yeni oda (`rooms++`).

```
heap, [0,5] [1,4] [2,7] [4,9] [6,8] [8,12] [10,14] [11,13]
[0,5]  → yeni oda   {5}
[1,4]  → yeni oda   {4,5}
[2,7]  → yeni oda   {4,5,7}
[4,9]  → 4<=4 yeniden kullan {5,7,9}   ... oda sayısı hep 3
```

### Cevap
```c
typedef struct { int start, end; } interval_t;

static int cmp_start(const void *a, const void *b)
{
    int x = ((const interval_t *)a)->start, y = ((const interval_t *)b)->start;
    return (x > y) - (x < y);                         /* x - y taşabilir */
}

static int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

/* ---------- Yöntem 1: başlangıca göre sırala + bitiş saatleri için min heap ---------- */
static void sift_down(int *h, int size, int i)
{
    for (;;) {
        int l = 2 * i + 1, r = 2 * i + 2, m = i;
        if (l < size && h[l] < h[m]) m = l;
        if (r < size && h[r] < h[m]) m = r;
        if (m == i) return;
        int t = h[i]; h[i] = h[m]; h[m] = t;
        i = m;
    }
}

static void sift_up(int *h, int i)
{
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h[p] <= h[i]) break;
        int t = h[p]; h[p] = h[i]; h[i] = t;
        i = p;
    }
}

int minResources_heap(interval_t *iv, int n)
{
    if (iv == NULL || n <= 0) return 0;

    qsort(iv, (size_t)n, sizeof *iv, cmp_start);          /* toplantıları zaman sırasına diz */

    int *heap = malloc((size_t)n * sizeof *heap);         /* her odanın şu anki bitiş saati */
    if (heap == NULL) return -1;
    int size = 0;

    for (int i = 0; i < n; i++) {
        if (size > 0 && heap[0] <= iv[i].start) {         /* en erken biten oda boşalmış */
            heap[0] = iv[i].end;                          /* yeniden kullan: kökü değiştir */
            sift_down(heap, size, 0);
        } else {                                          /* boşalan yok: yeni oda aç */
            heap[size] = iv[i].end;
            sift_up(heap, size);
            size++;
        }
    }
    free(heap);
    return size;                                          /* heap boyutu = oda sayısı */
}

/* ---------- Yöntem 2: başlangıçları ve bitişleri ayrı sırala, iki işaretçi ---------- */
int minResources_twoArrays(const interval_t *iv, int n)
{
    if (iv == NULL || n <= 0) return 0;

    int *starts = malloc((size_t)n * sizeof *starts);
    int *ends   = malloc((size_t)n * sizeof *ends);
    if (starts == NULL || ends == NULL) { free(starts); free(ends); return -1; }

    for (int i = 0; i < n; i++) { starts[i] = iv[i].start; ends[i] = iv[i].end; }
    qsort(starts, (size_t)n, sizeof *starts, cmp_int);
    qsort(ends,   (size_t)n, sizeof *ends,   cmp_int);

    int rooms = 0, e = 0;            /* e: en erken bitecek, henüz "boşaldı" sayılmamış toplantı */
    for (int s = 0; s < n; s++) {
        if (starts[s] < ends[e]) rooms++;                 /* kimse bitmedi: yeni oda */
        else                     e++;                     /* biri bitti: odası boşaldı, yeniden kullan */
    }
    free(starts); free(ends);
    return rooms;
}
```

### Karmaşıklık ve tuzaklar
- İkisi de **O(n log n)** zaman (sıralama), O(n) bellek.
- `end == start` çakışma değil: heap'te `<=`, iki dizide `<` (yeni oda) / aksi hâlde `e++`.
- `qsort` karşılaştırmasında `x - y` yazmayın (taşar).
- Sadece **sayı** isteniyorsa iki dizi yöntemi yeter. Hangi odaya atandı bilgisi lazımsa heap'e `(bitiş, oda_no)` koyun.

---

## B11. Sürücü başlatma sırası (topolojik sıralama) — Orta

**Soru:** `N` sürücü. `(a, b)` = "**b, a'dan ÖNCE** başlatılmalı". Geçerli bir sıra döndürün, döngü varsa `false`.

```
N=4, deps = [(1,0), (2,0), (3,1), (3,2)]  →  [0, 1, 2, 3]
N=2, deps = [(0,1), (1,0)]                →  false   (döngü)
```

### Basit açıklama
Sabah giyinmek gibi: çorap ayakkabıdan önce. **Hiçbir şeyi beklemeyenle başlayın**, onu bitirince ona bağlı olanlara "bir şart sağlandı" deyin. Şartları bitenler sıraya girer.

**Tutulan iki bilgi:**
1. `waiting[x]` = `x` kaç şeyi bekliyor.
2. Her `b` için "bu bitince kimlere haber vereceğim" listesi (komşuluk listesi).

**Yön dikkat:** `(a, b)`: `a`, `b`'yi **bekliyor** (`b` önce).

**Algoritma (Kahn):**
1. `waiting == 0` olanları kuyruğa koy.
2. Kuyruktan birini al, sıraya yaz.
3. Ona bağlı olanların `waiting`'ini 1 azalt. **0'a düşen** kuyruğa girer.
4. Tekrarla. Sonunda sıraya yazılan sayı `N` ise tamam, `N`'den azsa **döngü var → false**.

```
bekleyen = [0, 1, 1, 2]          (0: kimseyi beklemiyor; 1,2 → 0'ı; 3 → 1 ve 2'yi)
kuyruk [0] → 0 al, sıra [0];    1 ve 2'nin sayacı 0 olur → kuyruk [1, 2]
1 al, sıra [0,1];               3'ün sayacı 2→1
2 al, sıra [0,1,2];             3'ün sayacı 1→0 → kuyruk [3]
3 al, sıra [0,1,2,3] → N tane → geçerli
```
Döngü: `[(0,1),(1,0)]` → `waiting = [1,1]`, kuyruk baştan boş, sıraya 0 eleman yazıldı → `false`. **"Bir kısmı sıralandı" yetmez, hepsi sıralanmalı.**

Çıktı dizisi `order[]` aynı zamanda kuyruktur: `order[rd..wr)` = kuyrukta bekleyenler (ayrı kuyruk yapısı gerekmez).

### Cevap
```c
#define MAXN 1000
#define MAXM 5000

bool initOrder(int N, const int deps[][2], int m, int *order)
{
    static int waiting[MAXN];                 /* waiting[x]: x kaç şeyi bekliyor */
    static int head[MAXN];                    /* head[b]: b'ye bağlı kenarların ilki */
    static int nxt[MAXM];                     /* kenar -> aynı b'nin sonraki kenarı */
    static int to[MAXM];                      /* kenar -> b bitince haber verilecek a */

    if (N < 1 || N > MAXN || m < 0 || m > MAXM || order == NULL) return false;

    for (int i = 0; i < N; i++) { waiting[i] = 0; head[i] = -1; }

    for (int e = 0; e < m; e++) {
        int a = deps[e][0], b = deps[e][1];
        if (a < 0 || a >= N || b < 0 || b >= N) return false;
        to[e]  = a;                           /* b bitince a'ya haber */
        nxt[e] = head[b];
        head[b] = e;
        waiting[a]++;                         /* a, bir şey daha bekliyor */
    }

    int rd = 0, wr = 0;                       /* order[rd..wr) = kuyruk */
    for (int x = 0; x < N; x++)
        if (waiting[x] == 0) order[wr++] = x;

    while (rd < wr) {
        int b = order[rd++];                  /* kuyruktan al, sıraya yazılmış olur */
        for (int e = head[b]; e != -1; e = nxt[e]) {
            int a = to[e];
            if (--waiting[a] == 0) order[wr++] = a;     /* şartları bitti: kuyruğa */
        }
    }
    return wr == N;                           /* N'den azsa döngü var */
}
```

### Karmaşıklık ve tuzaklar
- **O(N + m)** zaman ve bellek.
- Kendine bağımlılık `(a, a)` döngü olarak yakalanır. Geçersiz indeksler `false`.
- `static` diziler yeniden girişli değildir (gömülüde `malloc` yerine bilinçli seçim).
- C'de `const int deps[][2]` parametresine `const`'sız dizi verirseniz `-Wpedantic` uyarı verir (eski ISO C kuralı, zararsız). Gerekirse `(const int (*)[2])` ile dönüştürün.
- Kalıp: **"önce şu, sonra bu"** + geçerli sıra veya döngü tespiti.

---

## B12. Son N örneğin tepe değeri — Zor

**Soru:** Akım sensörü sürekli örnek gönderiyor. Her yeni örnekte, **son `window` örneğin** en büyüğünü verin. `peak_add` **O(1) amortize**, `malloc` yok, bellek O(window).

```
window=3:  1,3,-1,-3,5,3,6,7  →  1,3,3,3,5,5,6,7
```

### Basit açıklama
Penceredeki `[3, 5]` için **3 bir daha asla en büyük olamaz**: 5 hem büyük, hem **daha geç** geldi, yani pencereden de daha geç çıkacak. 3'ün 5'e karşı şansı kalmadı, **atabiliriz.**

Yeni bir örnek gelince, **ondan küçük veya eşit eskileri arkadan atarız.** Listede kalanlar **büyükten küçüğe** sıralı olur, **ilk eleman = pencerenin en büyüğü.** İki uçtan atıyoruz (arkadan: küçükler, önden: pencereden çıkanlar), buna **deque** denir; gömülüde ring buffer ile yapılır. Her girdi **(kaçıncı örnek, değer)** çifti: kaçıncı örnek olduğu, pencereden çıkıp çıkmadığını anlamak için gerekli.

**Her çağrıda 4 adım:**
1. Önden at: öndeki girdi pencereden çıktıysa (`seq - girdi.seq >= window`).
2. Arkadan at: arkadaki değer yeni örnekten küçük veya eşitse (tekrarla).
3. Yeni örneği arkaya ekle.
4. Cevap = öndeki girdinin değeri.

```
window=3         gelen   liste (seq:değer, sol = ön)       döner
                 1       0:1                               1
                 3       1:3          (0:1 atıldı, ≤ 3)    3
                 -1      1:3, 2:-1                         3
                 -3      1:3, 2:-1, 3:-3                   3
                 5       4:5   (1:3 pencereden çıktı; -1 ve -3 ≤ 5 atıldı)  5
                 3       4:5, 5:3                          5
                 6       6:6   (5 ve 3 ≤ 6 atıldı)         6
                 7       7:7                               7
```

### Cevap
```c
#define DQ_CAP  64u                    /* 2'nin kuvveti, window <= DQ_CAP */
#define DQ_MASK (DQ_CAP - 1u)

typedef struct { uint32_t seq; int val; } entry_t;     /* kaçıncı örnek, değeri */

static entry_t  dq[DQ_CAP];            /* deque: ring buffer */
static uint32_t head, tail;            /* sürekli artan sayaçlar: ön ve arka */
static uint32_t seq;                   /* sıradaki örneğin numarası */
static uint32_t win;                   /* pencere boyutu */

void peak_init(int window)
{
    win = window < 1 ? 1u : (window > (int)DQ_CAP ? DQ_CAP : (uint32_t)window);
    head = tail = 0;
    seq = 0;
}

int peak_add(int sample)
{
    /* 1) Önden at: pencerenin dışına çıkan (seq farkı >= win) */
    while (tail != head && (uint32_t)(seq - dq[head & DQ_MASK].seq) >= win)
        head++;

    /* 2) Arkadan at: yeni örnekten küçük veya eşit olanlar artık max olamaz */
    while (tail != head && dq[(tail - 1u) & DQ_MASK].val <= sample)
        tail--;

    /* 3) Yeni örneği arkaya ekle */
    dq[tail & DQ_MASK] = (entry_t){seq, sample};
    tail++;
    seq++;

    /* 4) Cevap = ön */
    return dq[head & DQ_MASK].val;
}
```

### Karmaşıklık ve tuzaklar
- **Amortize O(1):** her örnek listeye bir kez girer, en fazla bir kez çıkar. Bellek ≤ `window`.
- `seq` sarılsa da `seq - girdi.seq` işaretsiz farkı doğru çıkar.
- Eşitlerde yeni olan kalır (`<=`), çünkü daha geç çıkar.
- **Minimum** için karşılaştırmayı çevirin (`>=`).
- Kalıp: **"son N elemanın en büyüğü/en küçüğü"** → monotonic deque.

---

# Kalıp Özeti (hangi soru hangi kalıp)

| Soru | Kalıp | Yapı |
|---|---|---|
| A1 Two Sum | tamamlayıcı ara | hash map |
| A2 Parantez | iç içe eşleşme | stack |
| A3 İlk tekil | iki geçiş | hash map + dizi |
| A4 UART tampon | sabit bellek, akış | ring buffer |
| A5 Derinlik / seviye | ağaç | DFS / kuyruk (BFS) |
| A6 K log birleştir | K sıralı kaynak | min heap |
| B1 Aralık toplamı | BST kuralıyla dal atla | BST |
| B2 Daha sıcak gün | sonraki büyük eleman | monotonic stack |
| B3 Tekrar eden paket | ilk görünüm | hash set |
| B4 En sık K | sayım + top K | hash map + min heap (K) |
| B5 Akıştan medyan | iki yarı | max heap + min heap |
| B6 Sıfırları taşı | filtrele, öne topla | iki işaretçi |
| B7 Zamanlayıcı | sıradaki en erken olay | min heap |
| B8 Alt dizi toplamı K | bakiye (prefix sum) | hash map |
| B9 Bağlantı tablosu | O(1) bul + O(1) sıra | hash map + çift yönlü liste |
| B10 Toplantı odası | zaman sırasıyla işle | sıralama + min heap veya iki dizi |
| B11 Başlatma sırası | bağımlılık sırası | graf + kuyruk (Kahn) |
| B12 Tepe değer | pencere en büyüğü | monotonic deque |
