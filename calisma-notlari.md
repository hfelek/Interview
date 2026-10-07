# Mülakat Çalışma Notları (Gömülü Sistemler, Veri Yapıları)

Tekrar bakılacak noktalar. Çözülen sorulardan çıkan dersler.

---

## 1. Hash fonksiyonu

**Knuth çarpımsal hash**
- Sabit: `2654435761` ≈ 2^32 / φ (φ = altın oran ≈ 1.618). Knuth, bu değere en yakın asal sayıyı önerir.
- Neden altın oran: ardışık anahtarlar tablonun uzak yerlerine düşer, kümelenme azalır.
- Çarpan **tek (odd)** olmalı. Çift çarpan alt bitleri sıfırlar.
- Kayma miktarı: 32 bit hash için `>> 16` (genişliğin yarısı), 64 bit için `>> 32`.
- Asıl Knuth yöntemi: `slot = (key * A) >> (32 - k)`, tablo boyutu 2^k. Üst bitler en kaliteli bitlerdir. `& mask` ile alt bit alıyorsanız önce `h ^= h >> 16` ile karıştırın.

**Hash yazarken yapılmaması gerekenler**
1. Anahtarı doğrudan `& mask` veya `% boyut` yapmak (desenli anahtarlar çakışır).
2. Çift sayıyla çarpmak.
3. Girdinin bir kısmını görmezden gelmek.
4. Deterministik olmayan değer kullanmak (rand, adres, zaman).
5. İşaretli (signed) taşma. Önce `uint32_t`'ye çevirin.
6. Pahalı yapmak.
7. Güvenlik gereken yerde kendi fonksiyonunu yazmak (hash flooding, SipHash kullan).
8. Test etmemek (çakışma ve en uzun yoklama zincirini say).

**Açık adresleme (doğrusal yoklama) kuralları**
- Kapasite 2'nin kuvveti, en az 2n (yük faktörü ≤ 0.5).
- Silme yoksa "boş slot görünce dur" güvenli.
- `calloc` ile tüm `isFilled` alanları 0 başlar. NULL kontrolü yap.
- Yoklama döngüsü `i <= mask` (tablo `mask+1` slot).

---

## 2. Two Sum / İlk tekil ID: hash map kalıpları

- Two Sum: tek geçiş. Her eleman için `target - nums[i]` daha önce görüldü mü? Sonra kendini ekle.
- İlk tekil: iki geçiş. Geçiş 1: sayaç çıkar. Geçiş 2: **orijinal diziyi** gez, sayacı 1 olan ilk elemanı dön. "İlk" kavramını sadece dizi taşır, hash tablosu sırayı saklamaz.
- En az geçen değer: sayımdan **sonra** min takibi yap. Sayım sırasında karşılaştırma yanlış sonuç verir. Beraberlik kuralını mülakatta sor.
- `add` fonksiyonu: slot boşsa yeni hücre (count = 1), anahtar eşitse `count++`, değilse sonraki slot. Anahtar eşitliğini kontrol etmezsen aynı anahtar için birden fazla hücre oluşur.
- Min heap bu sorularda gereksiz. Heap ancak "en az/en çok K tane" veya akış sorgularında gerekir.

---

## 3. Stack: parantez doğrulama

- Açan karakter gelince **kapanış karşılığını** push et. Kapanış gelince pop edip karşılaştır.
- Son koşul: eşleşmezlik **yok VE** stack **boş** (ikisi birlikte, "veya" değil). `"(("` örneği.
- Kenar durumlar: boş stack'ten pop (`"())"` → false), geçersiz karakter, stack taşması (sabit üst sınır), tek uzunluk → direkt false.
- Tek tür parantez olsaydı sayaç yeterdi. Üç tür olduğu için sıra önemli, LIFO gerek.

---

## 4. Ring buffer

**Seçim gerekçesi**
- Düz dizi + kaydırma: O(n). Bağlı liste: her eleman için `malloc`, ISR'da güvensiz. Ring buffer: sabit dizi, iki indeks, O(1), kopyalama yok.

**Sürekli artan sayaç yöntemi** (bir slot kaybı yok, `count` yok)
- `head` = toplam yazılan, `tail` = toplam okunan. İşaretsiz tip (`uint32_t`).
- Eleman sayısı = `head - tail` (0..N). Boş: `head == tail`. Dolu: `head - tail >= N`.
- Dizi erişimi: `buf[sayaç & (N-1)]`.
- Boyut **2'nin kuvveti olmak zorunda**: sayaç 2^32'de sarılırken `sayaç % N` indeksi atlamasın.
- Alternatifler: bir slotu boş bırakmak (kapasite N-1), `count` alanı (ISR/ana döngü yarışı çıkarır).

**Yapılan hatalar**
- `get`'te `tail--` yazılmış, doğrusu `tail++`.
- `get`'te maskeleme unutulmuş (dizi dışı okuma).
- `sizeof buf_t` yazılmış, tür adıyla `sizeof(buf_t)` parantezli olmalı.
- Doluyken `put` veriyi ezmemeli, hiçbir şeyi değiştirmeden `false` dönmeli.
- Boşken `*byte`'a dokunma. NULL kontrolleri.
- Yorumlar: `head` = bir sonraki yazılacak yer, `tail` = bir sonraki okunacak yer (indeks değil, sayaç).

---

## 5. ISR + ana döngü: eşzamanlılık

**Tek üretici, tek tüketici (SPSC)**
- `head`'i sadece üretici (ISR), `tail`'i sadece tüketici (ana döngü) yazar. Ortak yazılan alan yok, kilit gerekmez.
- **İkisi de `volatile`** olmalı (her biri bir bağlamda yazılıp diğerinde okunuyor).
- Bayat değer güvenli: üretici eski `tail` görürse daha dolu sanar, tüketici eski `head` görürse daha boş sanar. Veri bozulmaz.
- Mutex ISR'da **kullanılamaz** (ISR bloklanamaz, deadlock olur).
- Birden fazla üretici/tüketici varsa kısa bir kesme-kapatma bölümü gerekir. `PRIMASK`'ı kaydet, kapat, geri yükle. Körlemesine `enable` yapma.

**`volatile` yetmez**
- Atomiklik garantisi vermez. 8/16 bit MCU'da 32 bit erişim yırtılabilir. Sayaç tipini doğal kelime genişliğine indir (AVR: `uint8_t`, N ≤ 128).
- Sıralama garantisi vermez (normal `buf` yazımı ile volatile `head` yazımı arasında).

**Bariyer**
```c
#define BARRIER() __asm volatile("" ::: "memory")
```
- Derleyiciye "bu satırın ötesinde bellek erişimlerini taşıma" der. Makine kodu üretmez.
- `__asm` = inline assembly. `volatile` = silme/taşıma. `""` = boş komut. `"memory"` = bellek etkilenmiş olabilir.
- Sıra: üretici önce **veriyi yaz**, BARRIER, **sonra `head`'i artır**. Tüketici önce **`head`'i oku**, BARRIER, **sonra veriyi oku**, BARRIER, **sonra `tail`'i artır**.
- **Tek çekirdek:** `volatile` + derleyici bariyeri yeterli.
- **Çok çekirdek:** donanım bariyeri de gerekir (`__DMB()` veya `"dmb ish"`), cache tutarlılığını düşün.
- C11 `<stdatomic.h>` / `_Atomic` saf C'dir (C++ değil), release/acquire ile aynı iş. Derleyici destekliyorsa en güvenli seçenek.

**Mülakat cümlesi**
> "Her indeksin tek yazarı var, o yüzden kilit gerekmiyor. Veriyi yazdıktan sonra head'i yayınlıyorum, tüketici head'i okuduktan sonra veriyi okuyor. Tek çekirdekte derleyici bariyeri yeter, çok çekirdekte donanım bariyeri veya C11 atomik kullanırım. Mutex ISR'da kullanılamaz."

---

## Tekrar için kontrol soruları
- `head - tail` sarılma sırasında neden doğru çıkar? (İşaretsiz modüler aritmetik, N | 2^32.)
- ISR'da neden `malloc` ve mutex olmaz?
- `volatile` ile `atomic` farkı nedir?
- Hash'te neden önce karıştırıp sonra maskeliyoruz?
- Ring buffer'da dolu/boş ayrımının 3 yolu ve her birinin bedeli nedir?
