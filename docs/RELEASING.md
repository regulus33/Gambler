# Releasing Gambler for iPad

Releases are built, signed and uploaded to App Store Connect by GitHub Actions ([`.github/workflows/release-ios.yml`](../.github/workflows/release-ios.yml)) on a `macos-26` runner. App Store Connect only accepts builds made with Xcode 26 or later, so local Xcode 16 builds can't be uploaded.

## Cutting a release

1. Bump [`VERSION`](../VERSION) (e.g. `1.0.1`) and commit it.
2. Tag and push:
   ```bash
   git tag v1.0.1 && git push origin main v1.0.1
   ```
3. The workflow builds the app and AUv3, signs them, uploads to App Store Connect and creates a GitHub release. The build number is `<run number>.<attempt>`, so it goes up with every upload.
4. When Apple has processed the build (10–30 minutes, you get an email), open App Store Connect → the app → the new version, pick the build, update *What's New*, and **Submit for Review**.

The tag has to match `VERSION` (`v` + version), or the workflow stops before building.

**Test build without uploading:** Actions → *Release (iOS)* → *Run workflow*, leave **upload** off. It builds, signs and exports the app but doesn't send it anywhere. Without the signing secrets it still runs as an unsigned build check.

## One-time setup

You need an Apple Developer Program membership (team `TFM56WJ6SZ`).

### 1. App IDs (developer.apple.com → Certificates, Identifiers & Profiles → Identifiers)

Register two explicit App IDs, with no extra capabilities:

| Description | Bundle ID |
|---|---|
| Gambler | `com.tonegarden.Gambler` |
| Gambler AUv3 | `com.tonegarden.Gambler.GamblerAUv3` |

These come from `BUNDLE_ID` and `PRODUCT_NAME` in `CMakeLists.txt` and must never change after the first release.

### 2. Distribution certificate

Xcode → Settings → Accounts → select the team → **Manage Certificates…** → **+** → **Apple Distribution**. Then in Keychain Access → *My Certificates*, right-click **Apple Distribution: Zachary Rowden (TFM56WJ6SZ)** → **Export…** → save as `.p12` with a strong password.

### 3. Provisioning profiles (Certificates, Identifiers & Profiles → Profiles → +)

Create two **App Store Connect** distribution profiles, one per App ID above, both using the Apple Distribution certificate from step 2. Name them e.g. *Gambler App Store* and *Gambler AUv3 App Store* and download them.

### 4. App Store Connect API key (for uploading)

App Store Connect → Users and Access → Integrations → App Store Connect API → **Team Keys** → **+**. Give it **App Manager** access (enough to upload; no admin rights needed). Download the `.p8` (you can only download it once) and note the **Issuer ID** shown above the key list.

### 5. App record

App Store Connect → Apps → **+** → New App: iOS, name *Gambler: Chance Sequencer*, bundle ID `com.tonegarden.Gambler`, SKU e.g. `gambler-ios`. Uploads fail until this exists.

### 6. Store the secrets in GitHub

From the repo, in Terminal:

```bash
./scripts/set-release-secrets.sh \
  --p12 ~/Downloads/AppleDistribution.p12 \
  --app-profile ~/Downloads/Gambler_App_Store.mobileprovision \
  --auv3-profile ~/Downloads/Gambler_AUv3_App_Store.mobileprovision \
  --asc-key ~/Downloads/AuthKey_XXXXXXXXXX.p8 --asc-issuer <issuer-id>
```

It asks for the `.p12` password and stores everything in the repo's **app-store** environment:

| Secret | What it is |
|---|---|
| `IOS_DIST_CERT_P12`, `IOS_DIST_CERT_PASSWORD` | the Apple Distribution certificate and its password |
| `IOS_APP_PROFILE`, `IOS_AUV3_PROFILE` | the two App Store provisioning profiles |
| `ASC_KEY_P8`, `ASC_KEY_ID`, `ASC_ISSUER_ID` | the App Store Connect API key |

Then delete the downloaded files, or keep them somewhere safe like a password manager.

## Security notes

- The workflow only runs on version tags and manual runs, never on pull requests, so code from forks never sees the secrets.
- Third-party actions are pinned to exact commit SHAs.
- The certificate lives in a throwaway keychain that is deleted at the end of the job, along with the profiles and the API key file.
- The API key has App Manager access only. It can upload builds and edit metadata but can't manage users, certificates or banking.
- Once the repo is public, consider adding a required reviewer to the **app-store** environment (Settings → Environments) so every release waits for your approval.

## Renewal (once a year)

The distribution certificate and both profiles expire after a year (the workflow log prints the expiry dates). Create a new certificate, regenerate both profiles with it, and run `./scripts/set-release-secrets.sh` again with the new files.
