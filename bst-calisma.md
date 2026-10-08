# BST (İkili Arama Ağacı) Çalışma Notu

Önce basit açıklama, sonra kod. Sonunda kendin deneyeceğin alıştırmalar ve cevapları var. Bu nottaki kodlar derlendi ve rastgele ağaçlarla kaba çözümle karşılaştırıldı (ASan/UBSan açık).

**İçindekiler**
1. BST nedir?
2. Ağacın şekli ekleme sırasına bağlıdır
3. Arama
4. Ekleme
5. En küçük / en büyük
6. Silme (3 durum)
7. Ardıl, öncül, floor, ceiling
8. K. en küçük
9. BST mi? (doğrulama)
10. En düşük ortak ata (LCA)
11. Gezintiler
12. Alıştırmalar
13. Alıştırma cevapları
14. Gömülü sistem notları ve sık hatalar

Hepsinde kullanılan düğüm yapısı:

```c
typedef struct Node { int val; struct Node *left, *right; } Node;
```

---

## 1. BST nedir?

**Kural:** Her düğüm için
- **sol alt ağacın tüm değerleri** < düğümün değeri
- **sağ alt ağacın tüm değerleri** > düğümün değeri

Kural sadece **doğrudan çocuklar** için değil, **bütün alt ağaç** için geçerli.

```
          8
        /   \
       3     10          Her düğümün solu küçük, sağı büyük.
      / \      \         3'ün altındaki hepsi (1, 6, 4, 7) 8'den küçük.
     1   6      14       10'un altındaki hepsi (14, 13) 8'den büyük.
        / \     /
       4   7   13
```

**Neden kullanılır?** Arama kuralı basit: aradığın değer düğümden **küçükse sola, büyükse sağa** git. Her adımda aday kümenin yarısını atarsın (ağaç dengeliyse): **O(log n)**.

**En önemli özellik:** BST'nin **inorder gezisi** (sol, kök, sağ) değerleri **artan sırada** verir. Yukarıdaki ağaç için: `1 3 4 6 7 8 10 13 14`.

---

## 2. Ağacın şekli ekleme sırasına bağlıdır

Aynı değerler, farklı sırayla eklenince **farklı ağaç** çıkar.

```
3, 1, 5, 2, 4 sırasıyla:          1, 2, 3, 4, 5 sırasıyla:

      3                           1
     / \                           \
    1   5                           2
     \  /                            \
      2 4                             3
                                       \
 yükseklik 3 (dengeli)                  4
                                         \
                                          5
                                   yükseklik 5 (zincir, "dejenere")
```

Zincirde her işlem **O(n)** olur (bağlı liste gibi). Dengeli ağaçta **O(log n)**. Bu yüzden pratikte kendini dengeleyen ağaçlar vardır (AVL, Kırmızı-Siyah): ekleme/silmede döndürme (rotation) ile dengeyi korurlar. Mülakatta isimlerini ve bu amacı bilmek yeterli.

**Yükseklik** `h`: kökten en derin yaprağa kadar düğüm sayısı (burada tek düğüm = 1, boş ağaç = 0). BST işlemleri **O(h)**.

---

## 3. Arama

**Basit açıklama:** Köke başla. Aradığın değer düğümden küçükse sola, büyükse sağa. Bulursan bitti. `NULL`'a ulaşırsan yok.

Örnek ağaç (bölüm 1), `7` ara: `8` → (7 < 8) sola → `3` → (7 > 3) sağa → `6` → (7 > 6) sağa → `7` bulundu. **4 adım**, ağacın 9 düğümünden sadece 4'üne baktık.
`5` ara: `8` → `3` → `6` → (5 < 6) sola → `4` → (5 > 4) sağa → `NULL`. Yok.

```c
Node *bst_search(Node *root, int key)
{
    while (root && root->val != key)
        root = (key < root->val) ? root->left : root->right;
    return root;                    /* bulunamazsa NULL */
}
```
**Karmaşıklık:** O(h) zaman, O(1) alan.

---

## 4. Ekleme

**Basit açıklama:** Aramayla aynı yolu izle. `NULL`'a ulaştığın yer, yeni düğümün **tam yeridir**. Yeni düğüm **her zaman yaprak olarak** eklenir, mevcut düğümler yer değiştirmez.

`8, 3, 10, 1, 6, 14, 4, 7, 13` sırasıyla ekleyelim:

| Eklenen | Yol | Konum |
|---|---|---|
| 8 | ağaç boş | kök |
| 3 | 8'den küçük → sol | 8'in solu |
| 10 | 8'den büyük → sağ | 8'in sağı |
| 1 | 8 → sol, 3 → sol | 3'ün solu |
| 6 | 8 → sol, 3 → sağ | 3'ün sağı |
| 14 | 8 → sağ, 10 → sağ | 10'un sağı |
| 4 | 8 → sol, 3 → sağ, 6 → sol | 6'nın solu |
| 7 | 8 → sol, 3 → sağ, 6 → sağ | 6'nın sağı |
| 13 | 8 → sağ, 10 → sağ, 14 → sol | 14'ün solu |

Sonuç (bölüm 1'deki ağaç):
```
          8
        /   \
       3     10
      / \      \
     1   6      14
        / \     /
       4   7   13
```

**Kod (işaretçinin işaretçisi ile, baş düğüm özel durumu yok):**
```c
/* n->left = n->right = NULL olmalı. Yinelenen değer yok sayılır. */
Node *bst_insert(Node *root, Node *n)
{
    Node **pp = &root;                              /* "şu an bakılan bağlantının adresi" */
    while (*pp) {
        if (n->val < (*pp)->val)      pp = &(*pp)->left;
        else if (n->val > (*pp)->val) pp = &(*pp)->right;
        else return root;                           /* yinelenen */
    }
    *pp = n;                                        /* boş bağlantıya yeni düğümü as */
    return root;
}
```
`pp`, kökten başlayarak "hangi bağlantıya bakıyorum" adresini tutar. Boş bir bağlantıya ulaşınca `*pp = n` ile yeni düğümü oraya bağlarız. Ağaç boşsa `*pp` zaten `root`'tur, ayrı `if (root == NULL)` yazmaya gerek kalmaz.

**Özyinelemeli eşdeğeri:**
```c
Node *bst_insert_rec(Node *root, Node *n)
{
    if (root == NULL) return n;
    if (n->val < root->val)      root->left  = bst_insert_rec(root->left,  n);
    else if (n->val > root->val) root->right = bst_insert_rec(root->right, n);
    return root;
}
```
**Karmaşıklık:** O(h). Özyinelemeli sürüm O(h) yığın kullanır (dejenere ağaçta yığın taşabilir).

---

## 5. En küçük / en büyük

**Basit açıklama:** En küçük = **en soldaki** düğüm (hep sola git). En büyük = **en sağdaki**.

```c
Node *bst_min(Node *root) { while (root && root->left)  root = root->left;  return root; }
Node *bst_max(Node *root) { while (root && root->right) root = root->right; return root; }
```
Bölüm 1'deki ağaçta en küçük `1` (8 → 3 → 1), en büyük `14` (8 → 10 → 14).

---

## 6. Silme

En zor işlem, ama sadece **3 durum** var. Silinecek düğüme `X` diyelim.

### Durum 1: `X` yaprak (çocuğu yok)
Doğrudan sil, ebeveyninin bağlantısını `NULL` yap.

```
Önce:  3                  1 sil     Sonra: 3
      / \                                     \
     1   6                                     6
```

### Durum 2: `X`'in tek çocuğu var
`X`'i sil, **çocuğunu onun yerine** bağla.

```
14 sil (tek çocuğu 13):
        10                          10
          \                           \
           14            →             13
          /
         13
```

### Durum 3: `X`'in iki çocuğu var
Burası kritik. Yerine geçecek değer, sıralamayı bozmayan olmalı. İki aday:
- **Ardıl (successor):** sağ alt ağacın **en küçüğü** (kendisinden hemen sonra gelen değer).
- **Öncül (predecessor):** sol alt ağacın en büyüğü.

Biz **ardılı** kullanıyoruz:
1. `X`'in sağ alt ağacındaki en küçüğü bul (`S`). `S`'in **sol çocuğu olamaz** (yoksa daha küçük olurdu).
2. `S`'in değerini `X`'in düğümüne **kopyala**.
3. Sağ alt ağaçtan `S`'i **sil** (bu artık Durum 1 veya 2, kolay).

```
Orijinal ağaçta 3'ü sil (iki çocuğu: 1 ve 6).
Sağ alt ağaç: 6'nın altı (4, 6, 7). En küçüğü: 4  (ardıl).
4'ü 3'ün yerine kopyala, sağ alt ağaçtan eski 4'ü sil:

          8                              8
        /   \                          /   \
       3     10                       4     10
      / \      \         →           / \      \
     1   6      14                  1   6      14
        / \     /                        \     /
       4   7   13                         7   13
```
Sonuç hâlâ geçerli bir BST (inorder: `1 4 6 7 8 10 13 14`).

Kök silmek de aynı kural: orijinal ağaçta `8`'i silersek ardıl, sağ alt ağacın en küçüğü `10`. `10`'u köke kopyalarız, sağ alt ağaçtan eski 10'u sileriz (tek çocuğu 14 → Durum 2). Yeni kök `10`, preorder `10 3 1 6 4 7 14 13`.

```c
Node *bst_delete(Node *root, int key)
{
    if (root == NULL) return NULL;
    if (key < root->val)      root->left  = bst_delete(root->left,  key);
    else if (key > root->val) root->right = bst_delete(root->right, key);
    else {                                          /* silinecek düğüm bulundu */
        if (root->left == NULL)  { Node *r = root->right; free(root); return r; }   /* Durum 1 ve 2 */
        if (root->right == NULL) { Node *l = root->left;  free(root); return l; }   /* Durum 2 */
        Node *s = bst_min(root->right);             /* Durum 3: ardıl */
        root->val = s->val;                         /* değeri kopyala */
        root->right = bst_delete(root->right, s->val);   /* eski ardılı sil */
    }
    return root;                                    /* alt ağacın yeni kökü */
}
```
**Okuma kılavuzu:** Fonksiyon alt ağacın **yeni kökünü döndürür**; çağıran `root->left = bst_delete(root->left, key)` ile bağı günceller. Bu yüzden silinen düğümün yerine çocuğunu bağlamak bedavaya gelir.

**Yaprak ve tek çocuklu durumlar tek koşulda toplandı:** `left == NULL` ise `right`'ı (yaprakta o da `NULL`) döndürmek hem Durum 1'i hem Durum 2'yi çözer.

**Karmaşıklık:** O(h).

> **Gömülü notu:** Değeri kopyalamak, düğümlere **dışarıdan işaretçi tutuluyorsa** (örneğin bir düğüm havuzunda handle'lar) tehlikelidir: işaretçi aynı kalır ama düğümün içindeki değer değişir. O durumda ardıl düğümünü ağaçtan **sökerek** `X`'in yerine bağlamak gerekir. Mülakatta standart cevap değer kopyalamadır, bu notu ek olarak söyleyebilirsin.

---

## 7. Ardıl, öncül, floor, ceiling

Dört ilgili soru, hepsi aynı kalıp: aşağı inerken **"aday"** sakla.

| Fonksiyon | Anlamı |
|---|---|
| `successor(k)` | `k`'dan **kesin büyük** en küçük değer |
| `predecessor(k)` | `k`'dan **kesin küçük** en büyük değer |
| `floor(k)` | `k`'dan **küçük veya eşit** en büyük değer |
| `ceiling(k)` | `k`'dan **büyük veya eşit** en küçük değer |

**Basit açıklama (successor):** Köke inerken, düğüm `k`'dan **büyükse** bu düğüm bir adaydır (kaydet) ama daha küçük bir aday sol tarafta olabilir, **sola git**. Düğüm `k`'dan küçük veya eşitse cevap sağ tarafta, **sağa git**. Sonunda sakladığın son aday cevaptır.

Bölüm 1'deki ağaçta `successor(6)`: 8 > 6 aday=8, sola → 3 ≤ 6 sağa → 6 ≤ 6 sağa → 7 > 6 aday=7, sola → NULL. Cevap **7**.

```c
const Node *bst_successor(const Node *root, int key)
{
    const Node *succ = NULL;
    while (root) {
        if (key < root->val) { succ = root; root = root->left; }
        else                   root = root->right;
    }
    return succ;
}

const Node *bst_predecessor(const Node *root, int key)
{
    const Node *pred = NULL;
    while (root) {
        if (key > root->val) { pred = root; root = root->right; }
        else                   root = root->left;
    }
    return pred;
}

const Node *bst_floor(const Node *root, int key)        /* <= key olan en büyük */
{
    const Node *best = NULL;
    while (root) {
        if (root->val == key) return root;
        if (root->val < key) { best = root; root = root->right; }
        else                   root = root->left;
    }
    return best;
}

const Node *bst_ceiling(const Node *root, int key)      /* >= key olan en küçük */
{
    const Node *best = NULL;
    while (root) {
        if (root->val == key) return root;
        if (root->val > key) { best = root; root = root->left; }
        else                   root = root->right;
    }
    return best;
}
```
Hepsi **O(h)**, ek bellek O(1). `key` ağaçta olmak zorunda değil. Cevap yoksa `NULL`.

---

## 8. K. en küçük eleman

**Basit açıklama:** Inorder gezi değerleri **artan sırada** verir. Yani gezerken K. düğümde dur, bu cevap.

```c
#define MAX_DEPTH 64
bool bst_kth_smallest(const Node *root, int k, int *out)    /* k: 1 tabanlı */
{
    const Node *stack[MAX_DEPTH];
    int top = 0;
    const Node *cur = root;

    while (cur != NULL || top > 0) {
        while (cur) {                         /* mümkün olduğunca sola in */
            if (top >= MAX_DEPTH) return false;
            stack[top++] = cur;
            cur = cur->left;
        }
        cur = stack[--top];                   /* en soldaki düğüm */
        if (--k == 0) { *out = cur->val; return true; }     /* K. ziyaret */
        cur = cur->right;
    }
    return false;                             /* k > düğüm sayısı */
}
```
Bölüm 1 ağacı için `k = 3`: inorder `1, 3, 4, ...` → **4**. Süre O(h + k), açık yığın sayesinde özyineleme yok.

---

## 9. BST mi? (doğrulama)

**Soru:** Verilen ikili ağaç geçerli bir BST mi?

### Yaygın hata: sadece doğrudan çocuklara bakmak
"Sol çocuk < düğüm < sağ çocuk" kontrolü **yetmez**:

```
        5
       / \
      3   8             Her düğümün doğrudan çocukları doğru:
       \                3 < 5, 8 > 5, 6 > 3.
        6               Ama 6, 5'in SOLUNDA ama 5'ten BÜYÜK → geçersiz.
```
Kural bütün alt ağaç için geçerli olmalı.

### Yöntem 1: sınır taşı (aralık)
Her düğümün alabileceği değerler bir **aralıktır** `(alt, üst)`. Köke inerken aralık daralır:
- Sol çocuğa inerken **üst sınır** = düğümün değeri.
- Sağ çocuğa inerken **alt sınır** = düğümün değeri.

Her düğüm `alt < değer < üst` sağlamalı. Yukarıdaki örnekte `6`, 3'ün sağı olduğu için alt sınır 3, ama 3 de 5'in solunda olduğu için üst sınır 5. Yani `6` aralığı `(3, 5)` içinde olmalı, değil → geçersiz.

```c
static bool valid(const Node *n, long long lo, long long hi)    /* lo < değer < hi */
{
    if (n == NULL) return true;
    if (n->val <= lo || n->val >= hi) return false;
    return valid(n->left, lo, n->val) && valid(n->right, n->val, hi);
}

bool isValidBST(const Node *root) { return valid(root, LLONG_MIN, LLONG_MAX); }
```
**Neden `long long`?** Değerler `INT_MIN` / `INT_MAX` olabilir. `int` sınır kullanırsanız bu uç değerleri ayırt edemezsiniz. `long long` sınırı `int` aralığının dışında olduğu için hepsi doğru çalışır.

### Yöntem 2: inorder artan mı?
BST'nin inorder gezisi **kesin artan** olmalı. Gezerken bir önceki değeri tut, mevcut değer önceki değerden **büyük değilse** geçersiz.
```c
static bool inorder_check(const Node *n, const Node **prev)
{
    if (n == NULL) return true;
    if (!inorder_check(n->left, prev)) return false;
    if (*prev && n->val <= (*prev)->val) return false;       /* artan değil */
    *prev = n;
    return inorder_check(n->right, prev);
}

bool isValidBST_inorder(const Node *root)
{
    const Node *prev = NULL;
    return inorder_check(root, &prev);
}
```
Her ikisi de **O(n)** zaman, **O(h)** yığın. Yinelenen değer geçersiz sayılır (`<=`). Boş ağaç geçerlidir.

---

## 10. En düşük ortak ata (LCA)

**Soru:** İki düğümün en düşük (kökten en uzak) ortak atası kim?

**Basit açıklama (BST'de):** Köke başla. İki değer de düğümden **küçükse** ikisi de sol tarafta → sola git. İkisi de **büyükse** sağa git. Aksi halde (biri küçük/eşit, biri büyük/eşit, yani yollar burada ayrılıyor) **bu düğüm cevap.**

Bölüm 1 ağacında `LCA(4, 7)`: 8'de ikisi de küçük → sola → 3'te ikisi de büyük → sağa → 6'da `4 < 6 < 7`, yollar ayrılıyor → **6**.
`LCA(1, 7)`: 8'de ikisi küçük → 3'te `1 < 3 < 7`, ayrılıyor → **3**.
`LCA(4, 13)`: 8'de `4 < 8 < 13`, ayrılıyor → **8**.

```c
const Node *bst_lca(const Node *root, int p, int q)       /* p ve q ağaçta var sayılır */
{
    while (root) {
        if (p < root->val && q < root->val)      root = root->left;
        else if (p > root->val && q > root->val) root = root->right;
        else return root;
    }
    return NULL;
}
```
**O(h)** zaman, O(1) alan. (Genel ikili ağaçta, yani BST kuralı olmadan, LCA için bütün ağacı gezmek gerekir.)

---

## 11. Gezintiler (özet)

| Gezinti | Sıra | BST'de ne işe yarar |
|---|---|---|
| **Preorder** | kök, sol, sağ | Ağacı kaydetmek / kopyalamak |
| **Inorder** | sol, kök, sağ | **Sıralı çıktı**, K. en küçük, doğrulama |
| **Postorder** | sol, sağ, kök | Ağacı silmek (çocuklar önce) |
| **Seviye sırası (BFS)** | seviye seviye | Kuyrukla, genişlik odaklı |

```
          8
        /   \
       3     10
      / \      \
     1   6      14
        / \     /
       4   7   13

preorder   : 8 3 1 6 4 7 10 14 13
inorder    : 1 3 4 6 7 8 10 13 14     <- sıralı
postorder  : 1 4 7 6 3 13 14 10 8
seviye     : 8 3 10 1 6 14 4 7 13
```

```c
void preorder (const Node *n, void (*visit)(int)) { if (!n) return; visit(n->val); preorder(n->left, visit);  preorder(n->right, visit); }
void inorder  (const Node *n, void (*visit)(int)) { if (!n) return; inorder(n->left, visit);  visit(n->val);  inorder(n->right, visit); }
void postorder(const Node *n, void (*visit)(int)) { if (!n) return; postorder(n->left, visit); postorder(n->right, visit); visit(n->val); }

int bst_height(const Node *n)                    /* boş ağaç 0, tek düğüm 1 */
{
    if (n == NULL) return 0;
    int l = bst_height(n->left), r = bst_height(n->right);
    return 1 + (l > r ? l : r);
}

void tree_free(Node *n)                          /* postorder: çocuklar önce */
{
    if (!n) return;
    tree_free(n->left);
    tree_free(n->right);
    free(n);
}
```
Seviye sırası ve açık yığınla inorder için `cozumler.md` A5 ve `veri-yapilari-calisma-notu.md` bölüm 9'a bak.

---

## 12. Alıştırmalar (kendin dene, cevaplar bölüm 13'te)

**Alıştırma 1 (ekleme).** Boş bir BST'ye şu sırayla ekle: `8, 3, 10, 1, 6, 14, 4, 7, 13`. Ağacı çiz. Yüksekliği, preorder ve inorder çıktısını yaz.

**Alıştırma 2 (silme).** Alıştırma 1'deki ağaçtan sırayla şunları sil: `1`, sonra `14`, sonra `3`, sonra `8`. Her adımda hangi durum (1, 2, 3) olduğunu ve silmeden sonraki preorder'ı yaz.

**Alıştırma 3 (silme, iki çocuk).** Alıştırma 1'deki **orijinal** ağaçtan sadece `3`'ü sil (iki çocuklu). Sonra ayrı bir kopyadan sadece `8`'i (kök) sil. İkisinde de ardıl kim, ağaç nasıl görünüyor?

**Alıştırma 4 (şekil).** `1, 2, 3, 4, 5` sırasıyla ve `3, 1, 5, 2, 4` sırasıyla ekle. İkisinin yüksekliği nedir? Hangisinde arama daha yavaş?

**Alıştırma 5 (geçerli mi?).** Hangileri geçerli BST?

```
(a)      5            (b)      5            (c)      10           (d)      5
        / \                   / \                   /  \                   \
       3   8                 3   8                 5    15                  5
      / \                     \                         /  \
     1   4                     6                       6    20
```

**Alıştırma 6 (sorgular).** Alıştırma 1'in ağacında: `successor(6)`, `successor(7)`, `predecessor(10)`, `floor(5)`, `ceiling(5)`, 3. en küçük, `LCA(4, 7)`, `LCA(1, 7)`, `LCA(4, 13)` nedir?

**Alıştırma 7 (kendin yaz).** Aşağıdaki fonksiyonları bakmadan yaz:
1. düğüm sayısını döndüren `bst_count`
2. yaprak sayısını döndüren `bst_leaves`
3. bir değerin ağaçta olup olmadığını döndüren `bst_contains`
4. iki ağacın tamamen aynı olup olmadığını döndüren `same_tree`

---

## 13. Alıştırma cevapları

**Alıştırma 1.**
```
          8
        /   \
       3     10
      / \      \
     1   6      14
        / \     /
       4   7   13
```
Yükseklik **4** (8 → 3 → 6 → 4). Preorder: `8 3 1 6 4 7 10 14 13`. Inorder: `1 3 4 6 7 8 10 13 14`.

**Alıştırma 2.**

| Silinen | Durum | Ne olur | Preorder sonra |
|---|---|---|---|
| `1` | 1 (yaprak) | 3'ün solu `NULL` olur | `8 3 6 4 7 10 14 13` |
| `14` | 2 (tek çocuk: 13) | 10'un sağı 13 olur | `8 3 6 4 7 10 13` |
| `3` | 2 (1 silindiği için artık tek çocuk: 6) | 8'in solu 6 olur | `8 6 4 7 10 13` |
| `8` | 3 (kök, iki çocuk: 6 ve 10) | ardıl = sağ alt ağacın en küçüğü = `10`. Köke 10 kopyalanır, eski 10 silinir (tek çocuğu 13) | `10 6 4 7 13` |

Son ağaç:
```
      10
     /  \
    6    13
   / \
  4   7
```

**Alıştırma 3.**
- `3`'ü sil: iki çocuk (1 ve 6). Sağ alt ağaç `{6, 4, 7}`, en küçüğü `4` = ardıl. 3'ün yerine `4` yazılır, sağ alt ağaçtan eski `4` silinir (yaprak). Preorder: `8 4 1 6 7 10 14 13`.
- `8`'i (kök) sil: sağ alt ağaç `{10, 14, 13}`, en küçüğü `10` = ardıl. Köke `10` yazılır, eski `10` silinir (tek çocuğu 14, Durum 2). Preorder: `10 3 1 6 4 7 14 13`.

**Alıştırma 4.**
- `1, 2, 3, 4, 5` sırasıyla: zincir, yükseklik **5**. Her arama en kötü 5 adım (O(n)).
- `3, 1, 5, 2, 4` sırasıyla: yükseklik **3**. Daha hızlı.
- Aynı 5 değer, ama ekleme sırası ağacın performansını belirliyor. **Sıralı gelen veri BST'yi bozar.**

**Alıştırma 5.**
- (a) **Geçerli.** 1 < 3 < 4 < 5 < 8, her düğüm kuralı sağlıyor.
- (b) **Geçersiz.** `6`, 3'ün sağında ama 5'in solunda ve 5'ten büyük.
- (c) **Geçersiz.** `6`, 15'in solunda (6 < 15 doğru) ama 10'un sağ alt ağacında ve 6 < 10, yani 10'un sağında olamaz.
- (d) **Geçersiz.** Yinelenen değer (`5`'in sağında yine `5`). Yinelenen değerlere izin verilip verilmediği sorunun tanımına bağlıdır, bu kuralımıza göre geçersiz.

**Alıştırma 6.**

| Sorgu | Cevap |
|---|---|
| `successor(6)` | **7** |
| `successor(7)` | **8** |
| `predecessor(10)` | **8** |
| `floor(5)` | **4** (≤ 5 en büyük) |
| `ceiling(5)` | **6** (≥ 5 en küçük) |
| 3. en küçük | **4** (inorder: 1, 3, 4, ...) |
| `LCA(4, 7)` | **6** |
| `LCA(1, 7)` | **3** |
| `LCA(4, 13)` | **8** |

**Alıştırma 7 (iskelet cevaplar).**
```c
int bst_count(const Node *n)  { return n ? 1 + bst_count(n->left) + bst_count(n->right) : 0; }

int bst_leaves(const Node *n)
{
    if (n == NULL) return 0;
    if (n->left == NULL && n->right == NULL) return 1;
    return bst_leaves(n->left) + bst_leaves(n->right);
}

bool bst_contains(const Node *root, int key)
{
    while (root && root->val != key) root = (key < root->val) ? root->left : root->right;
    return root != NULL;
}

bool same_tree(const Node *a, const Node *b)
{
    if (a == NULL || b == NULL) return a == b;           /* ikisi de NULL ise aynı */
    return a->val == b->val && same_tree(a->left, b->left) && same_tree(a->right, b->right);
}
```

---

## 14. Gömülü sistem notları ve sık hatalar

**Gömülü:**
- **Dejenere ağaç** (sıralı gelen veri) O(n) işlem ve O(n) özyineleme derinliği demek. Özyineleme yerine açık yığın ve sınır kontrolü kullan.
- **`malloc` yerine düğüm havuzu:** sabit dizi + indeksle bağlama + boş düğüm listesi. Silinen düğüm havuza geri döner.
- Silmede **değer kopyalama** dışarıdan tutulan düğüm işaretçilerini geçersiz kılar. Gerekirse ardıl düğümünü sökerek bağla.
- Gerçek zamanlı kısıtta O(h) garantisi için **dengeli ağaç** (AVL, Kırmızı-Siyah) gerekir. Alternatif: sıralı dizi + binary search (ekleme yavaş ama arama garantili), ya da sabit boyutlu hash tablosu.
- Sabit sıralı veriyi (örneğin kalibrasyon tablosu) **flash'ta sıralı `const` dizi** olarak tutup binary search yapmak, ağaçtan hem küçük hem hızlıdır.

**Sık hatalar:**
- Doğrulamada sadece **doğrudan çocuklara** bakmak.
- Doğrulamada `int` sınırlarıyla `INT_MIN` / `INT_MAX` değerlerini kaçırmak.
- Aralık toplamında karar için **çocuğun** değerine bakmak (bulunduğun düğümün değerine bak).
- Silmede iki çocuklu durumda ardılı **sağ alt ağaçtan silmeyi** unutmak (ağaçta iki kopya kalır).
- Silmede dönüş değerini bağlamamak (`root->left = bst_delete(...)`).
- `NULL` kontrolü: `bst_min(NULL)`, boş ağaç.
- Yinelenen değer kuralını sormamak.
- Yüksekliği "kenar sayısı" mı "düğüm sayısı" mı diye sormamak (burada düğüm sayısı).
