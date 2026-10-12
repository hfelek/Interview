# Kamera tasarımı: bileşen seçimi notları (başka oturumdan aktarıldı)

Durum: kamera bloğu bitti, kablosuz bağlantı bloğunu konfigüre ederken kalındı. Devam noktasını kullanıcı söyleyecek.
Senaryo: pilli güvenlik kamerası (kapı zili alternatifi konuşuldu, seçilmedi).
Not: güç değerleri mertebe olarak, datasheet ve ölçümle doğrulanır. Mülakat cümlesi: "Roughly ..., I'd confirm with the datasheet and by measurement."

## Kalıp (her bileşen için)
Requirement -> Options -> Numbers -> Trade-off -> Pick -> Risk/verify

## 1. Pil
- Öneri: çıkarılabilir Li-ion paket (~6000 mAh) + koruma devresi + NTC'li şarj (0 °C altında şarj yok) + fuel gauge.
- Alternatifler: LiFePO4 (soğukta ve döngüde iyi, ağır), primary Li-FeS2/Li-SOCl2 (şarjsız, SOCl2 tepe akımda zayıf), alkalin (iç direnç yüksek, Wi-Fi darbesinde brownout), CR2032 (sadece µA sensörler).
- Bakılacaklar: günlük mAh x gün, tepe akım/iç direnç, sıcaklık, voltaj aralığı (regülatör), güvenlik/sertifika.

## 2. Kamera donanımı
- Zincir: lens -> sensör -MIPI CSI-2-> SoC (ISP, H.264/H.265, NPU) ; IR LED, IR-cut, ışık sensörü.
- Kaba güç: sensör 100-200 mW, SoC 0.5-1 W, IR LED 0.5-2 W (gece en büyük kalem), RAM onlarca-yüzlerce mW.
- SoC kriterleri: HW encoder, suspend/fast-boot, NPU, secure boot/crypto, SDK/LTS.
- IR: 850 nm (verimli, hafif kırmızı parıltı) vs 940 nm (görünmez, daha çok güç). Gece olayı gündüzün birkaç katı.

## 3. Çözünürlük ve güç
- 1080p 1x, 2K ~1.8x, 4K ~4x (piksel/sn). Asıl etki bit hızı -> Wi-Fi hava süresi.
- Kazançlar: H.265, 15 fps, akıllı bit hızı, iki akış (analiz küçük, kayıt tam).

## 4. Kablosuz tip
- Wi-Fi 4 2.4 GHz: TX 150-250 mA, video için şart. Wi-Fi 6 TWT: pilli için umut verici.
- BLE (~5-20 mA, kurulum), Zigbee/Thread, sub-GHz/LoRa, LTE-M/NB-IoT (Wi-Fi yoksa, PSM): video için uygun değil.
- Amazon Sidewalk: BLE/900 MHz düşük bant, Wi-Fi koptuğunda temel bildirim.
- Fikir: enerji = hava süresi. 10 sn x 2 Mbps ~ 2.5 MB; 10 Mbps etkin hızda ~2 sn, zayıf sinyalde çok daha uzun.
- Öneri: Wi-Fi 4 + BLE kombo çip (örnek CYW43439).

## 5. Wi-Fi modülü ve SoC bağlantısı
- SDIO 4-bit (datasheet: 50 MHz'de 200 Mbps'ye kadar), alternatif gSPI; BT için UART HCI.
- WL_REG_ON/BT_REG_ON, HOST_WAKE (SoC ve MCU'ya), 32.768 kHz uyku saati.
- Sensör: MIPI CSI-2 + I2C ayar + MCLK + reset/PWDN. MCU<->SoC: güç hattı pini, reset, UART/SPI.

## 6. MCU saati
- İki işlemci: güç yöneticisi MCU (hafif, hep açık) ve kamera SoC (ağır, olayda).
- MCU: birkaç MHz - birkaç on MHz, dahili RC (hızlı başlar), PLL yok (uyanma gecikmesi), RTC için 32.768 kHz LSE.
- Fikirler: dinamik güç ~ frekans (uA/MHz, voltage scaling); race-to-idle (ölçerek karar).
- SoC: DVFS, encoder/ISP saati çözünürlük ve fps'e göre; asıl kazanç SoC'yi tamamen kapatmak.

## 7. Eklenen başlıklar
PIR vs radar; güç mimarisi (buck/LDO, Iq, load switch, bulk cap, şarj IC, fuel gauge); bellek (NOR vs NAND/eMMC, aşınma); ses; güvenlik donanımı (secure boot, crypto, eFuse, secure element); RTC/32 kHz; anten/RF (Wi-Fi+BLE tek anten, metal kasa); ortam (-20..+60 °C, IP, yoğuşma); maliyet/tedarik; sertifikasyon (FCC, CE/RED, Bluetooth SIG, Wi-Fi Alliance, ön sertifikalı modül); üretim (fabrika testi, anahtar yükleme, kalibrasyon); SDK ekosistemi.
