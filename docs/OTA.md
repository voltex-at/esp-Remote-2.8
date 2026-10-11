# GitHub OTA — V1.3.0

Repozytorium: `voltex-at/esp-Remote-2.8`. Cel: `espremote-es3c28p-16mb-portrait`.
Manifest: `https://github.com/voltex-at/esp-Remote-2.8/releases/latest/download/ota.json`.
Adres pliku aplikacji jest związany z konkretną wersją wydania.
Nie jest pobierany token GitHub i nie jest potrzebny na publicznym repozytorium.

Pierwsze wgranie z V1.2: `firmware.bin` przez USB pod 0x10000 na niezmienionej tabeli
partycji. Są dwie partycje aplikacji po 5MiB; OTA wybiera nieaktywną. Obraz FULL
jest tylko do czystej instalacji USB. Nie wolno go wybrać jako pliku OTA.

Instalacja jest dwustopniowa: wybór wersji i potwierdzenie. Kandydat wygasa po 15 minutach;
trzeba wtedy ponownie sprawdzić aktualizacje. Odrzucane są wersje starsze i identyczne,
błędny cel, rozmiar poza partycją, niedozwolone adresy/przekierowania, zły nagłówek
ESP32-S3, niepełny transfer, błąd zapisu lub SHA-256. W razie błędu przed wyborem
nowej partycji uruchamiana aplikacja pozostaje dotychczasowa.

Certyfikaty sprawdza pakiet CA ESP-IDF. Czas NTP jest wymagany do poprawnej walidacji.
Nie wyłączamy sprawdzania TLS. OTA wymaga internetu, którego tryb Direct kamery
zwykle nie zapewnia. Pobieranie odbywa się w osobnym zadaniu; LVGL wyświetla postęp.
Podczas instalacji wstrzymuje się kamera, zmiany ustawień i operacje Wi-Fi.
Audio/nagrywanie/odtwarzanie i skan muszą zostać wcześniej zatrzymane. Znany poziom
baterii 15% lub mniej blokuje instalację. Nieznany odczyt ADC nie udaje pełnej baterii.

Ustawienia `Settings` i przestrzeń NVS `remote-v1` są zgodne z V1.2. OTA nie aktualizuje
grafik SD ani ESP-CAM; kamera nadal używa osobnego projektu `camera/`.
W repozytorium są gotowe fonty i grafiki w kodzie; kamera i SD pozostają niezależne.

Peppiuhr był odniesieniem dla sposobu dostarczania: GitHub Releases, `firmware.bin`,
`ota.json` i osobny obraz USB. Klient i ekran pilota są nową implementacją dopasowaną
do LVGL8 i pionu240×320. Nie skopiowano UI Peppiuhr ani jego partycji7MiB.

Automatyczny rollback wymaga skonfigurowanego bootloadera i sprawdzania nowego startu.
Ta wersja go nie deklaruje. Zachowaj możliwość wgrania aplikacji przez USB.

Walidacja: testy polityki adresów/wersji/celu, metadanych wydania oraz callbacków
rzeczywistego LVGL; kompilacja ESP32-S3 w Actions. Przed użyciem trzeba fizycznie
sprawdzić TLS, transfer, ponowny start i zachowanie ustawień na pilocie.
