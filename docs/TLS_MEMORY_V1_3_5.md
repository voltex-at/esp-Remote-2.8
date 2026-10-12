# V1.3.5 — TLS 32512 / brak RAM

Zdjęcie fizycznego pilota z V1.3.3 pokazuje **32512 = 0x7F00**. W użytej bibliotece
jest to `MBEDTLS_ERR_SSL_ALLOC_FAILED` (zwykle -0x7F00). ESP-TLS zapisuje `-ret`,
stąd dodatnia liczba na ekranie. Ten kod oznacza nieudaną alokację pamięci SSL/TLS.
Nie wskazuje błędu hasła ani nieudanego sprawdzania certyfikatu.

Arduino 2.0.17 / SDK ESP32-S3 ma `CONFIG_MBEDTLS_INTERNAL_MEM_ALLOC=1` i domyślne
bufory rekordów TLS 16384 bajty. Źródło SDK kieruje te alokacje wyłącznie do
wewnętrznego RAM. Wolna PSRAM nie pomaga takiemu alokatorowi; znaczenie ma też
największy wolny blok, a nie tylko suma wolnych bajtów.

V1.3.4 zachowała klienta TLS z V1.3.3, dlatego sama zmiana interfejsu nie usuwała
tego problemu. V1.3.5 instaluje wspierany alokator `mbedtls_platform_set_calloc_free`
po potwierdzeniu PSRAM i **przed uruchomieniem zadań sieciowych**:

- Alokacje od 1024 B trafiają najpierw do PSRAM; mniejsze preferują RAM wewnętrzny.
- Przy braku miejsca próbowana jest druga pula.
- `calloc` zeruje pamięć i odrzuca przepełnienie mnożenia; `free` obsługuje obie pule.
- Para funkcji pozostaje aktywna przez cały czas działania, bez zmiany globalnego
  alokatora w trakcie życia obiektów TLS.
- Zadanie obrazu kamery czeka również podczas sprawdzania OTA.

Nie zmniejszono 16KB buforów TLS, ponieważ serwer może wysłać taki rekord.
Zachowano certyfikaty CA, sprawdzanie hosta i SHA-256, większy UI z V1.3.4,
sterownik LCD i schemat NVS. Nie zmniejszono stosów zadań ani buforów DMA.

## Diagnostyka

Log USB 115200 zawiera `[TLS memory]`: wolny RAM i największy blok w obu pulach,
liczbę alokacji oraz rozmiar ostatniej nieudanej alokacji. Raport powstaje po
instalacji alokatora, przed HTTPS oraz po sukcesie/błędzie. Nie zawiera poświadczeń.
Kody nieudanej alokacji mbedTLS, z oboma znakami, są klasyfikowane jako błąd pamięci.

Test z atrapą pul pamięci odtwarza brak wewnętrznego RAM: stary alokator nie może
przydzielić 16384 B, a nowy przydziela zerowany bufor w PSRAM. Sprawdzono też
małe alokacje, zapasową pulę, brak obu pul, przepełnienie, zwalnianie i pojedynczą
instalację funkcji. Kompilacja linkuje rzeczywiste API SDK; testy LVGL obejmują
nowy komunikat 32512 PL/DE/EN i niezmienioną stronę główną.

To programowe testy alokatora i interfejsu. Skuteczność połączenia z GitHub na
fizycznym pilocie wymaga potwierdzenia po instalacji.

## Jednorazowa instalacja

Przy dotychczasowej instalacji merged i bez żadnego udanego OTA wgraj przez USB
**`bin/firmware.bin` pod 0x10000, bez Erase all**. Zachowuje ustawienia i dane NVS.
Po restarcie sprawdź OTA; gdy V1.3.5 jest najnowsza, oczekiwany wynik to
„Masz aktualną wersję”. Jeśli wystąpi błąd, zdjęcie lub log `[OTA]` / `[TLS memory]`
pozwoli ustalić dalszą przyczynę.

Jeśli wcześniejsze OTA zmieniło aktywny slot, sam zapis pod 0x10000 nie przełącza
slotu. Plik merged pod 0x0 to czysta instalacja i usuwa ustawienia.

Źródła SDK:
[przechwytywanie kodu TLS](https://github.com/espressif/esp-idf/blob/v4.4.7/components/esp-tls/esp_tls_mbedtls.c),
[alokator domyślny](https://github.com/espressif/esp-idf/blob/v4.4.7/components/mbedtls/port/esp_mem.c).
