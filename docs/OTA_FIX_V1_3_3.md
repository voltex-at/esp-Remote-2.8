# V1.3.3 — HTTPS OTA

Na zdjęciu V1.3.1 pilot był połączony z routerem i miał adres IPv4, ale sprawdzenie
aktualizacji kończyło się komunikatem „Brak wydania lub błąd HTTPS”. Ten komunikat
łączył kilka różnych błędów i nie pozwalał ustalić przyczyny na urządzeniu.

Wydanie i plik `ota.json` są publicznie dostępne; sprawdzono pełny łańcuch
przekierowań i odpowiedź HTTP 200. W binarium V1.3.2 potwierdzono użycie starego
domyślnego zestawu CA z Arduino 2.0.17. Wyodrębniony obiekt SDK nie zawiera
Sectigo Public Server Authentication Root E46/R46. To konkretna luka zgodności
z nowymi łańcuchami TLS, ale bez logu USB nie można potwierdzić, że była jedyną
przyczyną błędu na tym pilocie.

V1.3.3 ładuje własny zestaw publicznych CA przed pierwszym połączeniem HTTPS.
Źródło, data i suma pliku są zapisane w `remote/certs/README.md`. Zachowano
weryfikację certyfikatu, nazwy hosta, czasu, docelowego sprzętu oraz SHA-256 obrazu.
Nie używa się `setInsecure` ani HTTP. Karta SD nie jest używana do OTA.

Nowe komunikaty rozróżniają DNS, brak odpowiedzi/połączenia, TLS z kodem błędu,
HTTP 404, odrzucenie HTTP 403/429 i błędne przekierowanie. Log USB 115200 pokazuje
host, etap, kod ESP-IDF/mbedTLS i flagi certyfikatu. Nie zapisuje haseł ani pełnych
adresów przekierowań z tymczasowymi tokenami. Klient wykrywa ucięcie Location,
używa większego bufora żądania oraz czeka do 15 s na pierwszą synchronizację NTP.

## Instalacja naprawy

Jeśli dotychczasowe OTA nie potrafi zestawić HTTPS, poprawki tego klienta nie da się
wgrać przez tę samą niesprawną ścieżkę. Potrzebna jest jednorazowa instalacja USB:

- Po działającej instalacji merged V1.3.1/V1.3.2 i przed jakimkolwiek udanym OTA:
  `bin/firmware.bin` pod **0x10000**, bez kasowania całej pamięci. Zachowuje NVS.
- `bin/firmware-merged.bin` pod **0x0** to czysta instalacja, usuwająca ustawienia.
- Jeżeli aktywny slot został już zmieniony przez wcześniejsze OTA, samo zapisanie
  aplikacji pod 0x10000 nie przełącza slotu. Wymaga to właściwego wyboru slotu albo
  czystej instalacji merged.

Po restarcie wybierz **System → Aktualizacja OTA → Sprawdź aktualizacje**.
Z V1.3.3, gdy jest to najnowsze wydanie, oczekiwany wynik to „Masz aktualną wersję”.
Jeśli nadal wystąpi błąd, zdjęcie nowego komunikatu lub log `[OTA]` pozwoli ustalić
czy chodzi o DNS, router, TLS, czy odpowiedź GitHub.

Testy sprawdzają odtwarzalność binarnego zestawu CA, dokładne klucze i odciski nowych
rootów oraz ich podpisy. Firmware kompiluje się dla ESP32-S3; interfejs i nowe błędy
są testowane w rzeczywistym LVGL PL/DE/EN. Nie wykonano próby HTTPS/OTA na fizycznym
pilocie. Zachowano poprawki LCD i czytelność z V1.3.2 oraz schemat ustawień NVS.
