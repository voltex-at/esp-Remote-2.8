#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
# MontserratSemiBold.ttf is the weight-600 instance of the bundled OFL font.
# Fonts are committed, so firmware builds do not need Node or font conversion.
for size in 12 14 16 18 20 22; do
  npx --yes lv_font_conv@1.5.3 --font assets/MontserratSemiBold.ttf \
    --range 0x20-0x7f,0xa0-0x17f,0x2013-0x2014,0x2022,0x2026,0x203a,0x2082,0x2260 \
    --font assets/Symbols.ttf --range 0x25cb,0x25cf \
    --size "$size" --bpp 4 --format lvgl --no-compress \
    --output "remote/src/font_ui${size}.c"
done

# Medium weight for secondary text on modern subpages (still 18 px).
npx --yes lv_font_conv@1.5.3 --font assets/MontserratMedium.ttf \
  --range 0x20-0x7f,0xa0-0x17f,0x2013-0x2014,0x2022,0x2026,0x203a,0x2082,0x2260 \
  --font assets/Symbols.ttf --range 0x25cb,0x25cf \
  --size 18 --bpp 4 --format lvgl --no-compress --output remote/src/font_body18.c
python3 - <<'PYFONT'
from pathlib import Path
p=Path('remote/src/font_body18.c')
p.write_text(p.read_text().rstrip()+'\n')
PYFONT
