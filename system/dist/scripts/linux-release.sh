#!/usr/bin/env bash
# Publish the Linux lane's outputs as a GitLab release. With
# build_android=off the GitHub image lane never runs, so this is the
# release half: the mkimage tarballs, the static hsvc-vmd binaries and
# the host vmd ship as generic packages linked from the release, which
# never expire (unlike job artifacts).
#
# Tag scheme matches the GitHub lane: prealpha-v<date>-r<run> for
# debug builds, <channel>-v<date>-r<run> otherwise (release channel
# drops the prefix). prealpha tags are debug releases - the changelog
# is left alone and os_changelog.py does not count them as shipped.
# Anything else folds "Unreleased" into a section for the tag before
# releasing, same as sync-from-github.
set -euo pipefail

export PATH="$HOME/.local/bin:$PATH"
cd "${CI_PROJECT_DIR:-$PWD}"

PREFIX=""
case "${CHANNEL:-release}" in alpha|beta) PREFIX="$CHANNEL-" ;; esac
[ "${PREALPHA:-false}" = "true" ] && PREFIX="prealpha-"
TAG="${PREFIX}v$(date -u +%Y.%m.%d)-r${CI_PIPELINE_IID:-0}"
echo "Releasing Linux build as $TAG"

rm -rf release-assets SHA256SUMS.txt
mkdir -p release-assets
cp system/linux/out/images/hibiscus-linux-*.tar.xz release-assets/
cp system/linux/out/dist/hsvc-vmd-* release-assets/
cp vmd/out/dist/* release-assets/
(cd release-assets && for f in *; do
  [ -f "$f" ] || continue
  sha256sum "$f"
done > ../SHA256SUMS.txt)
cp SHA256SUMS.txt release-assets/
ls -la release-assets/

# reruns re-create the same tag - drop the leftovers first
glab release delete "$TAG" -R "$CI_PROJECT_PATH" -y 2>/dev/null || true
PKG_ID=$(glab api "projects/$CI_PROJECT_ID/packages?package_name=release-assets&package_version=$TAG" 2>/dev/null | jq -r '.[0].id // empty')
if [ -n "$PKG_ID" ] && [ "$PKG_ID" != "null" ]; then
  glab api --method DELETE "projects/$CI_PROJECT_ID/packages/$PKG_ID" 2>/dev/null || true
fi

git fetch -q origin main "+refs/tags/*:refs/tags/*"
case "$TAG" in
  prealpha-*)
    echo "prealpha build - changelog untouched" ;;
  *)
    if git show FETCH_HEAD:CHANGELOG.md 2>/dev/null | grep -qF "[$TAG]"; then
      echo "changelog already has a $TAG section"
    else
      git checkout -B main FETCH_HEAD
      python3 system/dist/scripts/os_changelog.py release "$TAG"
      git add CHANGELOG.md
      [ -d changelogs ] && git add -A changelogs/
      if git diff --cached --quiet; then
        echo "changelog unchanged"
      else
        git commit -m "docs: 标记 $TAG changelog"
        if ! git push -o ci.skip origin HEAD:main 2>/dev/null; then
          git fetch -q origin main
          git rebase origin/main
          python3 system/dist/scripts/os_changelog.py release "$TAG"
          git add CHANGELOG.md
          [ -d changelogs ] && git add -A changelogs/
          git commit --amend --no-edit
          git push origin HEAD:main
        fi
      fi
    fi
    ;;
esac

NOTES="Hibiscus Linux build $TAG."
REF_SHA=$(git rev-parse FETCH_HEAD 2>/dev/null || echo "$CI_COMMIT_SHA")
SECTION=$(git show FETCH_HEAD:CHANGELOG.md 2>/dev/null | awk -v t="$TAG" '
  /^## / { if (f) exit; if (index($0, t)) f=1 }
  f')
[ -n "$SECTION" ] && NOTES="$NOTES

$SECTION"
echo "$NOTES" > notes.md
{
  echo
  for f in release-assets/*; do
    n=$(basename "$f")
    case "$n" in
      hibiscus-linux-*.tar.xz) d="distro staging tree - userspace, units and the generic device profile; untar over any systemd rootfs" ;;
      hsvc-vmd-*)              d="static single-file build: a mouse is a 3-dof headset, click is the trigger" ;;
      vmd-linux-*)             d="qemu host driver with the Hibiscus Linux guest embedded - boots the OS in a VM, no other files needed" ;;
      vmd-guest-*.tar.xz)      d="guest image set for 'vmd -img' - untar and point -img at the dir" ;;
      SHA256SUMS.txt)          d="checksums" ;;
      *)                       d="" ;;
    esac
    echo "- \`$n\` $d"
  done
} >> notes.md

# Uploads go to the package registry before the release exists - glab
# release create would open the release first and upload one file at a
# time, leaving the page listing files that are not linked yet
PKG_PATH="$CI_SERVER_URL/api/v4/projects/${CI_PROJECT_PATH//\//%2F}/packages/generic/release-assets"
TAG_ENC=${TAG//./%2E}
LINKS="[]"
for f in release-assets/*; do
  n=$(basename "$f")
  n_enc=${n//./%2E}
  curl -fsS --retry 3 --header "JOB-TOKEN: $CI_JOB_TOKEN" --upload-file "$f" \
    "$CI_API_V4_URL/projects/$CI_PROJECT_ID/packages/generic/release-assets/$TAG_ENC/$n_enc" >/dev/null
  LINKS=$(jq -c --arg n "$n" --arg u "$PKG_PATH/$TAG_ENC/$n_enc" \
    '. + [{name: $n, url: $u, link_type: "other"}]' <<<"$LINKS")
done

glab release create "$TAG" \
  --name "Hibiscus Linux $TAG" \
  --notes-file notes.md \
  --ref "$REF_SHA" \
  --assets-links "$LINKS"
