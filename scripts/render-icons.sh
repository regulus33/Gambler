#!/bin/bash
# Renders the app icons from their SVG sources using headless Google Chrome:
#   packaging/icon-ios.svg   -> packaging/ios/Images.xcassets/AppIcon.appiconset/AppIcon-1024.png
#                               (full-bleed and opaque; iOS applies its own mask, Xcode makes the smaller sizes)
#   packaging/icon-macos.svg -> packaging/icon.png
#                               (rounded tile with transparent margin; JUCE builds the .icns from it)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CHROME="${CHROME:-/Applications/Google Chrome.app/Contents/MacOS/Google Chrome}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

render() { # <svg> <png> <opaque: yes|no>
    cat > "$TMP/icon.html" <<HTML
<!doctype html><html><body style="margin:0;background:transparent">
<img src="file://$1" width="1024" height="1024" style="display:block">
</body></html>
HTML
    rm -f "$TMP/icon.png"
    # Chrome sometimes fails when launched right after a previous headless run; retry a few times
    for attempt in 1 2 3; do
        "$CHROME" --headless=new --disable-gpu --hide-scrollbars --force-device-scale-factor=1 \
            --allow-file-access-from-files --default-background-color=00000000 --window-size=1024,1024 \
            --screenshot="$TMP/icon.png" "file://$TMP/icon.html" >/dev/null 2>&1 || true
        [ -s "$TMP/icon.png" ] && break
        sleep 2
    done
    [ -s "$TMP/icon.png" ] || { echo "Chrome failed to render $1" >&2; exit 1; }
    if [ "$3" = yes ]; then
        # Round-trip through BMP to drop the alpha channel (App Store icons must be opaque)
        sips -s format bmp "$TMP/icon.png" --out "$TMP/icon.bmp" >/dev/null
        sips -s format png "$TMP/icon.bmp" --out "$2" >/dev/null
    else
        cp "$TMP/icon.png" "$2"
    fi
    echo "$2  ($(sips -g pixelWidth -g pixelHeight -g hasAlpha "$2" | awk '/: /{printf "%s ", $2}'))"
}

render "$ROOT/packaging/icon-ios.svg" "$ROOT/packaging/ios/Images.xcassets/AppIcon.appiconset/AppIcon-1024.png" yes
render "$ROOT/packaging/icon-macos.svg" "$ROOT/packaging/icon.png" no
