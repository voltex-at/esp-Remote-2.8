#!/usr/bin/env python3
"""Build the ESP-IDF 4.4 subject/public-key bundle from committed PEM roots.

Format matches Espressif gen_crt_bundle.py (Apache-2.0). No network is used.
"""
import re
import struct
from pathlib import Path
from cryptography import x509
from cryptography.hazmat.primitives import serialization

ROOT = Path(__file__).resolve().parents[1]

def build(pem):
    certificates = [x509.load_pem_x509_certificate(c) for c in re.findall(
        rb'-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----', pem, re.S)]
    if not 1 <= len(certificates) <= 200:
        raise ValueError('Root count exceeds the ESP-IDF 4.4 bundle limit')
    records = sorted((c.subject.public_bytes(), c.public_key().public_bytes(
        serialization.Encoding.DER, serialization.PublicFormat.SubjectPublicKeyInfo)) for c in certificates)
    if len({name for name, _ in records}) != len(records):
        raise ValueError('Duplicate root subject')
    return struct.pack('>H', len(records)) + b''.join(
        struct.pack('>HH', len(name), len(key)) + name + key for name, key in records)

if __name__ == '__main__':
    out = ROOT / 'remote/certs/ota_roots.bin'
    out.write_bytes(build((out.with_suffix('.pem')).read_bytes()))
    print(f'{out.name}: {out.stat().st_size} bytes')
