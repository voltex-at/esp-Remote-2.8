#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
# npm install lv_font_conv@1.5.3; MontserratMedium.ttf is the weight-500 instance.
for size in 12 14 18; do
  npx lv_font_conv --font assets/MontserratMedium.ttf \
    --range 0x20-0x7f,0xa0-0x17f,0x2013-0x2014,0x2022,0x2026,0x203a,0x2082,0x2260 \
    --font assets/Symbols.ttf --range 0x25cb,0x25cf \
    --size "$size" --bpp 4 --format lvgl --no-compress \
    --output "remote/src/font_ui${size}.c"
done
