# ESP Remote 2.8

Firmware dla **LCDWIKI ES3C28P / ESP32-S3 N16R8**, LVGL 8.3.11, ekran pionowy **240×320**.
Wi-Fi z wyszukiwaniem sieci i klawiaturą hasła, języki polski/niemiecki/angielski,
sensory BME688/BSEC2 i SCD41 przez I²C/pogo, ESP-CAM przez router lub Direct,
karta SD przeznaczona na nagrania i logi. Nie zmieniono schematu ustawień V1.2.

V1.3.12 wykorzystuje tylną diodę RGB dla **świetlików obudowy**: czerwony start,
zielona gotowość, dowolny kolor RGB i osobna jasność. Sygnalizacja wybranego sensora
CO₂: zielony / od 1200 ppm pomarańczowy / od 1500 ppm czerwony. Brak aktualnego
odczytu: miganie niebieskie. **Ustawienia → Świetliki LED**.
[Instrukcja, ograniczenia i podgląd](docs/LED_LIGHT_PIPES_V1_3_12.md).

V1.3.11 poprawia kamerę: pełny ekran przełącza obraz i dotyk do **poziomu 320×240**,
a wyjście przywraca pion. Komunikat „Brak obrazu” i przyciski mają osobne obszary.
Wskaźniki Wi-Fi, baterii i procentów są wyrównane; segmenty oraz piorun mieszczą się
w ikonie. [Instrukcja i rzeczywisty podgląd](docs/CAMERA_ORIENTATION_V1_3_11.md).

V1.3.10 dodaje **sensory Bluetooth BTHome / BLE ESS**: najpierw dodaj urządzenie,
potem wybierz wyświetlany pomiar. Drugi ekran główny otwiera się przesunięciem palca.
Nowe większe ikony, bez daty, bateria bliżej procentów. Połączenie zaczyna się od
WLAN / Bluetooth / ESP, a aktualizacje są w **System → Software**.
[Obsługiwane sensory, instrukcja i podgląd](docs/SENSORS_BLE_V1_3_10.md).

V1.3.8 wzmacnia jasne obramowania, rozdziela ustawienia **Kamera / Audio**,
przebudowuje sensory na duże kafelki i dodaje podgląd kamery na całym ekranie
z przyciskami na obrazie. Nowe ikony w całym interfejsie; układ strony głównej
zachowany. [Opis i podgląd LVGL](docs/DASHBOARD_CAMERA_V1_3_8.md).

V1.3.7 dodaje **Ustawienia → Wygląd → Jasny / Ciemny**. Motyw zmienia się od razu
i zostaje po restarcie. Jasny wariant obejmuje wszystkie ekrany i klawiaturę.
Wybór jest zapisany osobno od dotychczasowych ustawień; aktualizacja zachowuje Wi-Fi
oraz konfigurację kamery. [Opis i podgląd obu motywów](docs/LIGHT_DARK_V1_3_7.md).

V1.3.6 odświeża wygląd podstron: płaskie grafitowe karty bez jasnych obramowań,
lżejsze opisy 18 px, prostsze teksty PL/DE/EN i aktualne odczyty sensorów na liście.
Strona główna pozostaje identyczna. Zachowano duże pola dotykowe i poprawkę TLS.
[Opis i rzeczywisty podgląd LVGL](docs/MODERN_UI_V1_3_6.md).

V1.3.5 naprawia alokację pamięci TLS dla błędu **32512 / 0x7F00**: duże bufory
preferują PSRAM, z zapasową pulą i diagnostyką RAM.
[Opis i instrukcja jednorazowego USB dla V1.3.1–V1.3.4](docs/TLS_MEMORY_V1_3_5.md).

## Instalacja i OTA

[Pobierz najnowsze wydanie](https://github.com/voltex-at/esp-Remote-2.8/releases/latest).
**Pierwszą wersję V1.3 z klientem GitHub OTA wgraj przez USB.**

| Plik | Zastosowanie |
|---|---|
| `firmware.bin` | Aktualizacja aplikacji przez OTA; z V1.2 również USB pod `0x10000`, z zachowaniem ustawień |
| `firmware-merged.bin` | Czysta instalacja USB pod `0x0`; usuwa ustawienia |
| `PILOT_LCD_TEST_APP.bin` | Niezależny test LCD/dotyku przez USB pod `0x10000` |
| `ota.json` | Wersja, sprzęt docelowy, rozmiar i SHA-256 aplikacji |

Na pilocie: **Ustawienia → System → Software → Sprawdź → Aktualizuj → Potwierdź**.
Połącz pilot z routerem z dostępem do internetu i poczekaj na ustawienie czasu NTP.
Zatrzymaj nagrywanie/odtwarzanie, audio i skan Wi-Fi przed aktualizacją.
Sprawdzenie wersji ani połączenie Wi-Fi nie rozpoczyna instalacji automatycznie.
Nowa instalacja wymaga wyboru sieci i wpisania własnego hasła oraz tokena kamery.
Firmware nie zawiera domyślnych poświadczeń; techniczne ArduinoOTA w LAN uruchamia
się wyłącznie z niepustym tokenem. Aktualizacja aplikacji zachowuje zapisane ustawienia.

Klient weryfikuje certyfikaty HTTPS, sprzęt, rozmiar partycji i SHA-256. Zapisuje drugą
partycję i wybiera ją dopiero po pełnej weryfikacji obrazu. SHA-256 wykrywa uszkodzenie;
nie jest podpisem wydawcy. Automatyczny rollback nie jest skonfigurowany.
Karta SD i dane NVS pozostają poza aktualizacją aplikacji.

## Rozwój i wydania

`pio run -d remote -e es3c28p` kompiluje pilot; `-e display_test` kompiluje test LCD.
Workflow `.github/workflows/firmware.yml` sprawdza testy, kompiluje oba warianty,
renderuje rzeczywisty interfejs LVGL i publikuje wydanie z `main` dopiero po sukcesie.
Kolejne wydanie wymaga zwiększenia `FIRMWARE_VERSION` w `remote/include/board_config.h`.
Istniejące wydania nie są nadpisywane. Każdy commit nie oznacza nowego wydania.

Pełna dokumentacja sprzętu i funkcji: [README_PL.md](README_PL.md).
OTA: [docs/OTA.md](docs/OTA.md). Diagnostyka LCD: [docs/LCD_FIX_V1_2.md](docs/LCD_FIX_V1_2.md).

V1.3.12 wymaga testu na fizycznym pilocie. Testy na PC i udana kompilacja nie potwierdzają
działania LCD, sensorów, radia ani aktualizacji przy zaniku zasilania.
