import importlib.util
from pathlib import Path
import unittest
spec=importlib.util.spec_from_file_location('release',Path(__file__).parents[1]/'tools/prepare_ota_release.py')
release=importlib.util.module_from_spec(spec);spec.loader.exec_module(release)
class ManifestTests(unittest.TestCase):
    def image(self):
        data=bytearray(256);data[0]=0xe9;data[12]=9;data[32:36]=b'\x32\x54\xcd\xab';return bytes(data)
    def test_metadata_tracks_actual_bytes(self):
        a=release.metadata(self.image(),'1.3.0');b=release.metadata(self.image()+b'x','1.3.0')
        self.assertNotEqual(a['sha256'],b['sha256']);self.assertEqual(b['size'],257)
        self.assertEqual(a['target'],'espremote-es3c28p-16mb-portrait')
        self.assertTrue(a['url'].endswith('/v1.3.0/firmware.bin'))
    def test_reject_wrong_chip_merged_size_and_invalid_version(self):
        image=bytearray(self.image());image[12]=0
        for data,version in [(bytes(image),'1.3.0'),(b'bad','1.3.0'),(self.image(),'v1.3.0'),(self.image(),'1.3'),(self.image(),'1.3.0/evil'),(self.image(),'65536.0.0'),(self.image()+b'x'*0x500000,'1.3.0')]:
            with self.assertRaises(ValueError):release.metadata(data,version)
if __name__=='__main__':unittest.main()
