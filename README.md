# ESP Remote 2.8

Firmware dla **LCDWIKI ES3C28P / ESP32-S3 N16R8**, LVGL 8.3.11, ekran pionowy **240×320**.
Wi-Fi z wyszukiwaniem sieci i klawiaturą hasła, języki polski/niemiecki/angielski,
sensory BME688/BSEC2 i SCD41 przez I²C/pogo, ESP-CAM przez router lub Direct,
karta SD przeznaczona na nagrania i logi. Nie zmieniono schematu ustawień V1.2.

V1.3.2 zwiększa kontrast, rozmiar i grubość tekstu oraz ikon. Nazwy i wartości ustawień
są rozdzielone; suwaki jasności/głośności są widoczne i pokazują procent. Dodano
**Wyświetlacz → Test kolorów**. [Opis i podgląd](docs/READABILITY_V1_3_2.md).
Zachowano szybką ścieżkę LCD i diagnostykę resetu z [V1.3.1](docs/LCD_FIX_V1_3_1.md).

## Instalacja i OTA

[Pobierz najnowsze wydanie](https://github.com/voltex-at/esp-Remote-2.8/releases/latest).
**Pierwszą wersję V1.3 z klientem GitHub OTA wgraj przez USB.**

| Plik | Zastosowanie |
|---|---|
| `firmware.bin` | Aktualizacja aplikacji przez OTA; z V1.2 również USB pod `0x10000`, z zachowaniem ustawień |
| `firmware-merged.bin` | Czysta instalacja USB pod `0x0`; usuwa ustawienia |
| `PILOT_LCD_TEST_APP.bin` | Niezależny test LCD/dotyku przez USB pod `0x10000` |
| `ota.json` | Wersja, sprzęt docelowy, rozmiar i SHA-256 aplikacji |

Na pilocie: **Ustawienia → System → Aktualizacja OTA → Sprawdź aktualizacje → Zainstaluj → Potwierdź instalację**.
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

V1.3.2 wymaga testu na fizycznym pilocie. Testy na PC i udana kompilacja nie potwierdzają
działania LCD, sensorów, radia ani aktualizacji przy zaniku zasilania.
