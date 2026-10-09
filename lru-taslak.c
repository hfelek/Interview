/* ============================================================================
 *  LRU ÖNBELLEK: ÇALIŞMA TASLAĞI
 * ============================================================================
 *  SORU
 *    En çok `capacity` anahtarı tutan bir önbellek yaz. Tablo doluyken yeni bir
 *    anahtar eklenirse, EN UZUN SÜREDİR KULLANILMAYAN anahtar atılır.
 *    İki işlem de O(1) ortalama olmalı.
 *
 *      bool lru_get(LRU *c, int key, int *value);   // bulunduysa true, *value dolar, "yeni kullanıldı" olur
 *      bool lru_put(LRU *c, int key, int value);    // varsa günceller (yeni kullanıldı), yoksa ekler
 *
 *  YAPI (değiştirme, testler buna bakıyor)
 *    - ListNode / List : çift yönlü bağlı liste (head = en yeni, tail = en eski)
 *    - HashMap         : key -> ListNode*   (çakışmada zincirleme; alanlarını değiştirebilirsin)
 *    - LRU             : ikisini birleştirir
 *
 *  YAPILACAKLAR SIRASI (her aşamayı bitirince bir sonrakine geç)
 *    Aşama 1  Bağlı liste        list_unlink, list_push_front, list_move_to_front
 *    Aşama 2  Hash map           map_bucket, map_create, map_destroy, map_get, map_put, map_remove
 *    Aşama 3  LRU temel          lru_create, lru_destroy, lru_get, lru_put (atma YOK, kapasite dolmuyor)
 *    Aşama 4  Atma               evict_oldest, lru_put içinde dolu durum
 *    Aşama 5  Rastgele test      her şey birlikte, bellek sızıntısı kontrolü
 *
 *  DERLE VE ÇALIŞTIR (aşama numarasını değiştir)
 *    gcc -std=c11 -Wall -Wextra -g -fsanitize=address,undefined -DSTAGE=1 lru-taslak.c -o lru && ./lru
 *
 *  ÇÖZÜM KAĞIDI: cozumler.md, bölüm B9 ve sohbetteki "modüler hash map + liste" sürümü.
 *  Önce kendin dene, takılınca sadece o fonksiyona bak.
 *
 *  BİTİNCE KENDİNE SOR (mülakatta söyleyeceklerin)
 *    [ ] Neden iki yapı? (hash: bul, liste: sıra)             [ ] get hash'i değiştirir mi? (hayır, sadece liste)
 *    [ ] Atarken hem listeden hem hash'ten sildin mi?         [ ] Düğümde neden key saklıyoruz? (atarken hash'ten silmek için)
 *    [ ] Karmaşıklık: get/put O(1) ortalama, bellek O(kapasite)   [ ] Kenar durumlar: kapasite 1, var olan anahtara put, NULL, bellek hatası
 * ========================================================================== */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma GCC diagnostic ignored "-Wunused-function"   /* taslakta henüz kullanılmayan static fonksiyonlar uyarı vermesin; bitirince sil */

#ifndef STAGE
#define STAGE 1
#endif

/* ============================================================================
 *  1) ÇİFT YÖNLÜ BAĞLI LİSTE   (AŞAMA 1)
 * ========================================================================== */
typedef struct ListNode {
    struct ListNode *prev, *next;      /* prev: daha YENİ olan, next: daha ESKİ olan (yoksa NULL) */
    int key;                           /* bu düğümün anahtarı (atarken hash'ten silmek için lazım) */
    int value;                         /* anahtara bağlı veri */
} ListNode;

typedef struct {
    ListNode *head;                    /* en yeni kullanılan */
    ListNode *tail;                    /* en eski kullanılan (atılacak olan) */
} List;

/* n'yi listeden çıkar. n'nin kendisini silme (free yok), sadece bağları düzelt.
 * İPUCU: n'nin önceki komşusu (prev) varsa onun next'i ne olmalı? Yoksa n baştaydı, head ne olmalı?
 *        Sonraki komşu (next) için aynısını düşün: prev'i ne olmalı? Yoksa n sondaydı, tail ne olmalı? */
static void list_unlink(List *l, ListNode *n)
{
    /* TODO */
    (void)l; (void)n;
}

/* n'yi listenin BAŞINA (en yeni) koy. n listede değil varsayılır.
 * İPUCU: n->prev ve n->next ne olmalı? Eski head varsa onun prev'i ne olmalı?
 *        Liste boşsa tail ne olmalı? Sonunda head ne olmalı? */
static void list_push_front(List *l, ListNode *n)
{
    /* TODO */
    (void)l; (void)n;
}

/* "yeni kullanıldı": n zaten baştaysa hiçbir şey yapma, değilse listeden çıkar ve başa koy. */
static void list_move_to_front(List *l, ListNode *n)
{
    /* TODO: yukarıdaki iki fonksiyonu kullan */
    (void)l; (void)n;
}

/* ============================================================================
 *  2) HASH MAP: key -> ListNode*   (AŞAMA 2)
 * ========================================================================== */
typedef struct Entry {
    int key;                           /* anahtar */
    ListNode *node;                    /* o anahtarın liste düğümü */
    struct Entry *next;                /* aynı kovadaki sıradaki kayıt (zincirleme) */
} Entry;

typedef struct {
    Entry   **buckets;                 /* buckets[k] = k. kovadaki zincirin ilk kaydı (boşsa NULL) */
    uint32_t  nbuckets;                /* kova sayısı: 2'nin kuvveti */
    unsigned  shift;                   /* 32 - log2(nbuckets) */
} HashMap;

/* key hangi kovaya girer? Sonuç 0 .. nbuckets-1 olmalı. Negatif key de çalışmalı.
 * İPUCU: (uint32_t)key * 2654435761u sonucunun en üst log2(nbuckets) bitini al (>> shift). */
static unsigned map_bucket(const HashMap *m, int key)
{
    /* TODO */
    (void)m; (void)key;
    return 0;
}

/* En çok max_items kayıt tutacak bir harita oluştur. Hata olursa NULL.
 * İPUCU: kova sayısı >= 2 * max_items ve 2'nin kuvveti olsun (zincirler kısa kalır).
 *        2'den başlayıp ikiye katla, kaç kez katladığını (k) say. shift = 32 - k.
 *        buckets için calloc (hepsi NULL). Bir şey başarısız olursa önce ayırdıklarını geri ver. */
static HashMap *map_create(unsigned max_items)
{
    /* TODO */
    (void)max_items;
    return NULL;
}

/* Haritanın tüm kayıtlarını (Entry) ve kendisini serbest bırak. Liste düğümlerine DOKUNMA (onlar senin değil).
 * İPUCU: her kovanın zincirini gez. Bir Entry'yi free etmeden ÖNCE next'ini bir yere kaydet. */
static void map_destroy(HashMap *m)
{
    /* TODO */
    (void)m;
}

/* key'in liste düğümünü döndür, yoksa NULL. */
static ListNode *map_get(const HashMap *m, int key)
{
    /* TODO: key'in kovasındaki zinciri gez */
    (void)m; (void)key;
    return NULL;
}

/* key -> node kaydını ekle. key haritada YOK varsayılır. Bellek yetmezse false.
 * İPUCU: yeni Entry'yi kovanın zincirinin BAŞINA ekle. */
static bool map_put(HashMap *m, int key, ListNode *node)
{
    /* TODO */
    (void)m; (void)key; (void)node;
    return false;
}

/* key'in kaydını haritadan çıkar ve Entry'yi free et. Yoksa hiçbir şey yapma.
 * İPUCU: "Entry **pp" (işaretçinin işaretçisi) ile gezersen baş düğüm için ayrı if gerekmez:
 *        pp önce kovanın kendisini, sonra bir önceki Entry'nin next alanını gösterir. */
static void map_remove(HashMap *m, int key)
{
    /* TODO */
    (void)m; (void)key;
}

/* ============================================================================
 *  3) LRU: hash map + liste birlikte   (AŞAMA 3 ve 4)
 * ========================================================================== */
typedef struct {
    int      capacity;                 /* en çok kaç anahtar */
    int      count;                    /* şu an kaç anahtar */
    List     list;                     /* sıra: head = en yeni, tail = en eski */
    HashMap *map;                      /* key -> liste düğümü */
} LRU;

/* capacity < 1 ise NULL. Hata olursa NULL (ve sızıntı bırakma). */
LRU *lru_create(int capacity)
{
    /* TODO */
    (void)capacity;
    return NULL;
}

/* Her şeyi serbest bırak: liste düğümleri, harita, LRU'nun kendisi. NULL verilirse hiçbir şey yapma. */
void lru_destroy(LRU *c)
{
    /* TODO */
    (void)c;
}

/* En eski düğümü (list.tail) tamamen sil. Aşama 4'te lru_put içinden çağırırsın.
 * İPUCU: listeden çıkar, haritadan çıkar (anahtarı düğümün kendisinden oku!), free et, count'u azalt.
 *        Sıra önemli: free'den sonra n->key okunamaz. */
static void evict_oldest(LRU *c)
{
    /* TODO */
    (void)c;
}

/* key bulunursa true döner ve (value NULL değilse) *value dolar. Bulunan düğüm "en yeni" olur.
 * İPUCU: sadece haritaya bak (listeyi aramak için kullanma), sonra listede öne al. */
bool lru_get(LRU *c, int key, int *value)
{
    /* TODO */
    (void)c; (void)key; (void)value;
    return false;
}

/* key varsa değeri güncelle ve öne al. Yoksa ekle (dolu ise önce en eskiyi at). Bellek yetmezse false.
 * İPUCU (sıra): 1) haritada var mı?  2) varsa güncelle + öne al, bitir.
 *               3) yoksa ve count == capacity ise evict_oldest.  4) yeni düğümü malloc ile oluştur.
 *               5) haritaya ekle (başarısız olursa düğümü free et).  6) listede öne koy.  7) count++. */
bool lru_put(LRU *c, int key, int value)
{
    /* TODO */
    (void)c; (void)key; (void)value;
    return false;
}

/* ============================================================================
 *  ----------  TESTLER: BURADAN AŞAĞISINI DEĞİŞTİRME  ----------
 * ========================================================================== */
static int failed = 0;
#define CHECK(cond) do { if (!(cond)) { printf("  ✗ satır %d: %s\n", __LINE__, #cond); failed++; } } while (0)

#if STAGE >= 1
static void test_list(void)
{
    List l = {0};
    ListNode a = {0}, b = {0}, c = {0};
    a.key = 1; b.key = 2; c.key = 3;

    list_push_front(&l, &a);
    CHECK(l.head == &a && l.tail == &a);                          /* tek düğüm: hem baş hem son */
    CHECK(a.prev == NULL && a.next == NULL);
    list_push_front(&l, &b);
    list_push_front(&l, &c);                                      /* sıra: c b a */
    CHECK(l.head == &c && l.tail == &a);
    CHECK(c.prev == NULL && c.next == &b && b.prev == &c && b.next == &a && a.prev == &b && a.next == NULL);

    list_unlink(&l, &b);                                          /* ortadan çıkar: c a */
    CHECK(l.head == &c && l.tail == &a && c.next == &a && a.prev == &c);

    list_move_to_front(&l, &a);                                   /* sondakini öne al: a c */
    CHECK(l.head == &a && l.tail == &c && a.next == &c && c.prev == &a && a.prev == NULL && c.next == NULL);

    list_move_to_front(&l, &a);                                   /* zaten başta: değişmez */
    CHECK(l.head == &a && l.tail == &c && a.next == &c && c.prev == &a);

    list_unlink(&l, &a);                                          /* baştan çıkar */
    CHECK(l.head == &c && l.tail == &c && c.prev == NULL);
    list_unlink(&l, &c);                                          /* son kalanı çıkar */
    CHECK(l.head == NULL && l.tail == NULL);
}
#endif

#if STAGE >= 2
static void test_map(void)
{
    HashMap *m = map_create(8);
    CHECK(m != NULL);
    if (!m) return;

    ListNode n[8];
    for (int i = 0; i < 8; i++) n[i].key = i;

    CHECK(map_get(m, 5) == NULL);                                 /* boş haritada yok */
    for (int i = 0; i < 8; i++) CHECK(map_put(m, i * 1000 - 3, &n[i]));       /* negatif anahtar da */
    for (int i = 0; i < 8; i++) CHECK(map_get(m, i * 1000 - 3) == &n[i]);
    CHECK(map_get(m, 1) == NULL);

    map_remove(m, 2 * 1000 - 3);
    CHECK(map_get(m, 2 * 1000 - 3) == NULL);
    CHECK(map_get(m, 3 * 1000 - 3) == &n[3]);                     /* diğerleri yerinde */
    map_remove(m, 777777);                                        /* olmayanı silmek zararsız */
    CHECK(map_put(m, 2 * 1000 - 3, &n[2]) && map_get(m, 2 * 1000 - 3) == &n[2]);   /* silinen tekrar eklenir */

    for (int i = 0; i < 8; i++) map_remove(m, i * 1000 - 3);
    for (int i = 0; i < 8; i++) CHECK(map_get(m, i * 1000 - 3) == NULL);
    for (int i = 0; i < 8; i++) CHECK(map_put(m, i, &n[i]));      /* boşaltıp yeniden doldur */
    for (int i = 0; i < 8; i++) CHECK(map_get(m, i) == &n[i]);

    map_destroy(m);
    map_destroy(NULL);                                            /* NULL zararsız */
}
#endif

#if STAGE >= 3
/* listeyi baştan sona anahtar sırasıyla yaz, önce ileri sonra geri gezip tutarlılığı da denetle */
static int order(const LRU *c, int *keys, int max)
{
    int n = 0;
    for (const ListNode *p = c->list.head; p && n < max; p = p->next) keys[n++] = p->key;

    int m = 0;                                                    /* geri yönde de aynı düğümler olmalı */
    for (const ListNode *p = c->list.tail; p && m < max; p = p->prev) {
        if (m < n && keys[n - 1 - m] != p->key) { printf("  ✗ prev/next bağları tutarsız\n"); failed++; break; }
        m++;
    }
    if (m != n) { printf("  ✗ ileri (%d) ve geri (%d) düğüm sayısı farklı\n", n, m); failed++; }
    return n;
}

static void test_lru_basic(void)
{
    LRU *c = lru_create(3);
    CHECK(c != NULL);
    if (!c) return;

    int v = -1, keys[8], n;
    CHECK(!lru_get(c, 1, &v));                                    /* boş önbellekte yok */
    CHECK(lru_put(c, 1, 10) && lru_put(c, 2, 20));
    CHECK(lru_get(c, 1, &v) && v == 10);
    CHECK(lru_get(c, 2, &v) && v == 20);
    CHECK(!lru_get(c, 3, &v));
    CHECK(lru_get(c, 1, NULL));                                   /* value NULL olabilir */
    n = order(c, keys, 8);
    CHECK(n == 2 && keys[0] == 1 && keys[1] == 2);                /* en son get(1): 1 en yeni */
    lru_destroy(c);

    CHECK(lru_create(0) == NULL && lru_create(-5) == NULL);
    lru_destroy(NULL);
    CHECK(!lru_put(NULL, 1, 1));
    CHECK(!lru_get(NULL, 1, &v));
}
#endif

#if STAGE >= 4
static void test_lru_evict(void)
{
    LRU *c = lru_create(2);
    CHECK(c != NULL);
    if (!c) return;
    int v = -1, keys[8], n;

    /* soru örneği */
    CHECK(lru_put(c, 1, 10) && lru_put(c, 2, 20));
    CHECK(lru_get(c, 1, &v) && v == 10);                          /* 1 en yeni */
    CHECK(lru_put(c, 3, 30));                                     /* dolu: 2 atılır */
    CHECK(!lru_get(c, 2, &v));
    CHECK(lru_get(c, 3, &v) && v == 30);
    CHECK(lru_get(c, 1, &v) && v == 10);
    n = order(c, keys, 8);
    CHECK(n == 2 && keys[0] == 1 && keys[1] == 3);
    lru_destroy(c);

    /* var olan anahtara put: değer güncellenir, atma olmaz, sıra yenilenir */
    c = lru_create(2);
    CHECK(lru_put(c, 1, 10) && lru_put(c, 2, 20) && lru_put(c, 1, 11));
    CHECK(lru_get(c, 2, &v) && v == 20);                          /* hâlâ var, atılmadı */
    CHECK(lru_put(c, 3, 30));                                     /* şimdi en eski 1 (2'ye get yapıldı) */
    CHECK(!lru_get(c, 1, &v) && lru_get(c, 2, &v) && lru_get(c, 3, &v));
    lru_destroy(c);

    c = lru_create(2);
    CHECK(lru_put(c, 1, 10) && lru_put(c, 2, 20) && lru_put(c, 1, 11));   /* put da "yeni kullanıldı" */
    CHECK(lru_put(c, 3, 30));                                     /* en eski 2 atılır */
    CHECK(lru_get(c, 1, &v) && v == 11 && !lru_get(c, 2, &v));
    lru_destroy(c);

    /* kapasite 1 */
    c = lru_create(1);
    CHECK(lru_put(c, 5, 50) && lru_put(c, 6, 60));
    CHECK(!lru_get(c, 5, &v) && lru_get(c, 6, &v) && v == 60);
    CHECK(lru_put(c, 6, 61) && lru_get(c, 6, &v) && v == 61);
    n = order(c, keys, 8);
    CHECK(n == 1 && keys[0] == 6);
    lru_destroy(c);

    /* uç ve negatif anahtarlar */
    c = lru_create(3);
    CHECK(lru_put(c, -2147483647 - 1, 1) && lru_put(c, 2147483647, 2) && lru_put(c, -7, 3));
    CHECK(lru_get(c, -2147483647 - 1, &v) && v == 1 && lru_get(c, 2147483647, &v) && v == 2 && lru_get(c, -7, &v) && v == 3);
    lru_destroy(c);
}
#endif

#if STAGE >= 5
typedef struct { int h, v; long used; bool on; } ref_t;           /* kaba referans: zaman damgalı dizi */
static ref_t ref[256]; static int ref_cap; static long clk;
static void ref_init(int c) { ref_cap = c; clk = 0; for (int i = 0; i < 256; i++) ref[i].on = false; }
static int ref_find(int h) { for (int i = 0; i < ref_cap; i++) if (ref[i].on && ref[i].h == h) return i; return -1; }
static bool ref_get(int h, int *v) { int i = ref_find(h); if (i < 0) return false; ref[i].used = ++clk; *v = ref[i].v; return true; }
static void ref_put(int h, int v)
{
    int i = ref_find(h);
    if (i < 0) {
        int fr = -1, old = -1;
        for (int j = 0; j < ref_cap; j++) {
            if (!ref[j].on) { fr = j; break; }
            if (old < 0 || ref[j].used < ref[old].used) old = j;
        }
        i = fr >= 0 ? fr : old;
    }
    ref[i] = (ref_t){h, v, ++clk, true};
}

static void test_random(void)
{
    srand(2024);
    for (int t = 0; t < 2000; t++) {
        int cap = (t % 10 == 0) ? 200 : 1 + rand() % 8;
        int range = (t % 10 == 0) ? 600 : 1 + rand() % 40;
        int base = (t % 3 == 0) ? -20 : (t % 3 == 1 ? 0 : 1000000);           /* negatif ve büyük anahtarlar */
        LRU *x = lru_create(cap);
        CHECK(x != NULL);
        if (!x) return;
        ref_init(cap);
        for (int op = 0; op < 300; op++) {
            int h = base + rand() % range;
            if (rand() % 2) {
                int val = rand() % 1000;
                if (!lru_put(x, h, val)) { printf("  ✗ lru_put false döndü (t=%d)\n", t); failed++; lru_destroy(x); return; }
                ref_put(h, val);
            } else {
                int a = -1, b = -1;
                bool ha = lru_get(x, h, &a), hb = ref_get(h, &b);
                if (ha != hb || (ha && a != b)) { printf("  ✗ get(%d) farklı: sen %d/%d, doğrusu %d/%d (t=%d, op=%d)\n", h, ha, a, hb, b, t, op); failed++; lru_destroy(x); return; }
            }
        }
        int keys[256]; (void)order(x, keys, 256);                             /* prev/next tutarlılığı */
        lru_destroy(x);                                                        /* sanitizer sızıntıyı yakalar */
    }
}
#endif

#define RUN(n, fn, label) do { int before = failed; printf("Aşama %d: %s\n", n, label); fn(); \
    printf(failed == before ? "  ✓ tamam\n" : "  ✗ %d kontrol başarısız\n", failed - before); } while (0)

int main(void)
{
#if STAGE >= 1
    RUN(1, test_list, "bağlı liste");
#endif
#if STAGE >= 2
    RUN(2, test_map, "hash map");
#endif
#if STAGE >= 3
    RUN(3, test_lru_basic, "LRU temel (atma yok)");
#endif
#if STAGE >= 4
    RUN(4, test_lru_evict, "LRU atma, güncelleme, kapasite 1");
#endif
#if STAGE >= 5
    RUN(5, test_random, "2000 rastgele senaryo (referansla karşılaştırma)");
#endif
    printf("\n%s (başarısız kontrol: %d)\n", failed ? "HENÜZ BİTMEDİ" : "HEPSİ GEÇTİ", failed);
    return failed ? 1 : 0;
}
