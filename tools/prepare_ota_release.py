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
                notes={'pl':'Naprawa HTTPS OTA: nowe certyfikaty CA, błędy DNS/TLS/HTTP. Zachowano czytelny interfejs.',
                       'de':'HTTPS-OTA: neue CA-Zertifikate, DNS/TLS/HTTP-Diagnose. Gut lesbare Oberfläche bleibt.',
                       'en':'HTTPS OTA fix: updated CA roots, DNS/TLS/HTTP diagnostics. Clear interface retained.'})

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
    (out/'notes.md').write_text(f'''ESP Remote v{version}: poprawka HTTPS OTA. Własny zestaw publicznych CA obejmuje nowe rooty Sectigo E46/R46, których brakowało w starym SDK. Zachowano weryfikację certyfikatu i hosta. Nowe komunikaty rozróżniają DNS, połączenie, TLS z kodem, HTTP 404/403/429 i przekierowania. Log USB 115200 pokazuje etap/kody bez haseł i tokenów. Pierwsze sprawdzenie czeka do 15 s na NTP.

Jeśli V1.3.1/V1.3.2 nie może połączyć się z GitHub przez HTTPS, tę poprawkę wgraj jednorazowo przez USB. Po dotychczasowej instalacji merged i przed udanym OTA: firmware.bin pod 0x10000, bez kasowania pamięci — zachowuje ustawienia. firmware-merged.bin pod 0x0 to czysta instalacja i usuwa ustawienia. Sam zapis pod 0x10000 nie przełącza aktywnego slotu, jeśli wcześniejsze OTA wybrało inny. Szczegóły: docs/OTA_FIX_V1_3_3.md.

Zachowano czytelność V1.3.2: czarne tło, bielszy tekst, mocne kolory, fonty Montserrat SemiBold, widoczne suwaki oraz Test kolorów. Zachowano szybką ścieżkę LCD, pion, PL/DE/EN i schemat NVS.

USB: firmware.bin pod 0x10000 przy dotychczasowym pierwszym slocie aplikacji. firmware-merged.bin pod 0x0 jest dla czystej instalacji i usuwa ustawienia.
Kolejne aktualizacje: System → Aktualizacja OTA → Sprawdź → Zainstaluj → Potwierdź. Wymagany router z internetem i czas NTP.
Przy czystej instalacji wybierz sieć i wpisz własne hasło oraz token kamery. Firmware nie zawiera fabrycznych poświadczeń. Aktualizacja aplikacji zachowuje dane już zapisane w NVS; techniczne ArduinoOTA w LAN uruchamia się tylko z niepustym tokenem.

Zachowano szybką ścieżkę LCD z V1.3.1, pion, PL/DE/EN, sensory, SD, diagnostykę restartu i schemat ustawień NVS. Nie zmieniono kalibracji, inwersji ani transmisji RGB565 działającego LCD. Ekran testowy pozwala porównać kolory panelu; podgląd z renderera nie jest zdjęciem urządzenia.

Kompilacja oraz testy programowe w Actions. Nie wykonano testu na fizycznym pilocie. Automatyczny rollback nie jest skonfigurowany ani deklarowany.
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
