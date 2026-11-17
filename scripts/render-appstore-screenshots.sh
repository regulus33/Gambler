#!/bin/bash
# Renders the captioned App Store screenshots from packaging/appstore/frame.html
# into packaging/appstore/screenshots/ at iPad 13" landscape size (2752 x 2064, opaque PNG).
#
# The app captures inside them are iPad mini landscape shots in packaging/appstore/source/:
#   mini-scale.png   (Scale view)    mini-melody.png   (Melody view)
# Retake them from the iPad mini simulator (rotated to landscape) with:
#   xcrun simctl io booted screenshot /tmp/shot.png && sips -r 270 /tmp/shot.png --out packaging/appstore/source/mini-scale.png
# (simctl saves in the device's native portrait orientation, hence the rotation.)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
KIT="$ROOT/packaging/appstore"
OUT="$KIT/screenshots"
CHROME="${CHROME:-/Applications/Google Chrome.app/Contents/MacOS/Google Chrome}"
W=2752; H=2064
COUNT=6

mkdir -p "$OUT"
rm -f "$OUT"/*.png
TMP="$(mktemp -d)"

# Serve the kit over HTTP so fonts and images load the same way they do in a browser
PORT=8799
python3 -m http.server "$PORT" --bind 127.0.0.1 --directory "$KIT" >/dev/null 2>&1 &
SERVER=$!; disown
trap 'kill $SERVER 2>/dev/null; rm -rf "$TMP"' EXIT
sleep 1

for n in $(seq 1 $COUNT); do
    "$CHROME" --headless=new --disable-gpu --hide-scrollbars --force-device-scale-factor=1 \
        --virtual-time-budget=5000 --window-size="$W,$H" \
        --screenshot="$TMP/shot.png" "http://127.0.0.1:$PORT/frame.html?n=$n" >/dev/null 2>&1
    # Drop the alpha channel: App Store Connect rejects transparent screenshots
    sips -s format bmp "$TMP/shot.png" --out "$TMP/shot.bmp" >/dev/null
    sips -s format png "$TMP/shot.bmp" --out "$OUT/ipad-$n.png" >/dev/null
    echo "$OUT/ipad-$n.png  ($(sips -g pixelWidth -g pixelHeight "$OUT/ipad-$n.png" | awk '/pixel/{printf "%s ", $2}'))"
done
