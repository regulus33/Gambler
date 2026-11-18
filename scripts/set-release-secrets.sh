#!/bin/bash
# Stores the iOS release signing material as GitHub Actions secrets in the "app-store" environment.
# Run it yourself in Terminal (it asks for the .p12 password); nothing is printed or written to disk.
#
#   ./scripts/set-release-secrets.sh \
#       --p12 ~/Downloads/AppleDistribution.p12 \
#       --app-profile ~/Downloads/Gambler_App_Store.mobileprovision \
#       --auv3-profile ~/Downloads/Gambler_AUv3_App_Store.mobileprovision \
#       --asc-key ~/Downloads/AuthKey_ABC123XYZ9.p8 --asc-issuer 00000000-0000-0000-0000-000000000000
#
# Any option can be left out to update only some secrets (e.g. just new profiles after a renewal).
# Needs the GitHub CLI (gh) logged in with access to the repo. See docs/RELEASING.md.
set -euo pipefail

REPO="${REPO:-regulus33/Gambler}"
ENV_NAME="app-store"
p12="" app_profile="" auv3_profile="" asc_key="" asc_issuer=""

while [ $# -gt 0 ]; do
    case "$1" in
        --p12) p12="$2"; shift 2 ;;
        --app-profile) app_profile="$2"; shift 2 ;;
        --auv3-profile) auv3_profile="$2"; shift 2 ;;
        --asc-key) asc_key="$2"; shift 2 ;;
        --asc-issuer) asc_issuer="$2"; shift 2 ;;
        -h|--help) sed -n 2,13p "$0"; exit 0 ;;
        *) echo "Unknown option: $1" >&2; exit 1 ;;
    esac
done

set_secret() { # <name> <value>
    printf '%s' "$2" | gh secret set "$1" --repo "$REPO" --env "$ENV_NAME"
    echo "  ✓ $1"
}
need_file() { [ -f "$1" ] || { echo "Not found: $1" >&2; exit 1; }; }

# Make sure the environment exists (creating it is harmless if it already does)
gh api --method PUT "repos/$REPO/environments/$ENV_NAME" >/dev/null
echo "Setting secrets in $REPO → environment '$ENV_NAME':"

if [ -n "$p12" ]; then
    need_file "$p12"
    read -r -s -p "Password for $(basename "$p12"): " p12_password; echo
    # Check the password and that it really is an Apple Distribution identity before storing anything
    openssl pkcs12 -in "$p12" -passin "pass:$p12_password" -nokeys -legacy 2>/dev/null | grep -q "Apple Distribution" \
        || openssl pkcs12 -in "$p12" -passin "pass:$p12_password" -nokeys 2>/dev/null | grep -q "Apple Distribution" \
        || { echo "Wrong password, or not an Apple Distribution certificate." >&2; exit 1; }
    set_secret IOS_DIST_CERT_P12 "$(base64 < "$p12")"
    set_secret IOS_DIST_CERT_PASSWORD "$p12_password"
    unset p12_password
fi

for pair in "IOS_APP_PROFILE:$app_profile" "IOS_AUV3_PROFILE:$auv3_profile"; do
    name="${pair%%:*}" file="${pair#*:}"
    [ -n "$file" ] || continue
    need_file "$file"
    echo "  $(basename "$file"): $(security cms -D -i "$file" | plutil -extract Name raw -)"
    set_secret "$name" "$(base64 < "$file")"
done

if [ -n "$asc_key" ]; then
    need_file "$asc_key"
    [ -n "$asc_issuer" ] || { echo "--asc-key needs --asc-issuer too" >&2; exit 1; }
    key_id=$(basename "$asc_key" .p8); key_id="${key_id#AuthKey_}"
    set_secret ASC_KEY_P8 "$(base64 < "$asc_key")"
    set_secret ASC_KEY_ID "$key_id"
    set_secret ASC_ISSUER_ID "$asc_issuer"
fi

echo "Done. Start a test build: gh workflow run release-ios.yml --repo $REPO"
