# Releasing

Maintainer notes. The app updates itself from GitHub releases, so a release must carry the signed update files or the clients can only show a link.

## Steps

1. Bump `kAppVersion` in `native/src/app_version.hpp` and the four version lines in `native/assets/app.rc`. Update `CHANGELOG.md` and run `python docs/assets/gen_badges.py`.
2. Build and test: `native\build.ps1`, then `native\tests\build_tests.ps1` (check the exit code).
3. Sign the build:

   ```powershell
   powershell -File native\sign_update.ps1 -Version X.Y.Z -OutDir C:\path\to\release
   ```

   It writes `Lettuce_Live_Editor.exe`, `update_manifest.txt` and `update_manifest.sig`. The script refuses to run if the executable is not version X.Y.Z, or if the public key compiled into the app (`kUpdatePublicKey` in `native/src/engine/self_update.hpp`) does not belong to the signing key, because every client would reject that update.
4. Build the zip with Python `zipfile` (not `Compress-Archive`) holding `Lettuce Live Editor.exe` and `LICENSE.txt`, named `PES2021_Lettuce_Live_Editor_vX.Y.Z.zip`.
5. Merge the pull request, then publish a **normal release** (not a pre-release) tagged `vX.Y.Z` on the merge commit, with these four assets: the zip and the three signed files. Put the SHA-256 of the zip in the notes.
6. Check it: `/releases/latest` must return the new tag, and the assets must download with the hashes from the manifest.

## Why the rules

- The updater reads `releases/latest`, which skips pre-releases and drafts. A pre-release is never offered to anyone.
- The signing key is the root of trust. It lives outside the repository at `%USERPROFILE%\.pes21-var-mod\update_signing_key.txt` (shared with the VAR mod, same maintainer). Back it up: without it no later version can be installed automatically, and clients would need one manual update to a build with a new public key.
- A release without the three files is not broken, the app just shows the notice with the release link.
- Versions up to 14.1.1 do not have the updater, so they update by hand once.
