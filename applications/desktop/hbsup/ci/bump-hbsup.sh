#!/bin/bash
# HBSUP release bump: cog decides whether commits under applications/desktop/hbsup
# warrant a version bump, writes the changelog + version commit + tag,
# which we push back to main. Same model as the cte lane: the bump commit
# goes up with ci.skip, the tag push starts the tag pipeline that builds
# the release on GitHub.
set -euo pipefail

git fetch origin main "+refs/tags/*:refs/tags/*"
git checkout -B main origin/main
git clean -fd

if ! version=$(cog bump --package hbsup --auto --dry-run 2>/dev/null) || [ -z "$version" ]; then
  echo "No hbsup version bump required, skipping"
  exit 0
fi
echo "Bumping hbsup to $version"

cog bump --package hbsup --auto

TAG=$(git tag --points-at HEAD | grep '^hbsup-v' | head -n1)
if [ -z "$TAG" ]; then
  echo "cog bump did not create a hbsup tag" >&2
  exit 1
fi

push_bump() {
  git push -o ci.skip origin HEAD:main
}

if ! push_bump; then
  echo "Main moved while bumping, rebasing onto latest origin/main"
  git fetch origin main "+refs/tags/*:refs/tags/*"
  git rebase origin/main
  git tag -f "$TAG"
  push_bump
fi

git push origin "$TAG"
echo "Released $TAG"
