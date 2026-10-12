#!/usr/bin/env python3
"""Render a light palette of the original SVG mountains into a flash asset."""
from pathlib import Path
import io
import cairosvg
from PIL import Image

root = Path(__file__).resolve().parents[1]
svg = (root / 'assets/mountains.svg').read_text()
colors = {'#000000':'#f3f5f8', '#11191e':'#e4ebf2', '#43545b':'#b4c3d3',
          '#96a4a5':'#f9fcff', '#1b292f':'#879caf', '#52636b':'#c6d4e2',
          '#0a1319':'#607a92', '#5b6a70':'#e8f0f8'}
for old, new in colors.items():
    svg = svg.replace(old, new)
(root / 'assets/mountains_light.svg').write_text(svg)
image = Image.open(io.BytesIO(cairosvg.svg2png(bytestring=svg.encode(),
                                              output_width=240, output_height=110))).convert('RGB')
image.save(root / 'assets/mountains_light.png')
pixels = [((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3) for r, g, b in image.get_flattened_data()]
rows = [','.join(f'0x{v:04x}' for v in pixels[i:i+24]) for i in range(0, len(pixels), 24)]
(root / 'remote/src/assets_light.cpp').write_text(
    '#include "assets.h"\nstatic const uint16_t data_mountains_light[]={\n' +
    ',\n'.join(rows) + '\n};\nconst lv_img_dsc_t img_mountains_light='
    '{{LV_IMG_CF_TRUE_COLOR,0,0,240,110},sizeof data_mountains_light,'
    '(const uint8_t*)data_mountains_light};\n')
print('Generated opaque RGB565 light mountains: 240 x 110, 52800 bytes.')
