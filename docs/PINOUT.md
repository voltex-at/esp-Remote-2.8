# ES3C28P — mapa pinów

Źródło: LCDWIKI ES3C28P, schemat v1.0. Zakłada wariant N16R8 z dotykiem.

| Funkcja | GPIO / adres |
|---|---|
| LCD SPI SCLK / MOSI / MISO | 12 / 11 / 13 |
| LCD CS / DC | 10 / 46 |
| LCD reset | wspólny sprzętowy EN; brak osobnego GPIO |
| Podświetlenie | 45, PWM kanał LEDC0 |
| I²C SDA / SCL | 16 / 15, 100kHz |
| FT6336G | 0x38, reset18, IRQ17 |
| ES8311 | 0x18 |
| BME688 | 0x76 lub 0x77 |
| SCD41 | 0x62 |
| SD CLK / CMD | 38 / 40 |
| SD D0 / D1 / D2 / D3 | 39 / 41 / 48 / 47 |
| Audio MCLK / BCLK / LRCLK | 4 / 5 / 7 |
| Audio TX / RX | 8 / 6 |
| Wzmacniacz | GPIO1, LOW=włączony |
| Bateria ADC | 9, dzielnik 100k/100k |
| BOOT | 0 |
| RGB LED | 42 (w V1 nie sterowany) |
| Wolne wyprowadzone GPIO | 2 / 3 / 14 / 21 |

## Pogo

Przypisz fizyczne kontakty do GND, 3.3V, SDA16 i SCL15 zgodnie z własnym okablowaniem. Fotografia/render mechaniczny nie określa ich kolejności. Ewentualny piąty kontakt pozostaw nieużywany lub zaprojektuj oddzielnie jako identyfikację/wykrywanie; firmware nie zakłada jego roli. Nie podłączaj nieznanego styku do zasilania.

BME688 goły układ wymaga zasilania 3.3V. Breakout musi mieć I²C pull-upy do 3.3V i CS ustawione dla I²C. SCD41 i BME688 mogą dzielić tę samą magistralę. Skan wykrywa również dotyk i kodek, nie przedstawia ich jako sensorów pogo.

Parametry baterii `BATTERY_DIVIDER`, `BATTERY_CALIBRATION` w `remote/include/board_config.h`. Pomiar ADC porównaj z miernikiem przy co najmniej dwóch poziomach naładowania. Brak sprzętowego sygnału wykrycia podłączonej baterii: napięcie mierzone przy zasilaniu USB bez ogniwa może nie być miarodajne.
