#!/usr/bin/env bash
# Version an OS release. Runs ONLY inside the manual dist pipeline
# (CHANNEL=release) - pushes to main never bump on their own.
#
# What it does, in order:
#   1. if HEAD already carries an os-v* tag (rerun of a failed build),
#      reuse it - same tree, same version
#   2. pick the next version: OS_VERSION if set, else OS_BUMP
#      (major|minor|patch, default minor) on top of the newest os-v* tag
#   3. write the "## [os-vX]" changelog section from merged MRs, linking
#      the future GitLab release URL
#   4. commit that, let cog tag it (disable-bump-commit keeps the commit
#      ours; cog still writes its package changelog, which is discarded)
#   5. push commit + tag; github-dispatch then ships the bumped tree
#
# Env: OS_VERSION (exact, e.g. 0.3.0) wins over OS_BUMP.
set -euo pipefail

git fetch origin main "+refs/tags/*:refs/tags/*"
git checkout -B main origin/main
git clean -fd

TAG=$(git tag --points-at HEAD | grep -E '^os-v[0-9]' | head -1 || true)
if [ -n "$TAG" ]; then
  echo "HEAD already tagged $TAG - reusing (rerun)"
else
  LAST=$(git tag -l 'os-v[0-9]*' --sort=-version:refname | head -1 || true)
  if [ -n "${OS_VERSION:-}" ]; then
    V="${OS_VERSION#os-v}"; V="${V#v}"
  elif [ -z "$LAST" ]; then
    V="0.1.0"
  else
    V=$(python3 - "$LAST" "${OS_BUMP:-minor}" <<'EOF'
import re, sys
m = re.match(r"os-v(\d+)\.(\d+)\.(\d+)", sys.argv[1])
maj, mnr, pat = map(int, m.groups())
kind = sys.argv[2]
if kind == "major": print(f"{maj+1}.0.0")
elif kind == "patch": print(f"{maj}.{mnr}.{pat+1}")
elif kind == "minor": print(f"{maj}.{mnr+1}.0")
else: sys.exit(f"bad OS_BUMP {kind} - major|minor|patch")
EOF
)
  fi
  TAG="os-v$V"
  echo "bumping os: ${LAST:-none} -> $TAG"

  python3 system/dist/scripts/os_changelog.py release "$TAG"
  git add CHANGELOG.md
  git commit -m "docs: 发布 $TAG"

  # the commit above is conventional, so changelog generation inside the
  # bump finds commits and does not error; --disable-bump-commit leaves
  # it unstaged and we drop it - the MR changelog is the real one
  cog bump --package os --version "$V" --skip-untracked --disable-bump-commit
  git reset --hard -q HEAD

  [ "$(git tag --points-at HEAD | grep -cx "$TAG")" -eq 1 ] || {
    echo "cog did not tag HEAD with $TAG" >&2; exit 1; }

  # ci.skip keeps the bump push from starting a pipeline; remotes without
  # push options get a plain push instead
  push_commit() {
    git push -o ci.skip origin HEAD:main 2>/tmp/push.err && return 0
    grep -q "push options" /tmp/push.err && git push origin HEAD:main
  }
  if ! push_commit; then
    echo "main moved, rebasing"
    git fetch origin main "+refs/tags/*:refs/tags/*"
    git rebase origin/main
    git tag -f "$TAG"
    git push origin HEAD:main
  fi
  git push origin "$TAG"
fi

# dotenv artifact for github-dispatch: the release tag to pass to the
# GitHub workflow as the image version + release tag
echo "OS_TAG=$TAG" | tee os-release.env
