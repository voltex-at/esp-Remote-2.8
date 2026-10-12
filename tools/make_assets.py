#!/usr/bin/env python3
"""Original SVG assets, rendered to flash-resident RGB565+A. No SD dependency."""
from pathlib import Path
import io,struct
import cairosvg
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
ASSET=ROOT/'assets';ASSET.mkdir(exist_ok=True)
icons={
'camera':'<path d="M3 7h4l2-3h6l2 3h4v13H3Z"/><circle cx="12" cy="13" r="4"/>',
'sensor':'<circle cx="12" cy="12" r="2"/><path d="M8 6a8 8 0 0 0 0 12m8-12a8 8 0 0 1 0 12M5 3a13 13 0 0 0 0 18M19 3a13 13 0 0 1 0 18"/>',
'settings':'<path d="m10 2 4 0 1 3 3 1 3-1 2 4-2 2v3l2 2-2 4-3-1-3 1-1 3h-4l-1-3-3-1-3 1-2-4 2-2v-3L1 9l2-4 3 1 3-1Z"/><circle cx="12" cy="12" r="4"/>',
'link':'<path d="m9 15 6-6M8 17l-2 2a4 4 0 0 1-6-6l5-5a4 4 0 0 1 6 0m2-1 2-2a4 4 0 0 1 6 6l-5 5a4 4 0 0 1-6 0" transform="translate(1 0)"/>',
'mic':'<rect x="9" y="2" width="6" height="13" rx="3"/><path d="M5 10v2a7 7 0 0 0 14 0v-2M12 19v3m-4 0h8"/>',
'speaker':'<path d="M3 9h4l5-5v16l-5-5H3ZM16 8a6 6 0 0 1 0 8m3-11a10 10 0 0 1 0 14"/>',
'temp':'<path d="M9 14V5a3 3 0 0 1 6 0v9a5 5 0 1 1-6 0ZM12 6v12m6-10h3m-3 4h3"/><circle cx="12" cy="18" r="1"/>',
'drop':'<path d="M12 2C10 7 4 11 4 15a8 8 0 0 0 16 0c0-4-6-8-8-13Z"/><path d="M8 14c-1 3 1 4 3 5"/>',
'co2':'<path d="M5 18a5 5 0 0 1-1-10 7 7 0 0 1 13-2 5 5 0 1 1 2 12Z"/><path d="M7 13h1m3 0h1m3 0h1"/>',
'air':'<path d="M2 8h13a3 3 0 1 0-3-3M2 12h18a3 3 0 1 1-3 3M2 17h8a3 3 0 1 1-3 3"/>',
'pressure':'<circle cx="12" cy="12" r="9"/><path d="m12 12 4-5M6 15h12M5 9h2m10 0h2M12 3v2"/>',
'bell':'<path d="M5 17h14l-2-3V9a5 5 0 0 0-10 0v5ZM10 21h4M12 2v2"/>',
'wifi':'<path d="M2 8a16 16 0 0 1 20 0M5 12a11 11 0 0 1 14 0M8 16a6 6 0 0 1 8 0"/><circle cx="12" cy="20" r="1"/>',
'battery':'<rect x="2" y="6" width="18" height="12" rx="2"/><path d="M22 10v4M6 9v6m4-6v6m4-6v6"/>',
'moon':'<path d="M20 14A9 9 0 1 1 10 3a7 7 0 0 0 10 11Z"/>',
'sun':'<circle cx="12" cy="12" r="4"/><path d="M12 1v3m0 16v3M1 12h3m16 0h3M4 4l2 2m12 12 2 2M4 20l2-2M18 6l2-2"/>',
'info':'<circle cx="12" cy="12" r="10"/><path d="M12 10v8M12 6v1"/>',
'video':'<rect x="2" y="5" width="13" height="14" rx="2"/><path d="m15 9 7-4v14l-7-4Z"/>',
'folder':'<path d="M2 5h8l2 3h10v13H2Z"/>',
'home':'<path d="m2 11 10-9 10 9M5 10v12h5v-7h4v7h5V10"/>',
'clock':'<circle cx="12" cy="12" r="10"/><path d="M12 5v8l5 3"/>',
'scan':'<path d="M2 8V2h6m8 0h6v6M2 16v6h6m8 0h6v-6M3 12h18"/>',
'check':'<path d="m4 12 5 5L21 5"/>',
'back':'<path d="m15 3-9 9 9 9"/>',
'motion':'<circle cx="15" cy="3" r="2"/><path d="m10 9 4-3 4 4h4M14 6l-3 8 5 3 2 5M11 14l-5 5H2M10 9l-3 4H3"/>',
'play':'<path d="m7 3 14 9-14 9Z"/>',
'plus':'<path d="M12 3v18M3 12h18"/>',
'power':'<path d="M12 2v10M7 4a9 9 0 1 0 10 0"/>',
'globe':'<circle cx="12" cy="12" r="10"/><ellipse cx="12" cy="12" rx="4" ry="10"/><path d="M2 12h20M4 6h16M4 18h16"/>'}
def svg(body,w=24,h=24):return f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">{body}</svg>'
items=[]
for name,body in icons.items():
 s=svg('<g stroke="#ffffff" fill="none" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">'+body+'</g>')
 (ASSET/(name+'.svg')).write_text(s);items.append((name,s,28,28))
mountain=svg('''<defs><linearGradient id="sky" x2="0" y2="1"><stop stop-color="#000000"/><stop offset="1" stop-color="#11191e"/></linearGradient></defs><rect width="240" height="110" fill="url(#sky)"/>
<path fill="#43545b" d="M0 95 20 65 43 78 66 52 93 80 125 27 140 36 155 17 173 41 202 57 221 38 240 65V110H0Z"/>
<path fill="#96a4a5" d="m66 52-13 30 13-13 18 11Zm59-25-32 53 27-21 5-20 15-3 15-19-14 30 32-6-19 11-14-5-14 13ZM221 38l-19 19 22-4 16 12Z"/>
<path fill="#1b292f" d="M0 105 44 76 73 104 125 27 118 68 149 45 173 41 160 82 221 38 208 86 240 65v45H0Z"/>
<path fill="#52636b" d="m125 27-27 55 14-8 6-6Zm24 18-32 43 20-10Zm72-7-50 64 30-22Z"/>
<path fill="#0a1319" d="M0 110 57 94 76 108 104 87 124 105 151 79 168 101 189 75 206 104 240 87v23Z"/>
<g stroke="#5b6a70" stroke-width="1" opacity=".5"><path d="m101 77-17 25m27-31-12 32m41-52-26 42m19-26-16 30m76-22-23 32m38-45-25 39"/></g>''',240,110)
(ASSET/'mountains.svg').write_text(mountain);items.append(('mountains',mountain,240,110))
module=svg('''<path fill="#283020" stroke="#828a65" stroke-width="1.5" d="m5 12 12-8h56l17 12-5 24-13 8H17L5 36Z"/><path fill="#5c6446" d="m17 4 5 10h57L73 4Zm-12 8 13 9 4-7-5-10ZM79 14l11 2-5 24-10-9Z"/><path fill="#11180f" stroke="#818768" d="M18 20h57v15H18Z"/><circle cx="46" cy="24" r="7" fill="#101817" stroke="#5b685d"/><circle cx="46" cy="24" r="3" fill="#253b44"/><g fill="#cfaa4a" stroke="#f9ce65" stroke-width=".5"><rect x="27" y="33" width="3" height="10" rx="1"/><rect x="35" y="33" width="3" height="10" rx="1"/><rect x="43" y="33" width="3" height="10" rx="1"/><rect x="51" y="33" width="3" height="10" rx="1"/><rect x="59" y="33" width="3" height="10" rx="1"/></g><g fill="#12170f" stroke="#828b69"><circle cx="15" cy="33" r="3"/><circle cx="77" cy="37" r="3"/></g>''',96,52)
(ASSET/'module.svg').write_text(module);items.append(('module',module,64,35))
header=['#pragma once','#include <lvgl.h>','extern "C" {']
source=['#include "assets.h"']
for name,s,w,h in items:
 raw=cairosvg.svg2png(bytestring=s.encode(),output_width=w,output_height=h)
 im=Image.open(io.BytesIO(raw)).convert('RGBA');im.save(ASSET/(name+'.png'))
 data=bytearray()
 for r,g,b,a in im.getdata():
  v=((r>>3)<<11)|((g>>2)<<5)|(b>>3);data+=struct.pack('<HB',v,a)
 source.append(f'static const uint8_t data_{name}[]={{'+','.join(str(x) for x in data)+'};')
 source.append(f'const lv_img_dsc_t img_{name}={{{{LV_IMG_CF_TRUE_COLOR_ALPHA,0,0,{w},{h}}},{len(data)},data_{name}}};')
 header.append(f'extern const lv_img_dsc_t img_{name};')
header.append('}')
header.insert(-1, 'extern const lv_img_dsc_t img_mountains_light;')
(ROOT/'remote/include/assets.h').write_text('\n'.join(header)+'\n')
(ROOT/'remote/src/assets.cpp').write_text('\n'.join(source)+'\n')
print('Generated',len(items),'original assets.')
