# ESP32 REMOTE — firmware V1.2

Projekt pilota LCDWIKI **ES3C28P (ESP32-S3 N16R8, dotyk, 240×320)** oraz towarzyszący firmware **ESP32-CAM AI Thinker / OV2640**. Interfejs: LVGL 8.3.11, ciemne karty, limonkowe akcenty, własne ikony, grafika gór i modułu pogo. Języki: polski, niemiecki, angielski. Wszystkie zasoby UI są kompilowane do flash; SD pozostaje dla babyphonu i logów.

**Przed uruchomieniem:** profil kamery jest założeniem, ponieważ nie podano jej modelu. Dla innej płytki zmień `camera/include/camera_config.h`. Kolejności fizycznych styków pogo nie da się wyznaczyć z renderu obudowy — potwierdź ją miernikiem. GPIO16=SDA, GPIO15=SCL, zasilanie sensorów 3.3V, wspólna masa. Podane GPIO oznaczają sygnały, a nie pozycje styków.

**Poprawka białego ekranu V1.2:** zmieniono transport LCD na bezpośredni SPI/RGB565
z parametrami porównanymi z Peppiuhr, zachowując pion 240×320 i LVGL8.3.
LCD startuje teraz przed testem PSRAM; błąd pamięci ma komunikat na ekranie i USB.
Dołączono niezależny test LCD/dotyku `PILOT_LCD_TEST_APP.bin`.
Instrukcja wgrywania, diagnoza i różnice orientacji: **`docs/LCD_FIX_V1_2.md`**.
Przy aktualizacji istniejącego pilota użyj APP na0x10000, aby zachować ustawienia.

## Szybkie uruchomienie

1. Zainstaluj VS Code + PlatformIO lub `python -m pip install platformio==6.1.18`.
2. W `camera/include/camera_config.h` zmień `CAM_DIRECT_PASSWORD` i `CAM_PAIR_TOKEN`. Hasło AP minimum 8 znaków, token 1–63 znaków. Dla routera wpisz `CAM_ROUTER_SSID` i `CAM_ROUTER_PASSWORD` (2.4 GHz).
3. Wgraj kamerę: `pio run -d camera -t upload`. AI Thinker wymaga adaptera USB-UART, GPIO0 do GND tylko na czas wejścia w tryb wgrywania. Następnie odłącz GPIO0 od GND i zresetuj.
4. Wgraj pilota: `pio run -d remote -t upload`. W razie potrzeby przytrzymaj BOOT, naciśnij RESET, zwolnij BOOT. Domyślny profil to 16MB flash / 8MB OPI PSRAM; inny wariant pamięci wymaga zmiany konfiguracji PlatformIO.
5. W pilocie: **Ustawienia → Połączenie → Wyszukaj sieci Wi-Fi**. Wybierz `ESP32-BABY-CAM`, wpisz hasło klawiaturą ekranową i zatwierdź. Pilot zapisze sieć i połączy się z kamerą bez routera. W Połączeniu ustaw również wspólny token; adres kamery w Direct to `192.168.4.1`.
6. Dla routera wybierz jego sieć z listy, wpisz hasło i ustaw adres IP kamery wypisany na jej porcie szeregowym. Oba urządzenia powinny być w tej samej sieci 2.4 GHz. **Ogólne → Język** pozwala wybrać Deutsch, Polski lub English.
7. Włóż do pilota kartę microSD z FAT32. Otwórz **Sensory → Podłącz sensor → Skanuj I²C**.

Hasła fabryczne w kodzie służą pierwszemu uruchomieniu. Zmieniaj je po obu stronach. HTTP/UDP są przeznaczone dla własnej, zabezpieczonej sieci lokalnej. Nie przekierowuj tych portów do Internetu. Firmware nie używa chmury.

## Co działa w kodzie

| Obszar | Implementacja |
|---|---|
| Ekran | LVGL 8.3.11, ILI9341V SPI, FT6336G, 21 widoków, przewijane listy, klawiatura ekranowa, trzy języki z polskimi znakami |
| Start / LCD | Bezpośredni transport SPI, inwersja IPS ON, pion MADCTL0x48; ekran diagnostyczny także bez PSRAM/LVGL, osobny test RGB i dotyku |
| BME688 | Oficjalna biblioteka Bosch BSEC2, temperatura i RH kompensowane, ciśnienie, IAQ i jego dokładność, eCO₂ i bVOC, tryb LP co 3 s, stan BSEC w NVS |
| SCD41 | Oficjalna biblioteka Sensirion SCD4x, bezpośredni CO₂ oraz zapasowe T/RH, gdy BME688 nie daje świeżych danych; nowe pomiary co około 5 s |
| Pogo / I²C | Skan 0x08–0x77, identyfikacja adresów, automatyczny ponowny skan co 10 s, odłączenie i ponowna inicjalizacja, timeout i próba odzyskania magistrali |
| Wykresy | Ostatnie 60 próbek w RAM, min/max, brak danych oznaczony kreską; bez danych demonstracyjnych w firmware |
| Alarmy | Progi temperatury, RH, CO₂, IAQ po osiągnięciu dokładności ≥2, niski akumulator, utrata kamery / Wi-Fi, ruch i głośny dźwięk z kamery |
| Kamera | Podgląd QVGA, odświeżanie docelowo do 8 fps, zdjęcia JPEG, zapis sekwencji JPEG z czasem, odtwarzanie na pilocie |
| Audio | ES8311 po stronie pilota, PCM 16kHz/16-bit/mono przez UDP, odsłuch i PTT; wymaga mikrofonu i wzmacniacza przy kamerze |
| SD | `/babyphone`: JPEG, RVC1 i opcjonalny WAV; `/logs`: CSV sensorów oraz zdarzeń. Rezerwa 64MB, limit klipu 5 minut, bez automatycznego kasowania |
| Bateria | ADC GPIO9, dzielnik 2:1, filtrowane napięcie, przybliżony procent, alarm ≤15% |
| Wi-Fi | Asynchroniczny skan z pilota, do 16 sieci, RSSI i zabezpieczenia, wybór SSID, hasło z ukrywaniem/podglądem, status połączenia, Direct lub router |
| Ustawienia | Jasność PWM, przygaszanie bezczynności, głośność, sieć, język, logowanie, interwał wykresu/logu, progi, NVS, reset z potwierdzeniem |
| Aktualizacja | USB / PlatformIO oraz ArduinoOTA w LAN; hasło OTA jest tokenem parowania z momentu uruchomienia połączenia |

## Wi-Fi i wybór języka w V1.1

- **Połączenie → Wyszukaj sieci Wi-Fi** uruchamia skan 2.4 GHz. Wyniki są uporządkowane według sygnału; ta sama sieć i rodzaj zabezpieczenia pojawiają się tylko raz z najsilniejszym punktem dostępu. Przycisk **Skanuj ponownie** odświeża listę.
- Dotknij sieci zabezpieczonej: pojawi się nazwa SSID, pole hasła i klawiatura. **Pokaż / Ukryj** przełącza widoczność. Zatwierdzenie klawiaturą zapisuje konfigurację i rozpoczyna połączenie; anulowanie pozostawia poprzednie ustawienia. Dla nowego SSID pole jest puste. Sieć otwarta łączy się po wyborze z pustym hasłem.
- Hasła: 8–63 bajty lub 64 znaki szesnastkowe dla klucza PSK. Personal WPA/WPA2/WPA3 jest rozpoznawane; WEP i sieci Enterprise nie mają obsługi logowania w tym interfejsie. Ukryty SSID wpisz ręcznie w **Połączenie → SSID**, następnie ustaw hasło i naciśnij **Połącz / ponów**.
- Sieć `ESP32-BABY-CAM` wybiera Direct; nowa inna sieć wybiera Router. Dla własnej nazwy AP kamery zmień **Tryb połączenia** na Direct i sprawdź adres kamery. Przełączenie trybu nie kasuje wybranego SSID.
- Karta Połączenie pokazuje stan łączenia, połączony SSID i IP lub błąd po około 15 sekundach. Pilot ponawia próbę w tle. Skanowanie nie blokuje LVGL; współdzielone radio może chwilowo zmniejszyć płynność podglądu kamery.
- **Ustawienia → Ogólne → Język**: Deutsch, Polski, English. Dotknięcie wiersza od razu zmienia język interfejsu i zapisuje go w NVS. Konfiguracja sieci i język przetrwają restart; SD nie służy do przechowywania tych ustawień.
- Aktualizacja samą aplikacją `PILOT_ES3C28P_APP.bin` zachowuje ustawienia z V1.0; struktura ustawień NVS nie została zmieniona.

## Różnice względem renderów referencyjnych

- Fizyczny ekran ma 240×320; referencje przedstawiają większe panele. Zachowano styl, a dłuższe listy przewijają się. Przy odczytach demo ilustracje w `docs/ui` pokazują **ten sam kod LVGL**, z celowo podstawionymi danymi tylko w symulatorze.
- Kamera AI Thinker **nie ma fabrycznie audio ani IR**. Przyciski audio są nieaktywne, dopóki kamera nie ogłosi działającego toru audio. Tryb nocny przełącza obraz na monochromatyczny i automatyczną ekspozycję; prawdziwa noktowizja wymaga oświetlacza IR i odpowiedniego sensora bez filtra IR. Nie włącza się biała dioda w pokoju dziecka.
- Wykrywanie dźwięku to próg energii sygnału. **Nie jest rozpoznawaniem płaczu przez AI**. Wykrywanie ruchu porównuje miniatury; ekspozycja lub zmiana światła mogą wywołać alarm.
- Temperatura i wilgotność na podglądzie pochodzą z **sensorów przy pilocie**, nie automatycznie z pokoju kamery. Nie należy traktować ich jako pomiaru pokoju dziecka, jeśli pilot znajduje się gdzie indziej.
- Pierwsza wersja obsługuje jedną skonfigurowaną kamerę i jeden aktywny BME68x (preferuje 0x76, potem 0x77). SCD4x ma stały adres 0x62, więc drugi na tej samej magistrali wymaga multipleksera.
- SCD41 mierzy CO₂. BME688 daje **szacowany eCO₂**, nie pomiar CO₂. IAQ wymaga rozgrzewania i kalibracji; procent dokładności nie jest wymyślany. `iaq_accuracy` 0–3 trafia do logów.
- Procent akumulatora jest orientacyjny i zależy od chemii, obciążenia oraz kalibracji ADC. TP4054 ładuje sprzętowo; jego status nie jest podłączony do MCU. Brak ikony „ładowanie”, czasu do rozładowania i programowego odcinania baterii.
- Sensory ruchu, światła i poziomu hałasu z renderów nie są sensorami BME688/SCD41. Ta wersja nie wyświetla fikcyjnych lx ani dB. Dźwięk ma poziom RMS w jednostkach ADC, nie skalibrowane dB SPL.
- Brak chmury, emaili, powiadomień push na telefon, Bluetooth do wideo, automatycznej jasności bez czujnika światła i wibracji bez silnika. Przewidziana rozbudowa wymaga konkretnych urządzeń/usług.
- Ustawienia 1080p/30fps z referencji nie są obietnicą wydajności tego układu. Lokalny podgląd 224×168 jest skalowany z QVGA 320×240. Górski motyw i ikonki są nowymi grafikami wektorowymi, nie wyciętymi panelami zdjęć.

## SD i nagrania

Zdjęcia mają rozszerzenie `.jpg`. Klipy `.rvc` używają kontenera RVC1: nagłówek ASCII `RVC1`, następnie rekordy `u32 czas_ms + u32 długość_jpeg + jpeg`, wartości little-endian. Czas jest monotoniczny, więc działa także bez Internetu i NTP. Odtwarzacz pokazuje lokalnie klip lub zdjęcie. Lista przedstawia do 12 ostatnio znalezionych plików; kolejność zależy od iteracji katalogu FAT.

Eksport na komputerze:

```bash
python tools/export_recording.py karta/babyphone/plik.rvc
python tools/export_recording.py karta/babyphone/plik.rvc --mp4
```

Drugie polecenie wymaga FFmpeg. Eksporter zachowuje odstępy czasowe klatek, pomija niekompletny ostatni rekord po awarii zasilania i nie przetwarza uszkodzonych długości. Nagranie automatycznie uruchamia odsłuch, jeśli kamera i pilot mają audio. Towarzyszący WAV używa tej samej nazwy; brakujące pakiety stają się ciszą. Odtwarzacz pilota i eksport MP4 odtwarzają obecnie **obraz**; WAV można odtworzyć osobno lub połączyć na PC. PTT jest półdupleksem: w trakcie mówienia odsłuch jest wyciszony. Na utratę Wi-Fi, brak klatek lub błąd SD zapis obrazu kończy się z komunikatem.

Logi CSV zapisują epoch i uptime; przy bezpośrednim połączeniu bez Internetu zegar może pokazywać `--:--`, dopóki NTP nie ustawi czasu. Uptime pozostaje użyteczny. Ustawienie interwału zmienia zapis i krok wykresu; nie zmienia wymaganego harmonogramu BSEC LP / SCD41.

## Audio po stronie AI Thinker — opcjonalne

Ustaw `CAM_AUDIO_ENABLED=1` dopiero po dodaniu zewnętrznego mikrofonu I²S (np. INMP441) i wzmacniacza I²S (np. MAX98357A). Profil używa **I²S1**; I²S0 należy do kamery. Piny przykładowe: BCLK14, LRCLK15, MIC4, SPEAKER13. To wyklucza korzystanie z SD i diody flash na kamerze. GPIO4/15 mają też zachowanie przy rozruchu — sprawdź schemat własnego modułu i poziomy urządzeń zewnętrznych. Mikrofon ustaw na lewy kanał. Wymagane wspólne GND i właściwe zasilanie modułów. Wariant nie został fizycznie sprawdzony na podłączonych modułach.

Tor pilota: ES8311, I²S GPIO4/5/7/8/6, aktywne LOW włączenie wzmacniacza GPIO1. Parametry 16kHz, MCLK 4.096MHz, 16-bit sloty stereo, komunikacja mono. W razie innej rewizji płytki sprawdź adres kodeka 0x18, źródło mikrofonu i mapowanie kanału. Regulacja głośności jest programowa, bez gwarancji maksymalnego poziomu akustycznego.

## Połączenie pogo i hot-plug

I²C15/16 jest współdzielone przez FT6336G i ES8311. Dostęp jest chroniony mutexem; skaner oznacza te układy jako wbudowane. Czujniki typowo: BME688 0x76/0x77, SCD4x 0x62. Adres nie wystarcza do odróżnienia BME680 od BME688 ani SCD40 od SCD41 — dokładny model weryfikuje się sprzętowo; w kodzie używane są zgodne sterowniki rodzin.

Zapewnij 3.3V na liniach SDA/SCL, umiarkowane pull-upy i krótkie przewody. Firmware nie zastępuje układu ochrony ESD, izolacji hot-swap ani dobrej kolejności styków masy/zasilania. Zwarcie wspólnej magistrali przez moduł może czasowo zatrzymać dotyk mimo timeoutu i odzyskiwania. Dla mocniejszej izolacji użyj bufora hot-swap albo drugiej magistrali na wolnych GPIO z nowym profilem sprzętu. Nie zasilaj gołego BME688 napięciem 5V. SCD41 potrzebuje zasilania zdolnego obsłużyć jego impulsy prądowe.

## Pliki i źródła

- `remote/`: kod pilota, `board_config.h`, konfiguracja LVGL i przypięte zależności PlatformIO.
- `camera/`: companion, pinout i konfiguracja sieci/audio.
- `assets/`: własne SVG, PNG i font Montserrat (OFL).
- `docs/PINOUT.md`, `docs/PROTOCOL.md`, `docs/VALIDATION.md`: szczegóły sprzętu, protokołu i sprawdzenia.
- `tools/`: odtworzenie zasobów, renderowanie prawdziwego LVGL na PC, eksport RVC.

Źródła: [LCDWIKI ES3C28P](https://www.lcdwiki.com/2.8inch_ESP32-S3_Display), [schemat](https://www.lcdwiki.com/res/ES3C28P/2.8inch_ESP32-S3_Display_Schematic.pdf), [Bosch BSEC2](https://github.com/boschsensortec/Bosch-BSEC2-Library), [Sensirion SCD4x](https://github.com/Sensirion/arduino-i2c-scd4x), [LVGL 8.3](https://docs.lvgl.io/8.3/), [Espressif ES8311](https://github.com/espressif/esp-bsp/tree/master/components/es8311), [Arduino-ESP32 CameraWebServer](https://github.com/espressif/arduino-esp32/tree/2.0.17/libraries/ESP32/examples/Camera/CameraWebServer). Szczegóły licencji w `NOTICE.md`.

Kompilacja i sprawdzenie desktopowe nie zastępują prób na urządzeniu. Pierwsze uruchomienie wymaga potwierdzenia pinów pogo, wariantu kamery, odczytu baterii miernikiem oraz testów obrazu, odłączania sensorów i audio na docelowym sprzęcie.
