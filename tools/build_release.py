#!/usr/bin/env python3
"""Package already-built ES3C28P application/full images and SHA256 checksums."""
import argparse
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def merge(environment, name, boot_app):
    build = ROOT / 'remote/.pio/build' / environment
    pieces = [(0, build / 'bootloader.bin'), (0x8000, build / 'partitions.bin'),
              (0xE000, boot_app), (0x10000, build / 'firmware.bin')]
    app = pieces[-1][1].read_bytes()
    if not app or app[0] != 0xE9:
        raise ValueError('Invalid ESP application image')
    output = bytearray(b'\xff' * (0x10000 + len(app)))
    previous_end = 0
    for offset, path in pieces:
        payload = path.read_bytes()
        if offset < previous_end or offset + len(payload) > len(output):
            raise ValueError('Overlapping or oversized image: ' + str(path))
        output[offset:offset + len(payload)] = payload
        previous_end = offset + len(payload)
    if output[0] != 0xE9:
        raise ValueError('Missing ESP32-S3 bootloader at 0x0')
    folder = ROOT / 'firmware'
    folder.mkdir(exist_ok=True)
    (folder / (name + '_APP.bin')).write_bytes(app)
    (folder / (name + '_FULL.bin')).write_bytes(output)
    print(name, 'APP', len(app), 'FULL', len(output), 'bytes')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--boot-app', type=Path, required=True,
                        help='framework-arduinoespressif32/tools/partitions/boot_app0.bin')
    args = parser.parse_args()
    merge('es3c28p', 'PILOT_ES3C28P', args.boot_app)
    merge('display_test', 'PILOT_LCD_TEST', args.boot_app)
    folder = ROOT / 'firmware'
    (folder / 'SHA256SUMS.txt').write_text(''.join(
        hashlib.sha256(path.read_bytes()).hexdigest() + '  ' + path.name + '\n'
        for path in sorted(folder.glob('*.bin'))))


if __name__ == '__main__':
    main()
