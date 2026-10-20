#!/usr/bin/env bash
# Commit a changelog fragment onto an MR's source branch when its
# pipeline opens: changelogs/unreleased/<iid>.md rides along in the diff,
# lands on main with the merge, and gets folded into CHANGELOG.md by
# os-changelog (push to main) and os-bump (release).
#
# Pushes with the CI job token - the project has
# ci_push_repository_for_job_token_allowed enabled, so no secrets are
# needed in MR pipelines. Job-token pushes never trigger pipelines, so
# the push cannot loop; on rerun the fragment is already current anyway.
# Fork MRs cannot be pushed to; the merge-time regeneration on main
# writes their entry instead.
set -euo pipefail

if [ "${CI_MERGE_REQUEST_SOURCE_PROJECT_ID:-}" != "${CI_PROJECT_ID:-}" ]; then
  echo "fork MR - merge-time changelog regen covers it"
  exit 0
fi

SRC="${CI_MERGE_REQUEST_SOURCE_BRANCH_NAME}"

git fetch origin "$SRC"
git checkout -B mr-changelog FETCH_HEAD

python3 system/dist/scripts/os_changelog.py record

if ! git status --porcelain changelogs/ | grep -q .; then
  echo "changelog fragment already current"
  exit 0
fi

git add changelogs/
git commit -m "docs: !${CI_MERGE_REQUEST_IID} changelog 片段"

push() {
  git push origin "HEAD:$SRC"
}
if ! push; then
  echo "source branch moved, rebasing once"
  git fetch origin "$SRC"
  git rebase FETCH_HEAD
  if ! push; then
    echo "::warning:: could not push fragment - merge-time regen covers it"
  fi
fi
