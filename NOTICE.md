# Zależności i licencje

Kod projektu i własne SVG: MIT, z wyjątkiem oznaczonych adaptacji i zależności.

- LVGL8.3.11: MIT, https://github.com/lvgl/lvgl/blob/v8.3.11/LICENCE.txt
- Adafruit_GFX1.11.11: BSD-2-Clause, Copyright (c) 2012 Adafruit Industries, https://github.com/adafruit/Adafruit-GFX-Library/blob/1.11.11/license.txt . Kopia w `docs/licenses/Adafruit-GFX.txt`.
- Adafruit BusIO1.17.4: licencja upstream w `docs/licenses/Adafruit-BusIO.txt`; zależność Adafruit_GFX.
- LovyanGFX1.2.7 nie jest już używane w pilocie V1.2; zachowano kopię wcześniejszej licencji w dokumentacji historycznej.
- Arduino-ESP32 2.0.17 / ESP-IDF4.4.7: licencje upstream (LGPL/Apache oraz licencje komponentów).
- BSEC2 1.10.2610 / algorytm2.6.1.0: **licencja Bosch BSEC**, nie MIT. Użytkownik pobierający bibliotekę musi zapoznać się z jej warunkami: https://github.com/boschsensortec/Bosch-BSEC2-Library/blob/master/LICENSE.md oraz https://www.bosch-sensortec.com/software-tools/software/bsec/ . Archiwum projektu nie zawiera źródeł ani prekompilowanych blobów biblioteki Bosch; pobiera je PlatformIO. Obraz firmware pilota, jeżeli dołączony, zawiera połączony algorytm i podlega również tym warunkom.
- BME68x Sensor API i wrapper: licencja upstream Bosch.
- Sensirion I2C SCD4x1.1.0 / Sensirion Core: BSD-3-Clause.
- ArduinoJson6.21.5: MIT.
- TJpg_Decoder1.1.0 / ChaN TJpgDec: licencja upstream permissive.
- Dwie ikony koła w fontach: DejaVu Sans / Bitstream Vera, licencja w `assets/DejaVu-LICENSE.txt`.
- Montserrat: SIL Open Font License1.1. `assets/OFL.txt`. Fonty ui12/ui14/ui18 są wygenerowanymi podzbiorami.
- Sekwencja rejestrów ES8311 w `board.cpp`: adaptacja z Espressif esp-bsp ES8311, Copyright Espressif Systems, Apache-2.0. Źródło https://github.com/espressif/esp-bsp/blob/master/components/es8311/es8311.c . Kopia licencji w `assets/Apache-2.0.txt`.

Grafiki referencyjne użytkownika służyły do zaprojektowania interfejsu; nie są dystrybuowane w paczce. Ikony, moduł i góry są własnymi SVG, a nie fragmentami zdjęć.

Parametry sprzętu i standardową sekwencję komend SPI porównano z Peppiuhr v1.19.15,
https://github.com/voltex-at/Peppiuhr/releases/tag/v1.19.15 . Nowy `PortraitDisplay`
jest własną implementacją transportu; nie dystrybuujemy źródeł UI/fasady Peppiuhr.
Mapę obrotów sprawdzono w oficjalnym Adafruit_ILI9341. Szczegóły w `docs/LCD_FIX_V1_2.md`.
