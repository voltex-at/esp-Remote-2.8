import hashlib
import importlib.util
import re
import struct
import unittest
from pathlib import Path
from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('bundle', ROOT / 'tools/make_ota_bundle.py')
bundle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bundle)

class TrustBundleTests(unittest.TestCase):
    def test_committed_bundle_matches_pinned_source(self):
        pem = (ROOT / 'remote/certs/ota_roots.pem').read_bytes()
        self.assertEqual(hashlib.sha256(pem).hexdigest(),
                         '50a6277ec69113f00c5fd45f09e8b97a4b3e32daa35d3a95ab30137a55386cef')
        data = (ROOT / 'remote/certs/ota_roots.bin').read_bytes()
        self.assertEqual(data, bundle.build(pem))
        count = struct.unpack_from('>H', data)[0]
        self.assertLessEqual(count, 200)
        offset, records = 2, {}
        for _ in range(count):
            n, k = struct.unpack_from('>HH', data, offset)
            offset += 4
            name, key = data[offset:offset+n], data[offset+n:offset+n+k]
            offset += n+k
            serialization.load_der_public_key(key)
            records[name] = key
        self.assertEqual(offset, len(data))
        self.assertEqual(list(records), sorted(records))
        # These new server roots are absent from the SDK's old default bundle.
        # Check both exact CA fingerprints and the keys loaded by ESP-IDF lookup.
        required = {
            'Sectigo Public Server Authentication Root E46': 'c90f26f0fb1b4018b22227519b5ca2b53e2ca5b3be5cf18efe1bef47380c5383',
            'Sectigo Public Server Authentication Root R46': '7bb647a62aeeac88bf257aa522d01ffea395e0ab45c73f93f65654ec38f25a06',
        }
        found = set()
        for block in re.findall(rb'-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----', pem, re.S):
            cert = x509.load_pem_x509_certificate(block)
            names = cert.subject.get_attributes_for_oid(x509.oid.NameOID.COMMON_NAME)
            cn = names[0].value if names else ''
            if cn in required:
                self.assertEqual(cert.fingerprint(hashes.SHA256()).hex(), required[cn])
                self.assertEqual(records[cert.subject.public_bytes()], cert.public_key().public_bytes(
                    serialization.Encoding.DER, serialization.PublicFormat.SubjectPublicKeyInfo))
                cert.verify_directly_issued_by(cert)
                found.add(cn)
        self.assertEqual(found, set(required))

    def test_invalid_input_cannot_generate_empty_trust(self):
        with self.assertRaises(ValueError):
            bundle.build(b'no certificates')

if __name__ == '__main__':
    unittest.main()
