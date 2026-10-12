#!/usr/bin/env python3
"""Package a compiled app for GitHub OTA; never publish a merged USB image as OTA."""
import argparse
import hashlib
import json
import re
import shutil
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPOSITORY = 'voltex-at/esp-Remote-2.8'
TARGET = 'espremote-es3c28p-16mb-portrait'

def metadata(app, version):
    if not re.fullmatch(r'(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)', version):
        raise ValueError('Expected a stable x.y.z version')
    if any(int(n)>65535 for n in version.split('.')):
        raise ValueError('Version component too large')
    if not 256 <= len(app) <= 0x500000 or app[0]!=0xe9 or app[12:14]!=b'\x09\x00' or app[32:36]!=b'\x32\x54\xcd\xab':
        raise ValueError('Not an ESP32-S3 application fitting the 5MB OTA slot')
    return dict(schema=1, target=TARGET, version=version, size=len(app),
                sha256=hashlib.sha256(app).hexdigest(),
                url=f'https://github.com/{REPOSITORY}/releases/download/v{version}/firmware.bin',
                notes={'pl':'Zgodne procenty baterii, piorun i animacja zasilania szacowanego z napięcia. Większa data i ikony na stronie głównej. Sześć sensorów na jednym ekranie z alarmami i nową ikoną pogo.',
                       'de':'Einheitliche Akkuwerte, Blitz und Ladeanimation aus Spannung geschätzt. Größeres Datum und Symbole auf der Startseite. Sechs Sensorwerte mit Alarmen auf einer Seite und neues Pogo-Symbol.',
                       'en':'Consistent battery percentages, voltage-estimated power bolt and charge animation. Larger home date and icons. Six sensor readings with alerts on one screen and a refreshed pogo icon.'})


def main():
    p=argparse.ArgumentParser();p.add_argument('--out',type=Path,default=ROOT/'release');a=p.parse_args()
    version=re.search(r'FIRMWARE_VERSION\[\]="([^"]+)"',(ROOT/'remote/include/board_config.h').read_text()).group(1)
    app=(ROOT/'firmware/PILOT_ES3C28P_APP.bin').read_bytes()
    full=(ROOT/'firmware/PILOT_ES3C28P_FULL.bin').read_bytes()
    if full[0x10000:]!=app or full[0]!=0xe9:
        raise ValueError('USB/OTA build mismatch')
    manifest=metadata(app,version);out=a.out;out.mkdir(parents=True,exist_ok=True)
    (out/'firmware.bin').write_bytes(app);(out/'firmware-merged.bin').write_bytes(full)
    (out/'ota.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
    for name in ('PILOT_LCD_TEST_APP.bin','PILOT_LCD_TEST_FULL.bin'):
        shutil.copy2(ROOT/'firmware'/name,out/name)
    (out/'notes.md').write_text(f'''ESP Remote v{version}\n\n# V1.3.9 — bateria i czytelne sensory

Procent w nagłówku i na stronie baterii pochodzi z tej samej próbki AppState.
Oba wskazania aktualizują się na bieżąco, także po zmianie poziomu zasilania.
Poprzednio duży procent odświeżał się dopiero po ponownym wejściu na stronę.

Pomiar ADC co 500 ms używa mediany 16 próbek i filtrowania napięcia. Zachowano
krzywą procentową dla baterii; próg 4,1 V nie wymusza sztucznego wyniku 100%.

| Zaobserwowane napięcie | Wskazanie |
|---|---|
| Wzrost o co najmniej 30 mV w oknie 2 s, poniżej 4,1 V | Piorun i animowane zielone segmenty; ładowanie szacowane |
| Co najmniej 4,1 V przez 2 s | Piorun pozostaje, segmenty pokazują poziom bez animacji |
| Spadek poniżej 4,06 V o co najmniej 65 mV od maksimum przez 2 s | Powrót do wskaźnika baterii bez pioruna |
| Niewielkie wahania przy 4,1 V | Histereza utrzymuje ikonę zasilania bez migotania |
| Niepoprawny pomiar ADC | Kreska zamiast procentu i napięcia |

Wykrywanie opiera się na obserwacji użytkownika dotyczącej tej płytki. Samo
napięcie nie potwierdza obecności kabla, prądu ładowania ani zakończenia ładowania.
Na stronie znajduje się informacja, że poziom i zasilanie są szacowane. Bateria
o wysokim napięciu po odłączeniu kabla może przez pewien czas zachować piorun.

Strona baterii mieści poziom, napięcie, ikonę i stan na jednym ekranie, również
po niemiecku. W obu motywach ma zielone segmenty; niski poziom bez zasilania
jest oznaczony czerwienią. Animacja i piorun są także widoczne w nagłówkach.

Strona główna ma większą godzinę, datę 18 px oraz większe Wi-Fi i baterię.
Data pochodzi z NTP; bez czasu sieciowego widoczny jest placeholder. Wi-Fi ma
grubszy kontur. Ikona sensorów przedstawia moduł z kontaktami pogo — to oryginalny
rysunek zapisany jako SVG i zasób firmware, bez zależności od karty SD.

Ekran sensorów pokazuje wszystkie sześć pomiarów w dwóch kolumnach i trzech
wierszach, bez przewijania. Kafelki 108 × 76 px zawierają duże wartości, nazwy,
jednostki oraz stany OK / Alarm / Brak. Kolor alarmu reaguje na ustawione progi;
nieaktualny pomiar jest zastępowany kreską. Dotknięcie kafelka otwiera wykres,
a koło zębate prowadzi do ustawień i skanowania I²C.

![Rzeczywisty renderer LVGL; przykładowe dane](BATTERY_SENSORS_V1_3_9.png)

Testy obejmują krzywą procentową, szum, okno wzrostu, opóźnienia, histerezę,
odłączenie, błędne napięcie i przepełnienie licznika czasu. Renderer PL/DE/EN
sprawdza zgodność obu procentów, 100%, animację i nieruchomy piorun, widoczność
wszystkich pomiarów, alarmy, brak danych oraz dotyk. Dotychczasowe testy kamery,
OTA, TLS, klawiatury i obu motywów pozostają w procesie wydania.

Ustawienia i schemat NVS nie zmieniły się. OTA zachowuje konfigurację oraz SD.
Testy na fizycznej płytce pozostają do wykonania, zwłaszcza zachowanie wskaźnika
przy podłączaniu i odłączaniu USB w różnych stanach naładowania.
''')
    checks=[]
    for path in sorted(out.iterdir()):
        if path.suffix in ('.bin','.json'):
            checks.append(hashlib.sha256(path.read_bytes()).hexdigest()+'  '+path.name+'\n')
    (out/'SHA256SUMS.txt').write_text(''.join(checks))
    with zipfile.ZipFile(out/f'ESPRemote-v{version}-SOURCE_BIN.zip','w',zipfile.ZIP_DEFLATED) as z:
        for path in sorted(ROOT.rglob('*')):
            rel=path.relative_to(ROOT)
            if not path.is_file() or any(part in ('.git','.pio','.sim_build','__pycache__','release') for part in rel.parts):continue
            if path.suffix in ('.o','.pyc','.ppm') or re.match(r'remote/cc[^/]+\.(?:o|s|c|ld|le)$',rel.as_posix()):continue
            z.write(path,'source/'+rel.as_posix())
        for path in sorted(out.iterdir()):
            if path.suffix in ('.bin','.json','.txt','.md'):z.write(path,'bin/'+path.name)
    print(version)

if __name__=='__main__':main()
