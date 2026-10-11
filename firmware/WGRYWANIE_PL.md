# Pliki BIN — pilot V1.2.0

Te obrazy skompilowano z ustawieniami fabrycznymi pokazanymi w kodzie. Wariant kamery to **AI Thinker / OV2640**, a pilota **LCDWIKI ES3C28P N16R8**. Dla innego modelu lub własnego hasła kamery/jej tokenu zbuduj firmware ze źródeł. W pilocie wybór sieci ze skanu, wpisywanie hasła, token i adres kamery są dostępne na ekranie. Język: Ustawienia → Ogólne → Język.

- `PILOT_ES3C28P_FULL.bin`: pełny obraz pilota, adres0x0.
- `PILOT_LCD_TEST_APP.bin`: niezależny test LCD/dotyku, adres0x10000, bez zmiany NVS. Po sprawdzeniu wróć do normalnej aplikacji.
- `PILOT_LCD_TEST_FULL.bin`: pełny obraz tego testu pod0x0 — kasuje ustawienia NVS tak jak pozostałe FULL.
- `KAMERA_AI_THINKER_FULL.bin`: pełny obraz kamery bez audio, adres0x0.
- `KAMERA_AI_THINKER_AUDIO_FULL.bin`: tylko dla zewnętrznego mikrofonu/wzmacniacza I²S zgodnie z konfiguracją pinów. Nie wgrywaj go w celu uzyskania dźwięku z niepodłączonych modułów.
- Pliki `*_APP.bin` są samą aplikacją dla partycji0x10000 lub aktualizacji OTA pilota. Nie zawierają bootloadera i tabeli partycji.

Pełny obraz zawiera także obszary między partycjami i **resetuje dotychczasowe ustawienia NVS**. Aktualizacja samej aplikacji zachowuje ustawienia V1.0/V1.1, jeżeli już wgrano odpowiednią tabelę partycji. Dla istniejącego pilota użyj `PILOT_ES3C28P_APP.bin` na 0x10000, aby zachować SSID, hasło i język. Kamera pozostaje w wersji V1.0.0; jej protokół jest zgodny.

Zacznij od normalnego APP; jeżeli obraz nadal jest biały, użyj LCD_TEST_APP.
Dokładny przebieg testu: `docs/LCD_FIX_V1_2.md`. Wyglądu na fizycznym pilocie
nie potwierdzono w tym środowisku.

Przykład (zastąp PORT własnym portem):

```bash
python -m pip install esptool==4.5.1
python -m esptool --chip esp32s3 --port PORT --baud 460800 write_flash 0x0 firmware/PILOT_ES3C28P_FULL.bin
python -m esptool --chip esp32 --port PORT --baud 460800 write_flash 0x0 firmware/KAMERA_AI_THINKER_FULL.bin
```

Pilota wprowadź do trybu BOOT/RESET. ESP32-CAM bez USB wymaga USB-UART i GPIO0doGND na czas wejścia w tryb wgrywania. Po wgraniu odłącz GPIO0odGND i zresetuj kamerę.

Domyślna sieć kamery: `ESP32-BABY-CAM`; hasło `ChangeMe-Cam-2026`; token `change-this-pair-token`; adres `192.168.4.1`. To ustawienia pierwszego uruchomienia. Dla codziennego użycia zmień je po obu stronach (kamera: kompilacja kodu, pilot: Ustawienia → Połączenie).

Sumy SHA256 w `SHA256SUMS.txt`. Plik pełny ma magic bootloadera pilota na0x0, kamery na0x1000. Nie wgrywaj obrazu kamery do pilota ani odwrotnie.
