#!/usr/bin/env python3
"""Extract exact-timed JPEG frames from RVC1. Optional ffmpeg -> MP4 (video only)."""
import argparse,struct,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('recording',type=Path);p.add_argument('--out',type=Path);p.add_argument('--mp4',action='store_true');a=p.parse_args()
out=a.out or a.recording.with_suffix('');out.mkdir(parents=True,exist_ok=True)
frames=[]
with a.recording.open('rb') as f:
 if f.read(4)!=b'RVC1': raise SystemExit('Not an RVC1 recording')
 while True:
  header=f.read(8)
  if not header:break
  if len(header)!=8:print('Incomplete tail ignored');break
  timestamp,n=struct.unpack('<II',header)
  if n<4 or n>102400:raise SystemExit('Invalid frame length')
  jpg=f.read(n)
  if len(jpg)!=n:print('Incomplete last frame ignored');break
  if not jpg.startswith(b'\xff\xd8') or not jpg.endswith(b'\xff\xd9'):raise SystemExit('Invalid JPEG')
  if frames and timestamp<frames[-1][0]:raise SystemExit('Nonmonotonic timestamp')
  name=f'frame_{len(frames):06d}.jpg';(out/name).write_bytes(jpg);frames.append((timestamp,name))
if not frames:raise SystemExit('No complete frames')
manifest=[]
for i,(ts,name) in enumerate(frames):
 duration=(frames[i+1][0]-ts)/1000 if i+1<len(frames) else .125
 manifest.extend([f"file '{name}'",f'duration {max(duration,.001):.6f}'])
manifest.append(f"file '{frames[-1][1]}'")
(out/'frames.txt').write_text('\n'.join(manifest)+'\n')
if a.mp4:
 subprocess.run(['ffmpeg','-y','-f','concat','-safe','0','-i',str(out/'frames.txt'),'-fps_mode','vfr','-pix_fmt','yuv420p',str(out.with_suffix('.mp4'))],check=True)
print(f'{len(frames)} frames extracted to {out}')
