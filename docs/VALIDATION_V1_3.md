# V1.3.0 — GitHub OTA

Walidacja programowa przed publikacją:

- Pilot ES3C28P, Arduino2.0.17 / LVGL8.3.11: SUCCESS, RAM159224B,
  aplikacja1665664B, partycja5MiB.
- Niezależny test LCD/dotyku: SUCCESS, aplikacja301712B.
- Test OTA: wersje starsze/równe, błędny format, granice hostów i adresów,
  format SHA-256, nagłówek/deskryptor aplikacji ESP32-S3 — OK.
- Siedem testów Pythona: eksport nagrań i metadane wydania — OK.
- Renderer rzeczywistego `ui.cpp`: wcześniejsze ekrany i callbacki Wi-Fi/języka,
  oraz OTA w3językach: sprawdzenie nie instaluje, dwuklik wiąże instalację
  z wybraną wersją, widok62%, wyłączone przyciski, zablokowany powrót,
  wszystkie kody błędów — OK, AddressSanitizer/UndefinedBehaviorSanitizer bez błędów.
  LeakSanitizer wyłączony ze względu na ograniczony dostęp do `/proc`.
- Osobne obrazy USB/OTA oraz manifest są generowane z tego samego builda;
  skrypt porównuje część aplikacji w obrazie FULL i wylicza SHA-256 z jej bajtów.
- Wybrano ESP-IDF `esp_crt_bundle_attach` z domyślnym osadzonym pakietem CA
  w `libmbedtls`, zamiast kolidującego nagłówka Arduino2. Nie wyłączono TLS.

GitHub Actions ponawia kompilację i testy przed opublikowaniem wydania.
Aktualny wynik i logi są dostępne w zakładce Actions repozytorium.

Nie było fizycznego pilota. Pozostają testy pobrania HTTPS na urządzeniu,
instalacji do drugiej partycji, restartu, zachowania ustawień i przerwania zasilania.
Nie potwierdzono laboratoryjnie radia/LCD/sensorów. Automatyczny rollback
nie jest skonfigurowany. Pierwsza instalacja GitHub OTA wymaga USB.
Dokument `VALIDATION.md` opisuje odziedziczoną walidację V1.2.
