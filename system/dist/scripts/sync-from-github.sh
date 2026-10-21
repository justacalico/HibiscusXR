#!/usr/bin/env bash
# Copy the newest GitHub release into a GitLab release of the same tag.
# --use-package-registry stores the files as generic packages linked from the
# release, which never expire (unlike job artifacts).
set -euo pipefail

gh auth status >/dev/null 2>&1 || { echo "gh not authenticated - set GH_TOKEN or run gh auth login" >&2; exit 1; }

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

(cd release-assets && for f in *; do
  [ -f "$f" ] || continue
  sha256sum "$f"
done > ../SHA256SUMS.txt)
cp SHA256SUMS.txt release-assets/
ls -la release-assets/

# same-named leftovers would collide with the package upload
glab release delete "$RELEASE_TAG" -R "$CI_PROJECT_PATH" -y 2>/dev/null || true
PKG_ID=$(glab api "projects/$CI_PROJECT_ID/packages?package_name=release-assets&package_version=$RELEASE_TAG" 2>/dev/null | jq -r '.[0].id // empty')
if [ -n "$PKG_ID" ] && [ "$PKG_ID" != "null" ]; then
  glab api --method DELETE "projects/$CI_PROJECT_ID/packages/$PKG_ID" 2>/dev/null || true
fi

# mark what shipped: fold "Unreleased" into a section named for this tag,
# so the changelog reads per-build and the notes lookup below finds it.
# os-v* tags already carry their section - os-bump writes it pre-tag. Runs
# before `glab release create` so the GitLab tag lands on the changelog
# commit instead of dangling one commit short.
if git show FETCH_HEAD:CHANGELOG.md 2>/dev/null | grep -qF "[$RELEASE_TAG]"; then
  echo "changelog already has a $RELEASE_TAG section"
else
  git fetch -q origin main "+refs/tags/*:refs/tags/*"
  git checkout -B main FETCH_HEAD
  python3 system/dist/scripts/os_changelog.py release "$RELEASE_TAG"
  git add CHANGELOG.md
  [ -d changelogs ] && git add -A changelogs/
  if git diff --cached --quiet; then
    echo "changelog unchanged"
  else
    git commit -m "docs: 标记 $RELEASE_TAG changelog"
    if ! git push -o ci.skip origin HEAD:main 2>/dev/null; then
      git fetch -q origin main
      git rebase origin/main
      python3 system/dist/scripts/os_changelog.py release "$RELEASE_TAG"
      git add CHANGELOG.md
      [ -d changelogs ] && git add -A changelogs/
      git commit --amend --no-edit
      git push origin HEAD:main
    fi
  fi
fi

# pull the matching changelog section (MR titles, one per MR) into the
# release notes
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
