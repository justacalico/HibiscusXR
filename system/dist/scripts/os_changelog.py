#!/usr/bin/env python3
"""OS changelog generator - entries are merge requests, not commits.

cocogitto builds changelogs from conventional commits, which cannot work
here: commits in this repo are short Chinese one-liners, and one MR can
carry twenty of them. What the user reads on a release is the MR list, so
this script asks the GitLab API for MRs merged since the last release tag
and writes one linked line per MR.

Every merged MR counts, no matter which folder it touched.

Two extra inputs fold into a section besides the API list:
  - fragment files under changelogs/unreleased/<iid>.md, committed onto
    MR branches by the os-changelog-mr job when an MR pipeline opens.
    They act as a fallback when the API cannot describe a merged MR.
  - manual lines: any Unreleased line that is not a generated MR entry
    survives regeneration, so hand-written notes ride into the release.

Usage:
  os_changelog.py unreleased        refresh the "## Unreleased" block only
  os_changelog.py release <tag>     turn Unreleased into a named "## [tag]" section
  os_changelog.py record            write the fragment for the current
                                    merge_request_event pipeline's MR

The base point is the newest os-v* tag (commit date). With none, the newest
date-scheme image tag (alpha-v*/beta-v*/vYYYY.*-rN) is used so the first
release does not dump the repo's whole MR history into one section.
"""
import json
import os
import re
import subprocess
import sys
import urllib.parse
from datetime import datetime, timezone

FILE = os.environ.get("CHANGELOG_FILE", "CHANGELOG.md")
FRAG_DIR = os.environ.get("CHANGELOG_FRAG_DIR", "changelogs/unreleased")
GLAB = os.environ.get("GLAB", "glab")
GENERATED_RE = re.compile(r"^- \[.*\]\(\S*/-/merge_requests/\d+\) \(!\d+\)\s*$")


def sh(*args):
    return subprocess.run(args, capture_output=True, text=True, check=True).stdout


def project_path():
    if os.environ.get("CI_PROJECT_PATH"):
        return os.environ["CI_PROJECT_PATH"]
    url = sh("git", "remote", "get-url", "origin").strip()
    m = re.search(r"gitlab\.com[:/](?P<p>.+?)(?:\.git)?$", url)
    if not m:
        sys.exit("cannot derive project path from origin: " + url)
    return m.group("p")


def api(path):
    out = sh(GLAB, "api", path)
    return json.loads(out) if out.strip() else []


def api_paginated(path):
    items, page = [], 1
    while True:
        got = api(f"{path}{'&' if '?' in path else '?'}per_page=100&page={page}")
        items += got
        if len(got) < 100:
            return items
        page += 1


def last_release_tag():
    tags = sh("git", "tag", "-l", "os-v*", "--sort=-version:refname").split()
    if tags:
        return tags[0]
    # pre-cog date scheme: alpha-v2026.10.03-r10 / beta-v* / v2026.*-r*
    tags = sh("git", "for-each-ref", "--sort=-creatordate",
              "--format=%(refname:short)", "refs/tags").split()
    for t in tags:
        if re.match(r"^(alpha-|beta-)?v\d{4}\.\d{2}\.\d{2}-r\d+$", t):
            return t
    return None


def base_date():
    tag = last_release_tag()
    if not tag:
        return None
    return sh("git", "log", "-1", "--format=%cI", tag).strip()


def ts(iso):
    return datetime.fromisoformat(iso.replace("Z", "+00:00")).timestamp()


def merged_mrs(proj_api):
    qs = "state=merged&order_by=updated_at&sort=asc"
    base = base_date()
    if base:
        qs += "&updated_after=" + urllib.parse.quote(base)
    mrs = api_paginated(f"projects/{proj_api}/merge_requests?{qs}")
    if base:
        mrs = [m for m in mrs
               if m.get("merged_at") and ts(m["merged_at"]) > ts(base)]
    return mrs


def fragments():
    """{iid: stored line} from changelogs/unreleased/<iid>.md files"""
    out = {}
    if os.path.isdir(FRAG_DIR):
        for name in sorted(os.listdir(FRAG_DIR)):
            m = re.match(r"^(\d+)\.md$", name)
            if m:
                line = open(os.path.join(FRAG_DIR, name)).read().strip()
                out[int(m.group(1))] = line
    return out


def section_entries(proj_api):
    """merged MRs from the API union fragment files, deduped by iid"""
    mrs = merged_mrs(proj_api)
    entries = [entry(m) for m in mrs]
    seen = {m["iid"] for m in mrs}
    for iid, line in fragments().items():
        if iid in seen:
            continue
        try:
            m = api(f"projects/{proj_api}/merge_requests/{iid}")
            entries.append(entry(m))
        except Exception:
            entries.append(line)
    return entries


def entry(m):
    return f"- [{m['title']}]({m['web_url']}) (!{m['iid']})"


def parse(text):
    """split into (preamble, [(title, body)]) on ## boundaries"""
    parts = re.split(r"(?m)^## ", text)
    preamble = parts[0]
    secs = []
    for p in parts[1:]:
        title, _, body = p.partition("\n")
        secs.append((title, body))
    return preamble, secs


def render(preamble, secs, linkdefs):
    out = preamble.rstrip() + "\n"
    for title, body in secs:
        out += f"\n## {title}\n{body.rstrip()}\n"
    if linkdefs:
        out += "\n" + "\n".join(sorted(linkdefs)) + "\n"
    return out


def linkdefs_of(text):
    return [l for l in text.splitlines() if re.match(r"^\[os-v[^\]]*\]: ", l)]


def manual_lines(body):
    """lines a human wrote - generated entries get rebuilt from the API,
    everything else carries over so notes survive regeneration"""
    return [l for l in body.splitlines()
            if l.strip() and not GENERATED_RE.match(l.strip())]


def record(proj_api):
    """write the changelog fragment for the MR behind a
    merge_request_event pipeline. Nothing to commit happens here - the
    caller inspects the worktree."""
    if os.environ.get("CI_MERGE_REQUEST_SOURCE_PROJECT_ID") != \
            os.environ.get("CI_PROJECT_ID"):
        print("fork MR - merge-time regeneration covers it")
        return
    iid = int(os.environ["CI_MERGE_REQUEST_IID"])
    title = os.environ["CI_MERGE_REQUEST_TITLE"]
    base = os.environ.get("CI_MERGE_REQUEST_PROJECT_URL", "").rstrip("/")
    if not base:
        base = "https://gitlab.com/" + proj_api.replace("%2F", "/")
    line = f"- [{title}]({base}/-/merge_requests/{iid}) (!{iid})"
    path = os.path.join(FRAG_DIR, f"{iid}.md")
    if os.path.exists(path) and open(path).read().strip() == line:
        print("fragment already up to date")
        return
    os.makedirs(FRAG_DIR, exist_ok=True)
    open(path, "w").write(line + "\n")
    print("wrote " + path)


def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else "unreleased"
    proj_api = urllib.parse.quote(project_path(), safe="")

    if mode == "record":
        record(proj_api)
        return

    text = open(FILE).read() if os.path.exists(FILE) else ""
    preamble, secs = parse(text) if text else (
        "# Changelog\n\nHibiscus releases. Entries are merge requests, one\n"
        "linked line per MR no matter how many commits it carried.\n", [])
    linkdefs = linkdefs_of(text)
    # drop linkdef lines that landed inside a parsed section body
    secs = [(t, "\n".join(l for l in b.splitlines()
                          if not re.match(r"^\[os-v[^\]]*\]: ", l)))
            for t, b in secs]

    unrel = next((b for t, b in secs if t == "Unreleased"), "")
    body = "\n".join(section_entries(proj_api) + manual_lines(unrel))
    secs = [(t, b) for t, b in secs if t != "Unreleased"]

    if mode == "unreleased":
        secs.insert(0, ("Unreleased", body))
    elif mode == "release":
        tag = sys.argv[2]
        date = datetime.now(timezone.utc).strftime("%Y-%m-%d")
        secs.insert(0, ("Unreleased", ""))
        secs.insert(1, (f"[{tag}] - {date}", body))
        web = "https://gitlab.com/" + proj_api.replace("%2F", "/")
        linkdefs.append(f"[{tag}]: {web}/-/releases/{tag}")
        # released fragments are folded into the section - drop the files
        for iid in fragments():
            os.remove(os.path.join(FRAG_DIR, f"{iid}.md"))
    else:
        sys.exit("usage: os_changelog.py [unreleased|release <tag>|record]")

    open(FILE, "w").write(render(preamble, secs, set(linkdefs)))


if __name__ == "__main__":
    main()
