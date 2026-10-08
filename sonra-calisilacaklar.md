# Sonra Çalışılacaklar

Anlaşılmayan veya tekrar bakılacak konular. Yeni madde ekledikçe bu dosya büyüyecek.

---

## 1. Topolojik sıralama (Soru 11: sürücü başlatma sırası) — ANLAŞILMADI, tekrar bak

**Soru:** `N` sürücü var. `(a, b)` = "b, a'dan ÖNCE başlatılmalı". Geçerli bir başlatma sırası ver. Döngü varsa `false`.

**Benzetme:** Sabah giyinmek. Çorap ayakkabıdan önce. Hiçbir şeyi beklemeyenle başla, onu giyince ona bağlı olanlara "bir şart sağlandı" de. Şartları bitenler sıraya girer.

**Yön dikkat:** `(a, b)` = `a`, `b`'yi **bekliyor** (`b` önce).

**Tutulan iki bilgi:**
1. `bekleyen[x]` = `x` kaç şeyi bekliyor.
2. Her `b` için: ona bağlı `a`'ların listesi (`b` bitince kimlere haber verilecek). Buna **komşuluk listesi** denir.

**Algoritma (Kahn):**
1. `bekleyen == 0` olanları kuyruğa koy.
2. Kuyruktan birini al, sıraya yaz.
3. Ona bağlı olanların `bekleyen`'ini 1 azalt. **0'a düşen** kuyruğa girer.
4. Kuyruk boşalana kadar tekrarla.
5. Sıraya yazılan sayı `N`'e eşitse tamam. `N`'den azsa **döngü var → `false`**.

**Örnek:** `N=4`, `deps = [(1,0), (2,0), (3,1), (3,2)]`

```
bekleyen = [0, 1, 1, 2]      (0 kimseyi beklemiyor; 1,2 → 0'ı; 3 → 1 ve 2'yi)

adım | kuyruk | alınan | sıra         | bekleyen
-----+--------+--------+--------------+--------------
  -  | [0]    |   -    | []           | [0, 1, 1, 2]
  1  | [0]    |   0    | [0]          | [0, 0, 0, 2]   (1,2 serbest, kuyruğa)
  2  | [1, 2] |   1    | [0, 1]       | [0, 0, 0, 1]
  3  | [2]    |   2    | [0, 1, 2]    | [0, 0, 0, 0]   (3 serbest, kuyruğa)
  4  | [3]    |   3    | [0, 1, 2, 3] | -
```
Sıra 4 eleman = N → geçerli.

**Döngü örneği:** `N=2`, `[(0,1), (1,0)]` → `bekleyen = [1, 1]`, kuyruk baştan boş, sıraya 0 eleman yazıldı → `false`.

**Yapı:** Komşuluk listesi gömülüde dizilerle (`head[b]`, `next[kenar]`, `to[kenar]`). Kuyruk için ayrı yapı gerekmez: çıktı dizisi `order[]` aynı zamanda kuyruktur (bir okuma, bir yazma indeksi).

**Karmaşıklık:** O(N + m) zaman ve bellek.

**Kalıp:** "Önce şu, sonra bu" bağımlılığı + geçerli sıra veya döngü tespiti → topolojik sıralama. (`veri-yapilari-calisma-notu.md` ve `hangi-yapiyi-secmeli.md` Bölüm 26'da kısa özet var.)

**YAPILACAK:** Mantık oturunca kodunu birlikte yazıp test et (dizi tabanlı komşuluk listesi, `malloc`'suz).

---

## 2. Tekrar bakılacaklar (anlaşıldı ama pekiştirilmeli)

- **Soru 8 (alt dizi toplamı = K):** Bakiye (kümülatif toplam) + hash map. Sıra: ÖNCE sorgula (`p - k`), SONRA `p`'yi ekle. Başlangıçta `{0: 1}`. Neden sliding window olmaz (negatifler).
- **Soru 10 (en az kaç oda):** İki yöntem. Heap (bitiş saatleri) ve iki sıralı dizi. Eşitlik (`end == start`) çakışma değil.
- **Soru 2 (kaç gün sonra daha sıcak), atlamalı çözüm:** Sağdan sola, `out[j]` ile komşuya sor, zıpla.
- **Soru 5 (akıştan medyan):** İki heap kodunu kendin yazmayı dene (kod `hangi-yapiyi-secmeli.md`'de değil, sohbette verildi; istenirse nota eklenir).
