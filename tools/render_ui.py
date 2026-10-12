#!/usr/bin/env python3
import argparse,subprocess,concurrent.futures
from pathlib import Path
from PIL import Image,ImageChops,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--lvgl',type=Path,required=True);p.add_argument('--build',type=Path,default=root/'tools/.sim_build');p.add_argument('--jobs',type=int,default=4);p.add_argument('--update-home-reference',action='store_true');a=p.parse_args();build=a.build.resolve();build.mkdir(parents=True,exist_ok=True)
inc=['-I'+str(root/'remote/include'),'-I'+str(a.lvgl.resolve()),'-DLV_CONF_INCLUDE_SIMPLE','-DLV_LVGL_H_INCLUDE_SIMPLE','-O1','-g','-pipe','-fsanitize=address,undefined','-fno-omit-frame-pointer']
files=list((a.lvgl/'src').rglob('*.c'))+list((root/'remote/src').glob('font_*.c'))
def compile(src):
 obj=build/(str(src.resolve()).replace('/','_')+'.o');subprocess.run(['gcc',*inc,'-c',str(src),'-o',str(obj)],check=True,stdout=subprocess.DEVNULL);return obj
with concurrent.futures.ThreadPoolExecutor(max_workers=max(1,a.jobs)) as e:objects=list(e.map(compile,files))
exe=build/'ui_preview';subprocess.run(['g++',*inc,'-std=c++17',str(root/'tools/ui_simulator.cpp'),str(root/'remote/src/ui.cpp'),str(root/'remote/src/assets.cpp'),str(root/'remote/src/assets_light.cpp'),*[str(o) for o in objects],'-lm','-o',str(exe)],check=True)
out=root/'docs/ui';out.mkdir(parents=True,exist_ok=True);subprocess.run([str(exe),str(out)],check=True)
for f in out.glob('*.ppm'):Image.open(f).save(f.with_suffix('.png'));f.unlink()
if a.update_home_reference:
    for lang,key in enumerate(('pl','de','en')):
        Image.open(out/f'ui_{lang}_00.png').save(root/f'tests/ui_reference/home_{key}.png')
for lang,key in enumerate(('pl','de','en')):
    reference=Image.open(root/f'tests/ui_reference/home_{key}.png').convert('RGB')
    actual=Image.open(out/f'ui_{lang}_00.png').convert('RGB')
    if ImageChops.difference(reference,actual).getbbox() is not None:
        raise AssertionError(f'Home screen changed in {key}; must remain pixel-identical')
print('Home screen PL/DE/EN: pixel-identical to V1.3.11 home reference.')
print(out)

# A shareable proof from actual firmware objects; keep native aspect ratios.
preview=Image.new('RGB',(1008,694),'#16212e');draw=ImageDraw.Draw(preview)
font=ImageFont.truetype(str(root/'assets/MontserratMedium.ttf'),18)
tiles=(('camera_portrait_0_0.png','Kamera · pion'),('camera_offline_portrait_0.png','Brak obrazu · pion'),
       ('ui_0_00.png','Wyrównane wskaźniki'),('camera_landscape_controls_0_0.png','Kamera · poziom'),
       ('camera_offline_landscape_0.png','Brak obrazu · poziom'),('camera_landscape_volume_0_0.png','Głośność · poziom'))
for i,(name,caption) in enumerate(tiles):
    x=12+(i%3)*332;y=12+(i//3)*352
    draw.text((x,y),caption,font=font,fill='#f7f9fc')
    screen=Image.open(out/name);preview.paste(screen,(x+(320-screen.width)//2,y+26))
draw.text((12,659),'LVGL · V1.3.11 · przykładowe dane i obraz testowy',font=font,fill='#b2becb')
preview.save(root/'docs/CAMERA_ORIENTATION_V1_3_11.png')
