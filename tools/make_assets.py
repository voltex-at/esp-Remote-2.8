#!/usr/bin/env python3
"""Pinned Lucide SVG icons and original illustration assets, rendered to flash-resident RGB565+A. No SD dependency."""
from pathlib import Path
import io,struct
import cairosvg
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
ASSET=ROOT/'assets';ASSET.mkdir(exist_ok=True)
# SVG icons are vendored from the pinned Lucide revision in assets/LUCIDE_SOURCE.json.
# The complete ISC/Feather license is retained in assets/Lucide-LICENSE.txt.
import json
manifest=json.loads((ASSET/'LUCIDE_SOURCE.json').read_text());names=[*manifest['icons'],*manifest.get('custom_icons',{})]
def svg(body,w=24,h=24):return f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">{body}</svg>'
items=[]
for name in names:
 s=(ASSET/(name+'.svg')).read_text().replace('currentColor','#ffffff')
 items.append((name,s,28,28))
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
 for r,g,b,a in im.get_flattened_data():
  v=((r>>3)<<11)|((g>>2)<<5)|(b>>3);data+=struct.pack('<HB',v,a)
 source.append(f'static const uint8_t data_{name}[]={{'+','.join(str(x) for x in data)+'};')
 source.append(f'const lv_img_dsc_t img_{name}={{{{LV_IMG_CF_TRUE_COLOR_ALPHA,0,0,{w},{h}}},{len(data)},data_{name}}};')
 header.append(f'extern const lv_img_dsc_t img_{name};')
header.append('}')
header.insert(-1, 'extern const lv_img_dsc_t img_mountains_light;')
(ROOT/'remote/include/assets.h').write_text('\n'.join(header)+'\n')
(ROOT/'remote/src/assets.cpp').write_text('\n'.join(source)+'\n')
print('Generated',len(items),'flash assets.')
