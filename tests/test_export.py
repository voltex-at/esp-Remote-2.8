import unittest,tempfile,subprocess,struct,sys
from pathlib import Path
from PIL import Image
import io
script=Path(__file__).resolve().parents[1]/'tools/export_recording.py'
class RvcTests(unittest.TestCase):
 def jpeg(self):
  f=io.BytesIO();Image.new('RGB',(32,24),'#88b56a').save(f,format='JPEG');return f.getvalue()
 def run_export(self,content):
  tmp=tempfile.TemporaryDirectory();self.addCleanup(tmp.cleanup);p=Path(tmp.name)/'sample.rvc';p.write_bytes(content);r=subprocess.run([sys.executable,str(script),str(p)],capture_output=True,text=True);return r,p.with_suffix('')
 def test_timing_and_complete_jpegs(self):
  jpg=self.jpeg();data=b'RVC1'+b''.join(struct.pack('<II',ts,len(jpg))+jpg for ts in [0,130,370]);r,out=self.run_export(data);self.assertEqual(r.returncode,0,r.stderr);self.assertEqual(len(list(out.glob('*.jpg'))),3);self.assertIn('duration 0.130000',(out/'frames.txt').read_text());self.assertIn('duration 0.240000',(out/'frames.txt').read_text());img=Image.open(out/'frame_000000.jpg');self.assertEqual(img.size,(32,24));img.close()
 def test_interrupted_write_preserves_complete_frame(self):
  jpg=self.jpeg();r,out=self.run_export(b'RVC1'+struct.pack('<II',0,len(jpg))+jpg+b'\x01\x02');self.assertEqual(r.returncode,0,r.stderr);self.assertEqual(len(list(out.glob('*.jpg'))),1)
 def test_invalid_large_length_rejected(self):
  r,_=self.run_export(b'RVC1'+struct.pack('<II',0,102401));self.assertNotEqual(r.returncode,0)
 def test_nonmonotonic_time_rejected(self):
  jpg=self.jpeg();r,_=self.run_export(b'RVC1'+b''.join(struct.pack('<II',ts,len(jpg))+jpg for ts in [400,100]));self.assertNotEqual(r.returncode,0)
 def test_wrong_magic_rejected(self):
  r,_=self.run_export(b'WRONG');self.assertNotEqual(r.returncode,0)
if __name__=='__main__':unittest.main()
