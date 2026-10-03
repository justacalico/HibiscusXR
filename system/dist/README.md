# dist

Build pipeline for the Hibiscus system images.

GitLab is the source of truth. Pushes mirror the monorepo to
[justacalico/HibiscusXR](https://github.com/justacalico/HibiscusXR), and a
manual pipeline dispatches the real build to GitHub Actions - the shared
runners here don't have the disk or the Android SDK the build needs. When the
GitHub run finishes, the release assets are copied back to a GitLab release
through the package registry, so they never expire.

## What a build does

1. `scripts/layout-sources.sh` copies the monorepo source dirs into
   `$PN2_ROOT` (`system/{tools,overlay,shim,hsvr,cted}`, `drivers/`,
   `controllers/`, `applications/{vrhome,quick-panel,settings,
   store,keyboard}`) - the sources are the monorepo checkout itself, nothing
   gets cloned.
2. `scripts/fetch-inputs.sh` downloads the pinned input packages from the
   private `neosalsa/dist-inputs` package registry (the proprietary blobs -
   kept private, see `overlay/PROPRIETARY-PVR.md` in the overlay repo).
3. `scripts/build-image.sh` runs the real `tools/build` chain on the runner:
   GSI xz -> simg2img -> build.prop -> overlay -> staged Pico stack ->
   fixes -> verify.
4. Outputs land as xz'd sparse `system-hibiscus.img.xz` / `system-hibiscus-full.img.xz`
   on a GitHub release, then on a same-named GitLab release via the
   `github-release-sync` job. Release assets are package-registry backed, so
   they never expire and download without a login.

## Running a build

Run a pipeline on `main` (web UI "Run pipeline", or `glab ci run`). The
`github-dispatch` job triggers the workflow and streams the GitHub log into
the job trace; `github-release-sync` publishes the assets when it succeeds.

## OS releases and versioning

The OS versions with cocogitto as the `os` monorepo package - tags look
like `os-v0.2.0`. Two hard rules:

- **Nothing versions automatically.** A push to main never bumps and never
  builds images. The only unattended job is `os-changelog`, which refreshes
  the `## Unreleased` block of `CHANGELOG.md` on each push.
- **A manual `CHANNEL=release` pipeline is the release.** Its `os-bump` job
  writes a changelog section named for the new tag (entries are merge
  request titles, one linked line per MR - never commits), commits it, cuts
  the `os-v*` tag, and pushes both back. The build then stamps that tag
  into the image, the GitHub release takes the same tag, and the GitLab
  release embeds the changelog section.

Manual pipelines on the default `alpha` (or `beta`) channel skip `os-bump`
entirely - test builds keep the `alpha-v<date>-r<run>` tag and leave the
changelog alone.

Overrides for `os-bump` (set as pipeline variables):

- `OS_VERSION=0.3.0` - exact version, beats `OS_BUMP`
- `OS_BUMP=major|minor|patch` - increment kind, default `minor`

`os-bump` is rerun-safe: when the pipeline is retried after a failed build
and main has not moved, the existing `os-v*` tag on HEAD is reused instead
of burning another version.

## Running locally

The same `build-image.sh` runs without CI. Point `PN2_ROOT` at a build root
and lay out the sources exactly like the workflow does - one script, one
list, so a local image ships the same content a GitHub run would:

```
export PN2_ROOT=/path/to/build-root
bash system/dist/scripts/layout-sources.sh
```

- supply the input packages - either run `fetch-inputs.sh` (needs the
  deploy-token secrets) or link the extracted dirs from an existing
  `~/PN2Lineage` workspace (`pvr_stack`, `pvr_apps_final`, `pvr_applibs`,
  `oem_final`, `overlay_pvr`, `airsvc`, `rfsa`, `qvr`, `cdsp`, `fan`,
  `seethrough`, `linklibs`, `build`, `overlay/lib64`,
  `notes/{libart-patched.so,vrshell_lib/}` and the GSI xz under `gsi/`).
  `libGLESv2_adreno.so` can also come from `images/vendor.img`
  (`/lib64/egl/`) - `debugfs -R "dump ..."` works offline.
- run `bash system/dist/scripts/build-image.sh`

The script's preflight step prints every missing input at once before doing
any work, so an empty root just tells you the whole list.

## Updating inputs

From the workspace root:

```
./scripts/upload-inputs.sh <new-version>
```

then bump the matching `PIN_*` in `manifest.env` and push. Every input is a
separate package so unchanged pieces are not re-uploaded.

## Flashing

The release images are xz'd Android sparse images - decompress, then flash:

```
unxz system-hibiscus-full.img.xz
fastboot oem pico unlock
fastboot -S 128M flash system system-hibiscus-full.img
```
