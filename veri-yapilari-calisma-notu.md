# Veri Yapıları Çalışma Notu (C, gömülü sistem mülakatı için)

Bu nottaki bütün C kodları derlendi (`-Wall -Wextra -Wpedantic`) ve AddressSanitizer / UBSan açıkken test edildi.

**İçindekiler**
0. Hangi problemde hangi yapı?
1. Array ve String
2. Hash Map ve Hash Set
3. Linked List
4. Stack
5. Queue ve Ring Buffer
6. Heap (Min Heap, Max Heap)
7. Ağaç (Tree) temelleri
8. BST (İkili Arama Ağacı)
9. Ağaç gezintileri (Traversal)
10. Karmaşıklık özet tablosu
11. Sık yapılan hatalar

---

## 0. Hangi problemde hangi yapı?

Soruyu okurken şu anahtar kelimelere bakın:

| Soruda gördüğünüz | Aklınıza gelmesi gereken |
|---|---|
| "Daha önce gördüm mü?", "tekrar eden var mı?" | **Hash set** |
| "Kaç kez geçiyor?", "değer → bilgi eşle" | **Hash map** |
| "Sıralı dizide ara" | **Binary search** (dizi) |
| "Ardışık alt dizi / pencere" | **İki işaretçi / sliding window** |
| Parantez eşleme, geri alma, "en son gelen önce" | **Stack** |
| "Geliş sırasıyla işle" (FIFO), seviye seviye gezme | **Queue** (BFS) |
| Sabit bellek, ISR'dan akış, `malloc` yok | **Ring buffer** |
| "En küçük / en büyük K tane", "sürekli en küçüğü ver" | **Heap** |
| K tane sıralı listeyi birleştir | **Min heap** |
| Sık ekle/sil, indeks gerekmiyor | **Linked list** |
| Hiyerarşi, her düğümün çocukları var | **Ağaç** |
| Sıralı tutulan ve arama yapılan dinamik veri | **BST** |
| "O(1) ekle + O(1) en eskiyi at" (LRU) | **Hash map + çift yönlü liste** |

---

## 1. Array ve String

### Array (dizi)
- Ardışık bellekte aynı tipten elemanlar. Boyut sabit (C'de).
- `a[i]` erişimi **O(1)**. Ortaya ekleme/silme **O(n)** (kaydırma gerekir).
- Sıralı dizide arama **O(log n)** (binary search).

**Sık kullanılan teknikler**
1. **İki işaretçi (two pointers):** Biri baştan, biri sondan (ters çevirme, palindrom, sıralı dizide iki toplam). Ya da ikisi aynı yönde (tekrarları silme).
2. **Sliding window:** Sabit/değişken boyutlu pencere kaydırılır, toplam her adımda **güncellenir** (baştan hesaplanmaz). O(n²) → O(n).
3. **Prefix sum:** `pre[i]` = ilk i elemanın toplamı. Aralık toplamı `pre[r+1] - pre[l]` O(1).
4. **Yerinde (in-place) çalışma:** Ek dizi açmadan swap ile.

**Binary search** (en çok hata yapılan yer: `mid` taşması ve sınırlar)
```c
int bsearch_idx(const int *a, int n, int target)
{
    int lo = 0, hi = n - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;      /* (lo+hi)/2 taşabilir! */
        if (a[mid] == target) return mid;
        if (a[mid] < target) lo = mid + 1;
        else                 hi = mid - 1;
    }
    return -1;
}
```

**Sliding window örneği:** boyutu `k` olan en büyük alt dizi toplamı
```c
long long max_window_sum(const int *a, int n, int k)
{
    if (k <= 0 || k > n) return 0;
    long long sum = 0, best;
    for (int i = 0; i < k; i++) sum += a[i];
    best = sum;
    for (int i = k; i < n; i++) {
        sum += (long long)a[i] - a[i - k];   /* pencereyi kaydır */
        if (sum > best) best = sum;
    }
    return best;
}
```

**Dizi tuzakları**
- Off-by-one: `i <= n` yerine `i < n`.
- Toplam `int`'i aşabilir, `long long` kullanın.
- Diziyi fonksiyona verince **boyut kaybolur** (işaretçiye dönüşür), boyutu ayrıca geçin. `sizeof(a)` fonksiyon içinde işaretçi boyutunu verir.

### String (C'de)
- C'de string, **`'\0'` ile biten `char` dizisidir**. Ayrı bir tür yok.
- `strlen(s)` **O(n)**. Döngü koşuluna `i < strlen(s)` yazmayın (O(n²) olur), bir kez hesaplayın.
- `char *s = "abc";` **string literal**, değiştirilemez (salt okunur bellek). Değiştireceksen `char s[] = "abc";`.
- Sonlandırıcıyı unutmak en sık hata: `n` karakter için `n + 1` byte ayırın.
- `strcpy`/`strcat` taşmayı denetlemez. `strncpy` ise kaynak sığmazsa **`'\0'` koymaz**. Güvenli yol: `snprintf` veya kendi sınır kontrolünüz.
- Karşılaştırma `==` ile değil `strcmp` ile (`==` adresleri karşılaştırır).
- Karakter sayma tablosu için `char`'ı `unsigned char`'a çevirin (negatif indeks olmasın).

**Ters çevirme (iki işaretçi)**
```c
#include <string.h>
void reverse_str(char *s)
{
    size_t n = strlen(s);
    if (n < 2) return;
    for (size_t i = 0, j = n - 1; i < j; i++, j--) {
        char t = s[i]; s[i] = s[j]; s[j] = t;
    }
}
```

**Anagram (karakter sayma, O(n))**
```c
bool is_anagram(const char *a, const char *b)
{
    int cnt[256] = {0};
    for (; *a; a++) cnt[(unsigned char)*a]++;
    for (; *b; b++) cnt[(unsigned char)*b]--;
    for (int i = 0; i < 256; i++)
        if (cnt[i] != 0) return false;
    return true;
}
```
Bu bir mini hash map'tir: anahtar aralığı küçükse (256) doğrudan dizi indeksi yeterli, hash'e gerek yok.

---

## 2. Hash Map ve Hash Set

**Ne işe yarar:** Anahtara göre ortalama **O(1)** ekleme, arama, silme.

- **Hash map:** anahtar → değer (örn. ID → sayaç).
- **Hash set:** sadece anahtar var. "Bu eleman kümede mi?" sorusu için. Hash map'in değersiz hâli.

**Nasıl çalışır:** `slot = hash(anahtar) & mask`. İki anahtar aynı slota düşerse **çakışma (collision)** olur. İki çözüm:

| Yöntem | Fikir | Gömülü için |
|---|---|---|
| **Zincirleme (chaining)** | Her slot bir bağlı liste | Düğümler için `malloc` gerekir |
| **Açık adresleme (open addressing)** | Slot doluysa sıradaki slota bak (doğrusal yoklama) | Tek sabit dizi, `malloc` yok. Gömülüde tercih edilir |

**Önemli kavramlar**
- **Yük faktörü** = eleman sayısı / slot sayısı. Açık adreslemede 0.5'i aşmayın, yoklama zincirleri uzar.
- Kapasite 2'nin kuvveti → `% n` yerine `& mask` (bölmesiz).
- Ortalama O(1), **en kötü O(n)** (hepsi aynı slota düşerse).
- Açık adreslemede silme **tuhaftır:** slotu boşaltırsanız zincir kopar. "Silindi" işaretçisi (tombstone) gerekir. Silme yoksa "boş slot görünce dur" kuralı güvenli.
- Hash fonksiyonu: anahtarı karıştırmalı (bkz. `calisma-notlari.md`, Knuth çarpımsal hash).

**Hash map (sayaç) örneği, açık adresleme**
```c
typedef struct { int key; int count; bool isFilled; } cell_t;

static uint32_t hash_slot(int key, uint32_t mask, uint32_t iter)
{
    uint32_t h = (uint32_t)key * 2654435761u;   /* Knuth çarpımsal */
    h ^= h >> 16;
    return (h + iter) & mask;                   /* doğrusal yoklama */
}

/* key varsa count++, yoksa yeni hücre açıp count = 1 */
bool hash_add(cell_t *table, int key, uint32_t mask)
{
    for (uint32_t i = 0; i <= mask; i++) {
        cell_t *c = &table[hash_slot(key, mask, i)];
        if (!c->isFilled) { c->isFilled = true; c->key = key; c->count = 1; return true; }
        if (c->key == key) { c->count++; return true; }
    }
    return false;   /* tablo dolu */
}

/* key'in sayacı, yoksa 0 */
int hash_count(const cell_t *table, int key, uint32_t mask)
{
    for (uint32_t i = 0; i <= mask; i++) {
        const cell_t *c = &table[hash_slot(key, mask, i)];
        if (!c->isFilled) return 0;
        if (c->key == key) return c->count;
    }
    return 0;
}
```
Kullanım: `cap` = 2'nin kuvveti ve ≥ 2n, `calloc(cap, sizeof(cell_t))`, `mask = cap - 1`.

**Hash set** için aynı tablodan `count` alanını çıkarırsınız: `insert` (varsa dokunma), `contains`.

**Kalıplar**
- **Two Sum:** her eleman için `target - x` tabloda var mı? Sonra kendini ekle (tek geçiş).
- **Sayma:** bir geçiş sayım, ikinci geçiş sorgu ("ilk tekil" için ikinci geçişte **diziyi** gez).
- **Tekrar var mı:** hash set'e bak, yoksa ekle.

---

## 3. Linked List (Bağlı Liste)

**Ne işe yarar:** Elemanlar bellekte dağınık, her düğüm bir sonrakini işaret eder. Ortaya ekleme/silme, **yeri biliniyorsa O(1)**. Ama indekse göre erişim **O(n)**, arama O(n).

| İşlem | Tekli bağlı liste |
|---|---|
| Başa ekle | O(1) |
| Sona ekle | O(n) (tail işaretçisi tutulursa O(1)) |
| Aramak | O(n) |
| Düğümü sil (önceki biliniyorsa) | O(1) |
| `a[i]`'ye erişim | O(n) |

**Çeşitleri:** tekli (`next`), çift yönlü (`next` + `prev`), dairesel (son düğüm başa bağlı).

**Dizi mi liste mi?** Dizi: erişim hızlı, bellek ardışık (cache dostu), boyut sabit. Liste: ekleme/silme esnek, ama her düğüm işaretçi + (genelde `malloc`) maliyeti ister. Gömülüde `malloc` yerine **sabit bir düğüm havuzu (pool)** ve boş düğümler listesi (free list) kullanılır.

```c
typedef struct LNode { int val; struct LNode *next; } LNode;

LNode *list_push_front(LNode *head, LNode *n) { n->next = head; return n; }
```

**Ters çevirme** (üç işaretçi: `prev`, `head`, `next`)
```c
LNode *list_reverse(LNode *head)
{
    LNode *prev = NULL;
    while (head) {
        LNode *next = head->next;   /* sonrakini kaydet */
        head->next = prev;          /* oku çevir */
        prev = head;
        head = next;
    }
    return prev;
}
```

**Değere göre silme (işaretçinin işaretçisi, baş düğüm özel durumu yok)**
```c
LNode *list_remove(LNode *head, int val)
{
    LNode **pp = &head;
    while (*pp) {
        if ((*pp)->val == val) {
            LNode *dead = *pp;
            *pp = dead->next;
            free(dead);
            break;
        }
        pp = &(*pp)->next;
    }
    return head;
}
```
`pp`, "önceki düğümün `next` alanının adresi"dir. Baş düğümü silmek de aynı kodla olur, ayrı `if` gerekmez.

**Hızlı/yavaş işaretçi (Floyd)**
```c
bool list_has_cycle(const LNode *head)         /* döngü var mı? */
{
    const LNode *slow = head, *fast = head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) return true;
    }
    return false;
}

const LNode *list_middle(const LNode *head)    /* orta düğüm */
{
    const LNode *slow = head, *fast = head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    return slow;
}
```

**Tuzaklar:** `NULL` kontrolü, baş düğümün değişmesi (fonksiyon yeni `head`'i döndürmeli ya da `LNode **head` almalı), silerken `next`'i **önce** kaydetmek, bellek sızıntısı, döngülü listede sonsuz döngü.

---

## 4. Stack (Yığın)

**Ne işe yarar:** **LIFO** (son giren ilk çıkar). Tüm işlemler **O(1)**.
- `push(x)`: üste ekle
- `pop()`: üsttekini al
- `peek()/top()`: üsttekine bak, silme
- `isEmpty()`

**Nerede kullanılır:** parantez eşleme, geri alma (undo), DFS (özyinelemenin yerine), fonksiyon çağrı yığını, ifade değerlendirme, "sonraki büyük eleman" (monotonic stack).

**Dizi ile gerçekleştirme (sabit bellek, `malloc` yok)**
```c
#define STACK_CAP 64
typedef struct { int data[STACK_CAP]; int top; } int_stack_t;   /* top = eleman sayısı */

void stack_init(int_stack_t *s) { s->top = 0; }

bool stack_push(int_stack_t *s, int v)
{
    if (s->top >= STACK_CAP) return false;      /* taşma */
    s->data[s->top++] = v;
    return true;
}

bool stack_pop(int_stack_t *s, int *v)
{
    if (s->top == 0) return false;              /* boş */
    *v = s->data[--s->top];
    return true;
}

bool stack_peek(const int_stack_t *s, int *v)
{
    if (s->top == 0) return false;
    *v = s->data[s->top - 1];
    return true;
}
```
Not: `stack_t` adını kullanmayın, POSIX `<signal.h>` içinde zaten var.

**Tuzaklar:** boş stack'ten `pop`, dolu stack'e `push` (taşma), `top`'un "son eleman indeksi" mi "eleman sayısı" mı olduğunu karıştırmak.

---

## 5. Queue (Kuyruk) ve Ring Buffer

**Queue:** **FIFO** (ilk giren ilk çıkar). `enqueue` (sona ekle), `dequeue` (baştan al), ikisi de **O(1)**.

**Nerede kullanılır:** BFS (seviye sırası), görev sıralama, üretici-tüketici, UART/sensör verisi tamponu.

**Gerçekleştirme:**
- **Bağlı liste:** `head` ve `tail` işaretçisi. `malloc` ister.
- **Düz dizi + kaydırma:** her `dequeue` O(n). **Kullanmayın.**
- **Ring (dairesel) buffer:** sabit dizi + iki indeks. En iyisi, gömülüde standart.

### Ring buffer
Dizinin sonuna gelince başa dönen kuyruk. `malloc` yok, kopyalama yok, her işlem O(1).

**Sürekli artan sayaç yöntemi** (dolu/boş ayrımı için bir slot kaybı yok):
- `head` = toplam yazılan sayısı, `tail` = toplam okunan sayısı. İkisi **işaretsiz** (`uint32_t`).
- Eleman sayısı = `head - tail`. Boş: `head == tail`. Dolu: `head - tail >= N`.
- Dizi erişimi: `buf[sayaç & (N - 1)]`.
- **N 2'nin kuvveti olmalı:** sayaç 2³²'de sarılırken indeks bozulmasın.

```c
#define RB_SIZE 8u                      /* 2'nin kuvveti */
#define RB_MASK (RB_SIZE - 1u)
typedef struct { uint8_t buf[RB_SIZE]; uint32_t head, tail; } ring_t;

void ring_init(ring_t *r) { r->head = 0; r->tail = 0; }

bool ring_put(ring_t *r, uint8_t v)
{
    if ((uint32_t)(r->head - r->tail) >= RB_SIZE) return false;   /* dolu: ezme! */
    r->buf[r->head & RB_MASK] = v;
    r->head++;
    return true;
}

bool ring_get(ring_t *r, uint8_t *v)
{
    if (r->head == r->tail) return false;                          /* boş */
    *v = r->buf[r->tail & RB_MASK];
    r->tail++;
    return true;
}
```
Bu kod tek iş parçacığı içindir. **ISR + ana döngü** için `volatile`, bariyer ve sıralama kuralları `calisma-notlari.md` içinde (bölüm 5).

**Dolu/boş ayrımının 3 yolu**
| Yöntem | Kapasite | Not |
|---|---|---|
| Bir slotu boş bırak | N-1 | basit |
| `count` alanı | N | ISR ile ana döngü arasında yarış çıkarır |
| **Sürekli artan sayaç** | N | N 2'nin kuvveti olmalı, kilitsiz tasarıma uygun |

**Deque (çift uçlu kuyruk):** iki uçtan ekle/çıkar. Sliding window maximum gibi sorularda kullanılır.

---

## 6. Heap (Yığın, öncelik kuyruğu)

**Ne işe yarar:** "**En küçüğü (veya en büyüğü) hızlıca ver**" ihtiyacı. **Öncelik kuyruğunun (priority queue)** gerçekleştirmesi.

**Min heap kuralı:** her düğüm çocuklarından **küçük veya eşit**. Böylece en küçük eleman her zaman **kökte**.
(Max heap: tersi, her düğüm çocuklarından büyük veya eşit, kökte en büyük.)

> Heap **tam sıralı değildir.** Sadece ebeveyn–çocuk ilişkisi garantili. İki kardeş arasında sıra yok.

**Dizide saklanır** (işaretçi yok). Tam ikili ağaç şeklinde soldan sağa doldurulur. Indeks formülleri (0 tabanlı):

```
ebeveyn(i) = (i - 1) / 2
sol(i)     = 2*i + 1
sağ(i)     = 2*i + 2
```
```
Dizi: [1, 3, 2, 7, 4, 5, 9]          1
                                    /   \
                                   3     2
                                  / \   / \
                                 7   4 5   9
```

| İşlem | Süre | Nasıl |
|---|---|---|
| `peek` (en küçük) | **O(1)** | `a[0]` |
| `push` | **O(log n)** | sona ekle, **yukarı kaydır (sift up)** |
| `pop` (en küçüğü çıkar) | **O(log n)** | kökü al, **son elemanı köke koy**, **aşağı kaydır (sift down)** |
| Rastgele elemanı bul | O(n) | heap bunun için değil |
| Diziyi heap yap (build heap) | **O(n)** | `n/2 - 1`'den 0'a kadar `sift_down` |

```c
#define HEAP_CAP 128
typedef struct { int a[HEAP_CAP]; int size; } minheap_t;

static void swap_int(int *x, int *y) { int t = *x; *x = *y; *y = t; }

static void sift_up(minheap_t *h, int i)
{
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h->a[p] <= h->a[i]) break;          /* kural sağlanıyor */
        swap_int(&h->a[p], &h->a[i]);
        i = p;
    }
}

static void sift_down(minheap_t *h, int i)
{
    for (;;) {
        int l = 2 * i + 1, r = 2 * i + 2, m = i;
        if (l < h->size && h->a[l] < h->a[m]) m = l;
        if (r < h->size && h->a[r] < h->a[m]) m = r;
        if (m == i) break;                      /* kural sağlanıyor */
        swap_int(&h->a[i], &h->a[m]);
        i = m;
    }
}

bool heap_push(minheap_t *h, int v)
{
    if (h->size >= HEAP_CAP) return false;
    int i = h->size++;
    h->a[i] = v;
    sift_up(h, i);
    return true;
}

bool heap_pop(minheap_t *h, int *out)
{
    if (h->size == 0) return false;
    *out = h->a[0];
    h->a[0] = h->a[--h->size];                  /* son elemanı köke taşı */
    sift_down(h, 0);
    return true;
}

bool heap_peek(const minheap_t *h, int *out)
{
    if (h->size == 0) return false;
    *out = h->a[0];
    return true;
}

/* a[] ve size doldurulduktan sonra, O(n) */
void heap_build(minheap_t *h)
{
    for (int i = h->size / 2 - 1; i >= 0; i--)
        sift_down(h, i);
}
```
**Max heap** için karşılaştırmaları ters çevirin (`<` ↔ `>`).

**Neden `sift_down`'da iki çocuğun küçüğüyle takas edilir?** Büyük çocukla takas ederseniz, küçük olan çocuk yeni ebeveynden büyük kalır ve kural bozulur.

**Kalıplar**
- **En büyük K eleman / K. en büyük:** boyutu K olan bir **min heap** tutun. Yeni eleman, kökten büyükse kökü atıp yenisini koyun (`sift_down`). Sonunda kök = K. en büyük. Bellek O(K), süre O(n log K). (Sezgi: min heap'te "en küçük" kökte olduğundan, K'ya sığmayan en küçüğü atmak kolay.)
- **K sıralı listeyi birleştir:** her listenin başını min heap'e koy, çıkardıkça o listenin sonrakini ekle. O(n log k).
- **Heap sort:** hepsini heap'e koy, sırayla çıkar. O(n log n).
- **Medyan (iki heap):** küçük yarı = max heap, büyük yarı = min heap.

**Tuzaklar:** `size` güncellemesinden önce/sonra indeks karıştırmak, `pop`'ta boş heap kontrolü, çocuk indeksinin `size`'ı aşıp aşmadığını kontrol etmemek.

---

## 7. Ağaç (Tree) temelleri

**Ağaç:** düğümlerden oluşan hiyerarşik yapı. Bir **kök (root)** var, her düğümün **çocukları** olabilir, **döngü yok.**

**Terimler**
| Terim | Anlamı |
|---|---|
| **Kök (root)** | En üstteki düğüm |
| **Yaprak (leaf)** | Çocuğu olmayan düğüm |
| **Ebeveyn / çocuk** | Doğrudan bağlı üst / alt düğüm |
| **Derinlik (depth)** | Kökten o düğüme kenar sayısı |
| **Yükseklik (height)** | Bir düğümden en derin yaprağa kenar sayısı |
| **Seviye (level)** | Aynı derinlikteki düğümler |
| **Alt ağaç (subtree)** | Bir düğüm ve altındakiler |

> Yükseklik/derinlik tanımı kaynağa göre "kenar sayısı" veya "düğüm sayısı" olabilir. Soruda hangisini kastettiğini sorun.

**İkili ağaç (binary tree):** her düğümün en fazla **iki** çocuğu var (sol, sağ).

```c
typedef struct Node { int val; struct Node *left, *right; } Node;
```

**Çeşitleri**
- **Dengeli (balanced):** sol ve sağ alt ağaç yükseklikleri çok farklı değil. Yükseklik ≈ log n.
- **Dejenere (degenerate):** her düğümün tek çocuğu var, **bağlı liste gibi.** Yükseklik = n.
- **Tam (complete):** son seviye hariç hepsi dolu, son seviye soldan doludur. Heap'ler böyledir (dizide saklanabilir).
- **Dolu (full):** her düğümün 0 ya da 2 çocuğu var.

Ağaç üzerindeki algoritmaların maliyeti genelde **yüksekliğe (h)** bağlıdır. Dengeli ağaçta h = O(log n), dejenere ağaçta h = O(n).

---

## 8. BST (İkili Arama Ağacı)

**Kural:** Her düğüm için **sol alt ağacın tüm değerleri < düğüm < sağ alt ağacın tüm değerleri.**

(Sadece doğrudan çocuklara bakmak yetmez. Kural **bütün alt ağaç** için geçerli.)

```
        5
       / \
      3   8
     / \ / \
    1  4 7  9
```

| İşlem | Dengeli | Dejenere (en kötü) |
|---|---|---|
| Arama | O(log n) | O(n) |
| Ekleme | O(log n) | O(n) |
| Silme | O(log n) | O(n) |
| En küçük / en büyük | O(h) | O(n) |

Dengeyi korumak için **AVL, Kırmızı-Siyah (Red-Black)** gibi kendini dengeleyen ağaçlar kullanılır. Mülakatta isimlerini ve "ekleme/silmede döndürme (rotation) ile dengeyi korurlar" demeniz yeterli.

**Arama** (kolay: küçükse sola, büyükse sağa)
```c
Node *bst_search(Node *root, int key)
{
    while (root && root->val != key)
        root = (key < root->val) ? root->left : root->right;
    return root;
}
```

**Ekleme** (işaretçinin işaretçisi ile, yinelenen değerler yok sayılır)
```c
Node *bst_insert(Node *root, Node *n)       /* n->left = n->right = NULL olmalı */
{
    Node **pp = &root;
    while (*pp) {
        if (n->val < (*pp)->val)      pp = &(*pp)->left;
        else if (n->val > (*pp)->val) pp = &(*pp)->right;
        else return root;                    /* yinelenen */
    }
    *pp = n;
    return root;
}
```

**En küçük:** en soldaki düğüm. **En büyük:** en sağdaki.
```c
Node *bst_min(Node *root)
{
    while (root && root->left) root = root->left;
    return root;
}
```

**Silme** (3 durum)
1. **Yaprak:** doğrudan sil.
2. **Tek çocuklu:** düğümü çocuğuyla değiştir.
3. **İki çocuklu:** sağ alt ağacın **en küçüğünü** (inorder ardılı, successor) bul, değerini bu düğüme kopyala, sonra o en küçüğü sağ alt ağaçtan sil.

```c
Node *bst_delete(Node *root, int key)
{
    if (root == NULL) return NULL;
    if (key < root->val)      root->left  = bst_delete(root->left,  key);
    else if (key > root->val) root->right = bst_delete(root->right, key);
    else {
        if (root->left == NULL)  { Node *r = root->right; free(root); return r; }
        if (root->right == NULL) { Node *l = root->left;  free(root); return l; }
        Node *s = bst_min(root->right);          /* inorder ardılı */
        root->val = s->val;
        root->right = bst_delete(root->right, s->val);
    }
    return root;
}
```

**En önemli özellik:** BST'nin **inorder gezisi değerleri artan sırada verir.** Bu bilgi doğrulama ve "K. en küçük" sorularında kullanılır.

---

## 9. Ağaç gezintileri (Traversal)

İki aile var: **DFS (derinlik öncelikli)** ve **BFS (genişlik öncelikli)**.

### DFS: üç sıra (kökün ne zaman ziyaret edildiğine göre)
| Gezinti | Sıra | Ne işe yarar |
|---|---|---|
| **Preorder** | kök, sol, sağ | Ağacı kopyalamak/kaydetmek (serialize) |
| **Inorder** | sol, kök, sağ | **BST'de sıralı çıktı** |
| **Postorder** | sol, sağ, kök | Ağacı silmek (çocuklar önce), yükseklik hesabı |

İsim kuralı: "pre/in/post" kökün **ne zaman** ziyaret edildiğidir (önce / ortada / sonda). Sol her zaman sağdan önce.

```
        5
       / \
      3   8
     / \ / \
    1  4 7  9

preorder : 5 3 1 4 8 7 9
inorder  : 1 3 4 5 7 8 9     <- sıralı
postorder: 1 4 3 7 9 8 5
levelorder:5 3 8 1 4 7 9
```

**Özyinelemeli (recursive)** (3 sıra arasında sadece `visit` satırının yeri değişir)
```c
void preorder (const Node *n, void (*visit)(int)) { if (!n) return; visit(n->val);  preorder(n->left, visit);  preorder(n->right, visit); }
void inorder  (const Node *n, void (*visit)(int)) { if (!n) return; inorder(n->left, visit);  visit(n->val);  inorder(n->right, visit); }
void postorder(const Node *n, void (*visit)(int)) { if (!n) return; postorder(n->left, visit); postorder(n->right, visit); visit(n->val); }
```
Süre O(n), alan O(h) (çağrı yığını).

**Döngüsel (iterative) inorder, açık yığın ile**
Özyineleme yerine kendi yığınınızı kullanırsınız. Gömülüde derin ağaçta **çağrı yığını taşmasını** önler ve sınırı siz kontrol edersiniz.
```c
#define MAX_DEPTH 64
int inorder_iter(const Node *root, int *out, int cap)
{
    const Node *stack[MAX_DEPTH];
    int top = 0, n = 0;
    const Node *cur = root;

    while ((cur != NULL || top > 0) && n < cap) {
        while (cur) {                       /* mümkün olduğunca sola in */
            if (top >= MAX_DEPTH) return n; /* çok derin, kontrollü çık */
            stack[top++] = cur;
            cur = cur->left;
        }
        cur = stack[--top];                 /* en soldaki düğüm */
        out[n++] = cur->val;                /* ziyaret */
        cur = cur->right;                   /* sonra sağ alt ağaç */
    }
    return n;
}
```
Mantık: sola kadar in, yığına koy. Geri çıkarken ziyaret et, sonra sağ alt ağaca geç.

### BFS: seviye sırası (level-order)
Kuyruk kullanılır (FIFO). Düğümü çıkar, ziyaret et, çocuklarını (sol, sağ) kuyruğa ekle.
```c
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
Süre O(n), alan O(w) (en geniş seviye).

### Hangisini ne zaman?
| Amaç | Gezinti |
|---|---|
| BST'yi sıralı yazdır / doğrula / K. en küçük | **Inorder** |
| Ağacı kaydet, kopyala | **Preorder** |
| Ağacı sil, alt ağaç sonuçlarını birleştir (yükseklik, boyut) | **Postorder** |
| Seviye seviye işle, en kısa yol (kenar sayısı) | **BFS** |
| Yol bul, her dalı sonuna kadar incele | **DFS** |

**DFS vs BFS bellek:** DFS: O(yükseklik), dejenere ağaçta kötü. BFS: O(genişlik), geniş/dengeli ağaçta kötü.

**Gömülü uyarısı:** Özyineleme **yığın taşmasına** yol açabilir (derin/güvenilmeyen girdi). Derinlik sınırlı değilse açık yığınla döngü yazın ve sınırı kontrol edin. Morris geçişi O(1) bellek kullanır ama ağaç işaretçilerini geçici değiştirir. `const` veya çok bağlamlı (ISR + ana döngü) erişimde güvenli değildir.

---

## 10. Karmaşıklık özet tablosu

(Ortalama durum, aksi belirtilmedikçe. n = eleman sayısı, h = ağaç yüksekliği)

| Yapı | Erişim | Arama | Ekleme | Silme | Not |
|---|---|---|---|---|---|
| Dizi | O(1) | O(n) | O(n) | O(n) | sıralıysa arama O(log n) |
| Bağlı liste | O(n) | O(n) | O(1)* | O(1)* | *yeri biliniyorsa |
| Stack | - | - | O(1) | O(1) | LIFO |
| Queue / Ring buffer | - | - | O(1) | O(1) | FIFO |
| Hash map/set | - | O(1) | O(1) | O(1) | en kötü O(n) |
| Min heap | - | O(n) | O(log n) | O(log n) | `peek` min: O(1) |
| BST (dengeli) | - | O(log n) | O(log n) | O(log n) | dejenere: O(n) |
| BST (dejenere) | - | O(n) | O(n) | O(n) | liste gibi |

**Gezintiler:** hepsi O(n) zaman. Alan: DFS O(h), BFS O(w).

---

## 11. Sık yapılan hatalar

- **Off-by-one:** `<` / `<=`, `n` / `n-1`.
- **`NULL` kontrolü:** işaretçiyi kullanmadan önce (listede, ağaçta).
- **Bellek:** `malloc` sonucu kontrol, `free` her çıkış yolunda, serbest bıraktıktan sonra kullanmamak.
- **İşaretli taşma:** `int` toplam/çarpım taşarsa tanımsız. `long long` veya `uint32_t` kullanın.
- **Dolu/boş durumu** (stack, ring buffer, heap): önce kontrol, sonra işlem.
- **Sıra:** sayma ile arama aynı geçişte değil (hash map sayım **bitmeden** karar vermeyin).
- **Özyineleme derinliği:** gömülüde yığın taşması.
- **Tanım belirsizliği:** yükseklik (kenar mı düğüm mü), beraberlik kuralı, yinelenen değer. **Önce sorun.**
- **Her soruda söyleyin:** neden bu yapı, zaman/alan karmaşıklığı, kenar durumlar.
