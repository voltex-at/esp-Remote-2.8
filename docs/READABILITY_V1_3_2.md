# Czytelność — V1.3.2

Po potwierdzeniu przez użytkownika działania V1.3.1 poprawiono wygląd interfejsu.
Nowy render LVGL: [podgląd](READABILITY_V1_3_2.png). Wartości w nim są przykładowe;
jest to render kodu `ui.cpp`, a nie fotografia fizycznego panelu.

- Neutralne czarne tło, białe teksty, jaśniejsze wartości pomocnicze i obramowania.
  Nasycona limonkowa zieleń, czerwony i błękit zastępują pastelowe akcenty.
- Montserrat SemiBold, główne opisy 16 px, nagłówki do 18 px. Długie nazwy dopasowują
  się do miejsca, zamiast ginąć pod wielokropkiem. Mniejsze fonty pozostają do
  objaśnień i w miejscach o ograniczonej szerokości.
- Nazwa i wartość ustawienia są w osobnych wierszach. Długie wartości SSID/IP
  przewijają się poziomo. Długie listy mają widoczny pasek przewijania.
- Grubsze ikony i wykresy. Lista sensorów zachowuje grafikę modułu pogo, ale daje
  więcej szerokości nazwie pomiaru. Ekran baterii pokazuje napięcie bez skrótów GPIO.
- Jawna nieprzezroczystość ścieżki, wskaźnika i uchwytu suwaka: bez domyślnego
  motywu LVGL te elementy wcześniej mogły pozostać niewidoczne. Procent zmienia się
  podczas ruchu; zapis ustawienia następuje po zwolnieniu suwaka.
- Nieaktywne przyciski i przełączniki są odróżnione od aktywnych.
- Test kolorów w **Ustawienia → Ogólne → Wyświetlacz → Test kolorów** pokazuje pełne
  R/G/B, czerń, szarość, biel i kolor aktywny. Jeśli panel nadal wygląda blado,
  fotografia tego ekranu przy ustawionej jasności pomaga ocenić problem.

Parametry transmisji RGB565, kolejność bajtów, inwersja LCD i inicjalizacja z V1.3.1
pozostają zgodne z działającym sterownikiem. Nie dodano eksperymentalnej korekcji gamma.
Fonty i grafiki są w firmware; SD nadal służy nagraniom i logom. Schemat NVS i
rozmiar `Settings` nie zostały zmienione.
Nowa instalacja ma puste SSID, hasło i token kamery; użytkownik wpisuje własne wartości.
Aktualizacja aplikacji nadal odczytuje istniejący blob NVS bez zmiany konfiguracji.
ArduinoOTA w LAN nie jest uruchamiane bez tokena. GitHub OTA pozostaje niezależne
od tokena kamery i wymaga połączenia z internetem.

Walidacja: kompilacja pilota i niezależnego testu LCD, renderer rzeczywistego LVGL
z ASan/UBSan, 66 ekranów PL/DE/EN oraz widoki hasła, skanu i OTA. Sprawdzane są
również geometria rozdzielonych nazw/wartości, pełna nazwa na stronie głównej,
widoczność suwaka, procent i callback zapisu jasności. GitHub Actions powtarza
testy przed publikacją. Ostateczny wygląd i kolory wymagają oceny na panelu użytkownika.

Instalacja: OTA z V1.3.1 przez router z internetem lub USB. `firmware.bin` pod 0x10000
dotyczy pierwszego slotu aplikacji; `firmware-merged.bin` pod 0x0 odtwarza cały układ
czystej instalacji i usuwa ustawienia. Nie używaj obrazu merged jako pliku OTA.
