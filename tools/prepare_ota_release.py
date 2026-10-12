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
                notes={'pl':'TLS 32512: bufory HTTPS korzystają z PSRAM, diagnostyka RAM. Zachowano większy UI i stronę główną.',
                       'de':'TLS 32512: HTTPS-Puffer nutzen PSRAM, RAM-Diagnose. Große Oberfläche und Startseite bleiben.',
                       'en':'TLS 32512: HTTPS buffers use PSRAM, RAM diagnostics. Larger UI and unchanged home retained.'})

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
    (out/'notes.md').write_text(f'''ESP Remote v{version}: poprawka błędu TLS 32512 (0x7F00, nieudana alokacja RAM). Domyślne SDK lokowało bufory TLS wyłącznie w RAM wewnętrznym. Wspierany alokator mbedTLS instalowany przed zadaniami sieciowymi preferuje PSRAM dla dużych buforów, z zapasową pulą. Zachowano zerowanie calloc i prawidłowe free. Log USB 115200 pokazuje wolną pamięć i największe bloki. Nowe komunikaty odróżniają brak RAM od błędu certyfikatu. Szczegóły: docs/TLS_MEMORY_V1_3_5.md.

V1.3.1–V1.3.4 z niedziałającym HTTPS wymagają jednorazowego USB. Po dotychczasowej instalacji merged i bez udanego OTA: firmware.bin pod 0x10000, bez Erase all — zachowuje ustawienia. Firmware-merged.bin pod 0x0 jest dla czystej instalacji i usuwa ustawienia. Sam zapis aplikacji nie przełącza slotu, jeśli wcześniejsze OTA wybrało inny.

Zachowano cały większy UI V1.3.4: strona główna piksel w piksel PL/DE/EN, napisy 20/18 px, duże pola dotykowe, przewijane listy i duża klawiatura. Zachowano certyfikaty, host/SHA-256 i potwierdzenie instalacji. Testy obejmują odtworzenie braku RAM, alokator, rzeczywisty LVGL i kompilację. Nie wykonano połączenia HTTPS/OTA na fizycznym pilocie.

Po restarcie sprawdź: System → Aktualizacja OTA → Sprawdź. Wymagany router z internetem i czas NTP. Karta SD pozostaje na nagrania/logi; aktualizacja aplikacji zachowuje NVS. Przy czystej instalacji wybierz sieć i wpisz własne hasło. GitHub OTA nie wymaga tokena kamery; techniczne ArduinoOTA w LAN uruchamia się tylko z niepustym tokenem.

Nie zmieniono sterownika LCD, orientacji ani schematu NVS. Kompilacja i testy programowe w Actions. Test HTTPS/OTA na fizycznym pilocie wymaga potwierdzenia. Automatyczny rollback nie jest skonfigurowany.
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
