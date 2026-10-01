#!/usr/bin/env bash
# Copy the hbsup GitHub release into a GitLab release of the same tag, so
# the binaries never expire. Companion to the dist sync script.
set -euo pipefail

export PATH="$HOME/.local/bin:$PATH"
cd "${CI_PROJECT_DIR:-$PWD}"

REPO="justacalico/HibiscusXR"
RELEASE_TAG="${RELEASE_TAG:?usage: RELEASE_TAG=hbsup-vX.Y.Z}"
echo "Syncing GitHub release $RELEASE_TAG -> $CI_PROJECT_PATH"

rm -rf hbsup-assets SHA256SUMS.txt
mkdir -p hbsup-assets
gh release download "$RELEASE_TAG" -R "$REPO" --dir hbsup-assets

if [ -n "${GITHUB_RUN_ID:-}" ]; then
  gh run view "$GITHUB_RUN_ID" -R "$REPO" --log > hbsup-assets/github-logs.txt 2>/dev/null || true
fi

(cd hbsup-assets && sha256sum * > ../SHA256SUMS.txt)
cp SHA256SUMS.txt hbsup-assets/
ls -la hbsup-assets/

glab release delete "$RELEASE_TAG" -R "$CI_PROJECT_PATH" -y 2>/dev/null || true
PKG_ID=$(glab api "projects/$CI_PROJECT_ID/packages?package_name=hbsup-assets&package_version=$RELEASE_TAG" 2>/dev/null | jq -r '.[0].id // empty')
if [ -n "$PKG_ID" ] && [ "$PKG_ID" != "null" ]; then
  glab api --method DELETE "projects/$CI_PROJECT_ID/packages/$PKG_ID" || true
fi

glab release create "$RELEASE_TAG" \
  --name "HBSUP $RELEASE_TAG" \
  --notes "Hibiscus Backup desktop builds, mirrored from the GitHub release." \
  --ref "$CI_COMMIT_SHA" \
  --use-package-registry \
  "$PWD/hbsup-assets"/*
