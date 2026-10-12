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
                notes={'pl':'Nowy wygląd podstron: płaskie karty, czytelniejsza typografia, prostsze teksty i odczyty sensorów na żywo. Strona główna bez zmian.',
                       'de':'Neue Unterseiten: flache Karten, klare Schrift, kürzere Texte und Live-Sensorwerte. Startseite unverändert.',
                       'en':'Redesigned subpages: flat cards, clear typography, shorter copy and live sensor readings. Home unchanged.'})


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
    (out/'notes.md').write_text(f'''ESP Remote v{version}: nowy wygląd podstron. Płaskie grafitowe karty bez jasnych obramowań, akcent limonkowy dla aktywnych funkcji, tytuły SemiBold i teksty pomocnicze Medium 18 px. Ujednolicono nawigację i klawiaturę; skrócono i poprawiono teksty PL/DE/EN. Sensory pokazują aktualne pomiary bez otwierania szczegółów. Ekran OTA oddziela wersję, widoczny komunikat stanu i duże przyciski. Podgląd: docs/MODERN_UI_V1_3_6.png. Szczegóły: docs/MODERN_UI_V1_3_6.md.

Strona główna pozostaje piksel w piksel identyczna w trzech językach. Zachowano duże pola dotykowe (minimum 44 × 44 px), sterownik LCD, orientację 240 × 320, ustawienia NVS oraz poprawkę pamięci TLS z V1.3.5. Karta SD pozostaje na nagrania i logi.

Instalacja na pilocie: Ustawienia → System → Aktualizacja → Sprawdź → Aktualizuj → Potwierdź. Potrzebne Wi-Fi przez router z internetem oraz zsynchronizowany czas. V1.3.1–V1.3.4 z niedziałającym HTTPS wymagają jednorazowego USB. Po dotychczasowej instalacji merged i bez udanego OTA: firmware.bin pod 0x10000, bez Erase all — zachowuje ustawienia. Sam zapis aplikacji nie przełącza slotu, jeśli wcześniejsze OTA wybrało inny. firmware-merged.bin pod 0x0 służy do czystej instalacji i usuwa ustawienia.

Weryfikacja: rzeczywisty renderer LVGL, trzy języki, brak nakładania elementów list, osiągalność końca listy, dotyk i klawiatura, bieżące odczyty sensorów, mieszczące się komunikaty OTA, potwierdzenie instalacji i blokady podczas zapisu. Kompilacja obu wariantów i testy OTA/TLS w Actions. Podgląd używa przykładowych danych; nie jest zdjęciem urządzenia. Wymagane potwierdzenie wyglądu i aktualizacji na fizycznym pilocie.
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
