# Lokalny protokół companion v1

Połączenie Direct jest Wi-Fi AP→STA bez routera. Tryb Router to wspólny WLAN 2.4GHz. To nie ESP-NOW: obraz i audio potrzebują normalnego IP; ESP-NOW nie został przedstawiony jako transport wideo.

HTTP port80, obowiązkowy nagłówek `X-Pair-Token`. Token jest taki sam w pilocie i `CAM_PAIR_TOKEN` w kamerze. Brak uwierzytelnienia → 401. Urządzenia nie wysyłają danych do usług chmurowych.

| Żądanie | Wynik |
|---|---|
| `GET /capture` | `image/jpeg`, QVGA 320×240, Content-Length |
| `GET /status` | JSON z protocol=1, cap.audio/motion/infrared/cry_ai, night, motion_enabled, noise_enabled, motion_seq, noise_seq |
| `GET /control?var=night&val=0/1` | Monochromatyczne / kolorowe; IR tylko z dodatkowym sprzętem i konfiguracją |
| `GET /control?var=motion&val=0/1` | Włącz/wyłącz porównywanie miniatur |
| `GET /control?var=noise&val=0/1` | Próg RMS, tylko z aktywnym audio |
| `GET http://kamera:81/stream` | Standardowy multipart MJPEG; też wymaga nagłówka tokenu |

Pilot pobiera pojedyncze JPEG co najmniej co 125ms (docelowy limit8fps), jeśli sieć i kamera pozwalają. Korzysta z oddzielnego zadania, więc blokady HTTP nie są wykonywane w LVGL. Przechwytywanie jest ciągłe, również poza ekranem Live, aby monitorować ruch. Nie otwieraj równocześnie wielu ciężkich strumieni: kamera ma ograniczone zasoby. Pilota skonfiguruj adresem hosta bez `http://` i ścieżki; port80 jest stały.

## Audio UDP

Port3333 po obu stronach. PCM mono 16kHz signed16 little-endian. Każda pełna paczka ma 584 bajty:

| Offset | Rozmiar | Pole |
|---|---|---|
| 0 | 4 | ASCII BAP1 |
| 4 | 4 | Numer sekwencji u32 LE |
| 8 | 64 | Token NUL padded; maksymalnie63 znaków |
| 72 | 512 | 256 próbek PCM s16 LE (16ms) |

Pakiet 72-bajtowy jest heartbeat/żądaniem subskrypcji audio. Kamera pamięta nadawcę przez2sekundy; pilot wysyła heartbeat co500ms podczas odsłuchu. Pakiet584B od pilota oznacza PTT: kamera odtwarza dane i wstrzymuje wysyłanie audio, aby ograniczyć sprzężenie. Pilot weryfikuje adres źródła, magic, długość i token. Token w lokalnym protokole nie jest szyfrowaniem; zabezpieczenie transmisji zapewnia lokalna sieć Wi-Fi. V1 nie implementuje korekcji błędów, jitter-buffer, AEC ani retransmisji UDP; zakłócenia mogą powodować przerwy.

## RVC1

Little-endian, magic RVC1 + powtarzane rekordy (timestamp_ms u32, jpeg_length u32, jpeg bytes). Limit pojedynczego JPEG100KiB, klipu5minut. WAV PCM16kHz/mono/s16 ma tę samą nazwę podstawową i oddzielny plik. CSV sensorów nie jest mieszany z nagraniami. Program eksportujący jest w `tools/export_recording.py`.
