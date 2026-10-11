# LCD i diagnostyka restartu — V1.3.1

Na nagraniu sprzętu widać rysowanie ekranu startowego pionowymi pasami, chwilowo
poprawne menu, następnie ciemny ekran i ponowne rysowanie. Film nie zawiera logu USB,
więc nie potwierdza, czy restart powoduje panic, watchdog, zasilanie lub inny problem.
Nie deklarujemy, że usunięto przyczynę restartów bez testu na urządzeniu.

Potwierdzony błąd kodu V1.3.0: sterownik nadpisywał `writeFillRect`, ale funkcja
`Adafruit_GFX::fillScreen` z biblioteki 1.11.11 wywołuje `fillRect`, następnie
`writeFastVLine`, `drawFastVLine` i `writePixel`. Brak nadpisania publicznego `fillRect`
oznaczał 76800 osobnych transakcji i okien adresowych dla jednego ekranu 240×320.
Powolne przejście od lewej do prawej z filmu odpowiada tej kolejności rysowania.

Poprawka kieruje obie warstwy API GFX i linie poziome/pionowe do transferu blokowego.
Pełne wypełnienie ma jedno okno adresowe i 153600 bajtów RGB565. Nie zmieniono pinów,
częstotliwości SPI, inicjalizacji kontrolera ani orientacji. Ostatni nieparzysty piksel
wypełnienia wysyłany jest oddzielnie, aby Arduino2 nie odczytywało dodatkowych bajtów.
Wcześniejszy adapter GFX w teście miał inną kolejność wywołań niż biblioteka i ukrywał
ten błąd; poprawiono adapter oraz dodano test publicznego API przez klasę bazową GFX.
Nowy test odtworzono na sterowniku z V1.3.0: nie przechodził sprawdzenia jednego
okna pełnego wypełnienia. Na V1.3.1 przechodzi dla pionu 0° i 180° z ASan/UBSan,
włącznie z obciętymi prostokątami i nieparzystą liczbą pikseli.

## Wgranie i sprawdzenie

1. Wgraj `firmware.bin` przez USB pod **0x10000** przy dotychczasowej tabeli partycji.
   Nie kasuj całej pamięci. Zachowuje to konfigurację NVS V1.2/V1.3.0.
2. Ekran startowy powinien pojawić się bez wielosekundowego rysowania pasami.
   Sprawdź, czy menu pozostaje widoczne przez co najmniej minutę.
3. Jeśli ekran ponownie zgaśnie albo start się powtórzy, wgraj
   `PILOT_LCD_TEST_APP.bin` pod **0x10000**. Test pokazuje pasy RGB i licznik `ALIVE`;
   nie uruchamia Wi-Fi, SD, LVGL ani audio i nie wymaga PSRAM do rysowania.
   Obserwuj go przez minutę i sprawdź dotyk czterech rogów.
4. Po teście przywróć `firmware.bin` pod **0x10000**. USB nie aktualizuje wyboru slotu OTA;
   powyższe adresy dotyczą aplikacji uruchamianej z fabrycznego pierwszego slotu.
   Jeśli wcześniej instalowano przez OTA i aktywny jest drugi slot, użyj aktualizacji
   OTA z działającego menu lub świadomie odtwórz układ czystą instalacją FULL pod 0x0
   (usuwa ustawienia). Nie zalecamy kasowania pamięci jako pierwszego kroku diagnozy.

## Log potrzebny do ustalenia restartu

Monitor USB **115200 baud**: uruchom zapis, naciśnij RESET i zbierz całość z dwóch
uruchomień. Oprócz ewentualnego `Guru Meditation`, backtrace i komunikatu brownout:

- `[BOOT RESET]` podaje nazwę i numer powodu resetu oraz etap poprzedniego startu,
- `[BOOT STAGE]` podaje kolejne etapy, czas, wolną pamięć i minimalny zapas stosu,
- `[HEALTH]` co 10 sekund potwierdza czas działania i zapas pamięci/stosu pętli,
- test LCD drukuje `[LCD TEST] uptime=...`; licznik na ekranie umożliwia obserwację bez logu.

Etap w pamięci RTC jest wskazówką: crash w równoległym zadaniu może nastąpić w czasie
innego etapu głównego zadania. Po utracie zasilania RTC nie przechowuje historii.
Log nie wypisuje haseł Wi-Fi ani tokenów parowania.

Poprawkę kompiluje i testuje GitHub Actions przed publikacją. Test sprzętowy musi
wykonać użytkownik: do środowiska kompilacji nie podłączono fizycznego pilota.
