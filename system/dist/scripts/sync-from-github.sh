#!/usr/bin/env bash
# Copy the newest GitHub release into a GitLab release of the same tag.
# --use-package-registry stores the files as generic packages linked from the
# release, which never expire (unlike job artifacts).
set -euo pipefail

export PATH="$HOME/.local/bin:$PATH"
cd "${CI_PROJECT_DIR:-$PWD}"

REPO="justacalico/HibiscusXR"
# gh release view defaults to the "latest" (non-prerelease) release, which
# skips alpha/beta tags.  list gets every release newest-first instead.
RELEASE_TAG="${RELEASE_TAG:-$(gh release list -R "$REPO" --limit 1 --json tagName --jq '.[0].tagName')}"
echo "Syncing GitHub release $RELEASE_TAG -> $CI_PROJECT_PATH"

rm -rf release-assets SHA256SUMS.txt
mkdir -p release-assets
gh release download "$RELEASE_TAG" -R "$REPO" --dir release-assets

if [ -n "${GITHUB_RUN_ID:-}" ]; then
  gh run view "$GITHUB_RUN_ID" -R "$REPO" --log > release-assets/github-logs.txt 2>/dev/null || true
fi

(cd release-assets && sha256sum * > ../SHA256SUMS.txt)
cp SHA256SUMS.txt release-assets/
ls -la release-assets/

# same-named leftovers would collide with the package upload
glab release delete "$RELEASE_TAG" -R "$CI_PROJECT_PATH" -y 2>/dev/null || true
PKG_ID=$(glab api "projects/$CI_PROJECT_ID/packages?package_name=release-assets&package_version=$RELEASE_TAG" 2>/dev/null | jq -r '.[0].id // empty')
if [ -n "$PKG_ID" ] && [ "$PKG_ID" != "null" ]; then
  glab api --method DELETE "projects/$CI_PROJECT_ID/packages/$PKG_ID" 2>/dev/null || true
fi

# pull the matching changelog section (MR titles, one per MR) into the
# release notes - for os-v* tags the section lives on main after os-bump
NOTES="Mirrored from the GitHub release."
git fetch -q origin main 2>/dev/null || true
REF_SHA=$(git rev-parse FETCH_HEAD 2>/dev/null || echo "$CI_COMMIT_SHA")
SECTION=$(git show FETCH_HEAD:CHANGELOG.md 2>/dev/null | awk -v t="$RELEASE_TAG" '
  /^## / { if (f) exit; if (index($0, t)) f=1 }
  f')
[ -n "$SECTION" ] && NOTES="$NOTES

$SECTION"
echo "$NOTES" > notes.md

glab release create "$RELEASE_TAG" \
  --name "Hibiscus system images $RELEASE_TAG" \
  --notes-file notes.md \
  --ref "$REF_SHA" \
  --use-package-registry \
  "$PWD/release-assets"/*
