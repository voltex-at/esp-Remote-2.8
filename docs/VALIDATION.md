# Walidacja V1.2 — 2026-10-11

## Wykonano

- Nowe testy natywne `portrait_test.cpp`: surowe współrzędne FT6336G, pion0/180°, granice240×320, przycinanie prostokątów — OK.
- `display_transport_test.cpp` kompiluje rzeczywisty `portrait_display.cpp` z atrapą SPI. Dwa warianty rotacji:0 i2. Sprawdzono komendy inicjalizacji, opóźnienia120/120/20ms, piny, CS/DC, MADCTL0x48/0x88, okna adresowe, kolejność bajtów RGB565, pojedyncze/nieparzyste piksele i bufor wyrównany tylko do2bajtów. AddressSanitizer/UndefinedBehaviorSanitizer: bez błędów. Atrapa odwzorowuje odczyty32-bitowe w Arduino2 `writePixels`; nie jest to pomiar sygnałów na GPIO.
- V1.2 nie zmienia struktury `Settings`, przestrzeni NVS `remote-v1`, logów SD ani protokołu kamery.

- Kompilacja pilota `pio run -d remote -e es3c28p -j 4`: ESP32-S3 N16R8, PlatformIO 6.1.18 / espressif32 6.9.0 / Arduino 2.0.17. Link z właściwym blobem BSEC2 dla ESP32-S3. Wynik SUCCESS. APP ma 1448928 bajtów (partycja 5MB), statyczny RAM 156004 bajty. Obiekty LVGL i większe bufory pracują w PSRAM; zadania i bufory audio także potrzebują wolnego RAM podczas działania. Bufor LCD jest w wewnętrznym RAM i używa synchronicznego SPI.
- Niezależny `display_test`: SUCCESS, APP 301712 bajtów, statyczny RAM 19388 bajtów. Korzysta z tego samego sterownika LCD, bez LVGL i bez wymogu PSRAM w `setup()`. Nowe normalne i testowe obrazy APP/FULL mają sumy SHA256 w `firmware/SHA256SUMS.txt`.
- Kamera bez zmian względem V1.1: obrazy i źródła zachowane z załączonej paczki. Poprzednia kompilacja `camera -e esp32cam`: SUCCESS, około883kB aplikacji /53kB statycznego RAM.
- Poprzednia kompilacja `camera -e esp32cam_audio`: SUCCESS, około910kB aplikacji /55kB statycznego RAM. Ten wariant sprawdza kod opcjonalnych I²S mikrofonu i wzmacniacza; nie potwierdza okablowania. Kamer nie kompilowano ponownie w tej poprawce LCD.
- Natywny renderer z **tym samym `remote/src/ui.cpp`**, LVGL8.3.11 i tymi samymi fontami/assetami:21 ekranów ×3języki =63 rendery, 3 widoki hasła, 3 stany skanu Wi-Fi, dodatkowo ekran offline, 150 zmian stanu i przejść. AddressSanitizer i UndefinedBehaviorSanitizer nie zgłosiły błędów. LeakSanitizer jest wyłączony, ponieważ środowisko nie daje dostępu do listy wątków w `/proc`; nie jest to test wycieków.
- Przegląd podglądów: poprawiono grubość fontu, brakujące polskie/symboliczne glify, styl przełączników, długości wierszy, układ skanowania pogo oraz ikonę baterii. Długie listy są przewijane; długość niektórych etykiet jest skracana kropkami, a ich pełne dane edytuje się klawiaturą.
- Testy nowych callbacków LVGL: wybór wszystkich 3 języków; uruchomienie skanu z Połączenia; klawisze liter, wielkich liter, cyfr i backspace; ukrywanie/podgląd hasła; odrzucenie za krótkiego hasła i błędnego klucza 64-znakowego; akceptacja klucza HEX; zapis SSID/hasła przed Connect; anulowanie bez zmiany ustawień; wyczyszczenie poprzedniego hasła przy wyborze sieci otwartej. Wszystkie OK.
- Natywny test agregacji sieci: ukryty SSID, najsilniejszy AP, oddzielenie sieci otwartej i WPA2 o tej samej nazwie, sortowanie, limit 16 wyników. Wszystkie OK.
- Pięć testów eksportera RVC: dokładne odstępy czasowe, prawdziwe JPEG, zachowanie kompletnej klatki po urwaniu zapisu, odrzucenie błędnego nagłówka, za dużej długości i malejącego czasu. Wszystkie OK.

## Odtworzenie sprawdzenia

```bash
pio run -d remote -j 4
pio run -d remote -e display_test -j 4
pio run -d camera -e esp32cam -j 4
pio run -d camera -e esp32cam_audio -j 4
python -m unittest discover -s tests -v
python tools/render_ui.py --lvgl remote/.pio/libdeps/es3c28p/lvgl --jobs 4
g++ -std=c++17 -I remote/include tests/wifi_logic_test.cpp -o wifi_logic_test
./wifi_logic_test
g++ -std=c++17 -I remote/include tests/portrait_test.cpp -o portrait_test
./portrait_test
g++ -std=c++17 -I tests/display_stub -I remote/include tests/display_transport_test.cpp remote/src/portrait_display.cpp -o display_transport_test
./display_transport_test
g++ -std=c++17 -DREMOTE_LCD_ROTATION=2 -I tests/display_stub -I remote/include tests/display_transport_test.cpp remote/src/portrait_display.cpp -o display_transport_test_180
./display_transport_test_180
```

Renderer potrzebuje GCC/G++, Python/Pillow. `make_assets.py` wymaga CairoSVG. Kompilacja fontów jest opcjonalna, gdyż pliki C są gotowe; `make_fonts.sh` korzysta z lv_font_conv1.5.3. Dane przykładowe istnieją tylko w `tools/ui_simulator.cpp`, nie w `remote/src`.

## Pozostaje na docelowym sprzęcie

Najpierw sprawdź instrukcję `docs/LCD_FIX_V1_2.md`: poprawiony firmware i niezależny
LCD_TEST, kolory, dotyk w rogach, log startu i działanie ekranu błędu pamięci.
Bez logu z urządzenia nie rozstrzygnięto pierwotnej przyczyny białego obrazu.

Nie było podłączonej płytki, sensorów ani kamery. Dla V1.1 trzeba fizycznie sprawdzić skan dostępnych sieci 2.4 GHz, Direct i router, prawidłowe/błędne hasło, brak AP, sieć otwartą i ukryty SSID, stabilność podglądu podczas skanu oraz zachowanie ustawień po restarcie. Nie testowano rzeczywistych asocjacji radiowych ani uwierzytelniania WPA3. Nie potwierdzono fizycznie: kolorów i orientacji ekranu, współrzędnych dotyku, działania GPIO46w danej rewizji, adresu kodeka i kanału mikrofonu, stabilności hot-plugpogo, wydajności karty, temperatury BME688 w obudowie, kalibracji ADC, jakości i opóźnień audio/wideo, reakcji na zaniki zasilania. Zmierz napięcie baterii i potwierdź fizyczny pinout pogo przed pierwszym podłączeniem modułu. Dla kamery innej niż AIThinker zmień profil sprzętu.

To są testy kompilacji, parsera i interfejsu na PC, a nie potwierdzenie gotowości całego urządzenia po testach laboratoryjnych. Brakujące fizyczne moduły audio/IR/światła/ruchu i usługi chmurowe nie są symulowane jako działające.
