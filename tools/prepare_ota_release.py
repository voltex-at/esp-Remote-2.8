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
                notes={'pl':'Wyraźniejsze jasne karty, osobne Audio i Kamera, duże kafelki sensorów, pełnoekranowy podgląd z przyciskami na obrazie i nowe ikony.',
                       'de':'Deutlichere helle Karten, separate Audio- und Kamera-Einstellungen, große Sensorkacheln, Vollbildkamera mit Bedienelementen im Bild und neue Symbole.',
                       'en':'Stronger light-theme cards, separate Audio and Camera settings, large sensor tiles, full-screen video with overlay controls and refreshed icons.'})


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
    (out/'notes.md').write_text(f'''ESP Remote v{version}\n\n# V1.3.8 — sensory, kamera i osobne audio

Jasny motyw ma ciemniejsze obramowania kart i większą różnicę między kartą a tłem.
Ustawienia zawierają osobne strony Kamera i Audio. Audio grupuje głośność, odsłuch,
alarm dźwięku i mówienie; Kamera grupuje obraz, tryb nocny, ruch i nagrania.

Sensory mają sześć dużych kafelków w dwóch kolumnach: nazwa, jednostka, aktualna
wartość i wskaźnik dostępności. Dotknięcie kafelka otwiera wykres. Dalsze pomiary
i ustawienia są dostępne przez przewijanie; przycisk plus otwiera wyszukiwanie.
Brak lub nieaktualne dane pokazują kreskę zamiast starego odczytu.

Podgląd kamery wypełnia ekran 240 × 320 bez nagłówka Kamera 1. Przyciski na obrazie
obsługują powrót, cały kadr/powiększenie, głośność, ustawienia kamery, przytrzymanie
mikrofonu, zdjęcie, nagrywanie i odsłuch. Dotknięcie wolnej części obrazu chowa lub
pokazuje przyciski. Przełączenie kadru nie zmienia pliku zdjęcia ani nagrania.
Cały kadr zachowuje obraz 4:3 i czarne pasy; powiększenie przycina boki.

Bufory obrazu 320 × 240 używają PSRAM. Brak obrazu jest sygnalizowany, a funkcje
wymagające kamery, audio lub SD są blokowane, gdy te zasoby są niedostępne.
Przyciski mają pola dotykowe co najmniej 44 × 44 px.

Ikony w całym interfejsie pochodzą z jednej wersji Lucide. Źródło i licencja są
w assets/LUCIDE_SOURCE.json i assets/Lucide-LICENSE.txt. Ciemna strona główna ma
zachowany układ, tekst i kolory; zmieniono na niej wyłącznie ikonki.

![Rzeczywisty renderer LVGL; przykładowe pomiary i obraz testowy](DASHBOARD_CAMERA_V1_3_8.png)

Sprawdzono 150 ekranów PL/DE/EN w obu motywach, duże cele dotykowe, przewijanie,
klawiaturę, aktualizacje odczytów, nawigację do wykresu, rozdział ustawień audio,
przyciski i gesty podglądu, zapis głośności oraz działanie OTA w rendererze LVGL.
Test geometrii sprawdza zachowanie proporcji, przycięcie oraz pokrycie całego
bufora podczas dekodowania QVGA i VGA. Testy OTA, TLS, motywu, LCD i pakowania
pozostają w procesie wydania. Testy na fizycznym pilocie pozostają do wykonania.

Aktualizacja OTA zachowuje konfigurację i dane SD. Po wcześniejszej instalacji
merged, bez późniejszego przełączenia slotu OTA, plik firmware.bin można wgrać
przez USB pod 0x10000 bez Erase all. Po udanym OTA użyj ekranu aktualizacji;
sam zapis pod 0x10000 nie wybiera aktywnej partycji. Obraz firmware-merged.bin
pod 0x0 przeznaczono do czystej instalacji i usuwa ustawienia.
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
