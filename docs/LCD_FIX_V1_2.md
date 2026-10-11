# V1.2 — LCD ES3C28P, pion 240×320

## Co zmieniono

W V1.1 start sprawdzał PSRAM **przed** inicjalizacją LCD. Brak PSRAM powodował
powrót z `setup()` bez obrazu błędu. Wcześniejszy panel LovyanGFX używał również
inwersji OFF; konfiguracja działającego Peppiuhr używa inwersji IPS ON.
To potwierdzone różnice w kodzie, **nie potwierdzenie przyczyny na Twoim urządzeniu**.

V1.2 uruchamia LCD przed alokacją PSRAM/LVGL i wyświetla komunikaty startowe.
W razie problemu z PSRAM, buforem LVGL lub zadaniem systemowym pokazuje błąd
na LCD oraz powtarza go na USB co 2 sekundy.

Nowy `PortraitDisplay` jest małym, synchronicznym sterownikiem SPI/RGB565.
Nie używa autodetekcji panelu ani DMA LovyanGFX. Adafruit_GFX służy tylko
do rysowania tekstu/figur diagnostycznych; właściwy interfejs nadal używa LVGL8.3.11.
Nie przenoszono aplikacji Peppiuhr ani jej LVGL9.

Parametry sprzętowe porównano z Peppiuhr v1.19.15:

| Parametr | Peppiuhr | Pilot V1.2 |
|---|---|---|
| SPI | FSPI, mode0, 40MHz | FSPI, mode0, 40MHz |
| SCK / MOSI / MISO | 12 / 11 / 13 | 12 / 11 / 13 |
| CS / DC / BL | 10 / 46 / 45 | 10 / 46 / 45 |
| Reset / wybudzenie | 0x01 +120ms / 0x11 +120ms | Takie same polecenia i opóźnienia |
| Format | RGB565, 0x3A=0x55 | RGB565, 0x3A=0x55 |
| MADCTL | 0x28, poziom 320×240 | 0x48, pion 240×320 |
| IPS / DISPON | 0x21 / 0x29 +20ms | Takie same polecenia |
| Dotyk FT6336G | x=rawY, y=239−rawX | x=rawX, y=rawY |

W Peppiuhr `setRotation()` nie zmienia orientacji kontrolera; poziom jest
zakodowany w MADCTL. W pilocie obsługiwany jest pion 0° lub 180°.
Dla montażu odwrotnego dodaj `-DREMOTE_LCD_ROTATION=2` do `build_flags` i skompiluj;
MADCTL stanie się 0x88, a dotyk otrzyma tę samą transformację.
Nie wystarczy usunąć bitu MV z 0x28: trzeba również ustawić właściwy kierunek MX/MY.
Wartości 0x48/0x88 odpowiadają standardowym obrotom0/2 ILI9341 i odwrotnej
transformacji dotyku zastosowanej w Peppiuhr dla obrotu1.

## Co wgrać najpierw

1. Na pilota, który ma już tabelę partycji V1.1, wgraj
   `firmware/PILOT_ES3C28P_APP.bin` pod **0x10000**. Zachowuje ustawienia NVS.
   Dla czystego urządzenia użyj `PILOT_ES3C28P_FULL.bin` pod **0x0**;
   pełny obraz resetuje ustawienia.
2. Po resecie powinien pojawić się ekran startowy, a następnie GUI LVGL.
   W logu USB115200 szukaj `[LCD]`, `[BOOT]`, `[LVGL] first flush` i `[BOOT] READY`.
3. Jeżeli pełna aplikacja nadal nie pokazuje obrazu, wgraj
   `PILOT_LCD_TEST_APP.bin` pod **0x10000**. To test tego samego sterownika,
   bez LVGL, Wi-Fi, SD, sensorów i audio. Nie zapisuje konfiguracji NVS.
   Oczekiwany obraz: trzy pionowe pasy RED/GREEN/BLUE, biała ramka,
   napis 240x320 i współrzędne dotyku. Dotknij czterech rogów; krzyżyk
   powinien trafiać w palec. BOOT odrysowuje test.
4. Przy testowym BIN też możesz użyć pełnego obrazu `PILOT_LCD_TEST_FULL.bin`
   pod 0x0, ale wtedy NVS zostanie wyczyszczone.
5. Po teście wróć do normalnego `PILOT_ES3C28P_APP.bin` pod 0x10000.

Przykład aktualizacji zachowującej ustawienia:

```bash
python -m esptool --chip esp32s3 --port PORT --baud 460800 write_flash 0x10000 firmware/PILOT_ES3C28P_APP.bin
```

## Interpretacja testu

- Test LCD działa, pełny firmware pokazuje błąd PSRAM: sprawdź wariant N16R8,
  pamięć OPI, `qio_opi` i log — nie zmieniaj pinów LCD na chybił trafił.
- Test LCD działa, ale pełna aplikacja nadal jest pusta: prześlij log od resetu,
  zwłaszcza ostatni etap `[BOOT]` i obecność pierwszego flush.
- Test LCD również pozostaje biały: prześlij log testu oraz oznaczenia płytki.
  Pozostają komunikacja SPI, rewizja panelu, zasilanie lub problem przed `setup()`.
- Kolory lub dotyk nie pasują: prześlij zdjęcie pasów oraz współrzędne z rogów.
  Jeżeli cały obraz jest odwrócony o 180°, użyj wariantu rotacji2 ze źródeł.

Nie podłączono tu fizycznego pilota. Sprawdzenie komend przez atrapę SPI
i udana kompilacja nie potwierdzają działania radia, LCD ani dotyku na płytce.

## Źródło parametrów

[Peppiuhr](https://github.com/voltex-at/Peppiuhr), wydanie
[v1.19.15](https://github.com/voltex-at/Peppiuhr/releases/tag/v1.19.15),
archiwum `Peppiuhr-v1.19.15-SOURCE_BIN.zip`, pliki `source/src/PeppiDisplay.cpp`,
`PeppiDisplay.h` i `Touch.h`.

SHA256 `PeppiDisplay.cpp`:
`614ca809001cfeb85dbfb3338e3258bdd48a04eba7974443a3d287d85b4f3ccb`.
Wykorzystano fakty sprzętowe i standardowe komendy kontrolera; nowy sterownik
nie zawiera skopiowanej fasady TFT_eSPI ani UI zegara.
Mapę obrotów sprawdzono także w oficjalnym
[sterowniku Adafruit ILI9341](https://github.com/adafruit/Adafruit_ILI9341/blob/master/Adafruit_ILI9341.cpp),
w funkcji `setRotation()`.
