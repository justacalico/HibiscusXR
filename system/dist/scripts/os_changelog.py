#!/usr/bin/env python3
"""OS changelog generator - entries are merge requests, not commits.

cocogitto builds changelogs from conventional commits, which cannot work
here: commits in this repo are short Chinese one-liners, and one MR can
carry twenty of them. What the user reads on a release is the MR list, so
this script asks the GitLab API for MRs merged since the last release tag
and writes one linked line per MR.

An MR only counts for the OS changelog when it touches at least one path
outside applications/desktop/ and websites/ - those have their own release
lanes and changelogs.

Usage:
  os_changelog.py unreleased        refresh the "## Unreleased" block only
  os_changelog.py release <tag>     turn Unreleased into a named "## [tag]" section

The base point is the newest os-v* tag (commit date). With none, the newest
date-scheme image tag (alpha-v*/beta-v*/vYYYY.*-rN) is used so the first
cog release does not dump the repo's whole MR history into one section.
"""
import json
import os
import re
import subprocess
import sys
import urllib.parse
from datetime import datetime, timezone

# prefixes whose MRs get their own changelogs - an MR touching only these
# paths never lands in the OS changelog
NON_OS_PREFIXES = ("applications/desktop/", "websites/")
FILE = os.environ.get("CHANGELOG_FILE", "CHANGELOG.md")
GLAB = os.environ.get("GLAB", "glab")


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


PROJ = project_path()
PROJ_API = urllib.parse.quote(PROJ, safe="")
WEB = "https://gitlab.com/" + PROJ


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


def mr_paths(iid):
    try:
        return [d["new_path"] for d in api_paginated(
            f"projects/{PROJ_API}/merge_requests/{iid}/diffs")]
    except Exception:
        return None  # on API trouble keep the MR - noisy beats silent


def merged_mrs():
    qs = "state=merged&order_by=updated_at&sort=asc"
    base = base_date()
    if base:
        qs += "&updated_after=" + urllib.parse.quote(base)
    mrs = api_paginated(f"projects/{PROJ_API}/merge_requests?{qs}")
    if base:
        mrs = [m for m in mrs
               if m.get("merged_at") and ts(m["merged_at"]) > ts(base)]
    out = []
    for m in mrs:
        paths = mr_paths(m["iid"])
        if paths is not None and all(
                p.startswith(NON_OS_PREFIXES) for p in paths):
            continue
        out.append(m)
    return out


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


def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else "unreleased"
    text = open(FILE).read() if os.path.exists(FILE) else ""
    preamble, secs = parse(text) if text else ("# Changelog\n\n"
        "Hibiscus OS image releases. Entries are merge requests, one\n"
        "linked line per MR no matter how many commits it carried.\n", [])
    linkdefs = linkdefs_of(text)
    # drop linkdef lines that landed inside a parsed section body
    secs = [(t, "\n".join(l for l in b.splitlines()
                          if not re.match(r"^\[os-v[^\]]*\]: ", l)))
            for t, b in secs]

    if mode == "unreleased":
        body = "\n".join(entry(m) for m in merged_mrs())
        secs = [(t, b) for t, b in secs if t != "Unreleased"]
        secs.insert(0, ("Unreleased", body))
    elif mode == "release":
        tag = sys.argv[2]
        body = "\n".join(entry(m) for m in merged_mrs())
        secs = [(t, b) for t, b in secs if t != "Unreleased"]
        date = datetime.now(timezone.utc).strftime("%Y-%m-%d")
        secs.insert(0, ("Unreleased", ""))
        secs.insert(1, (f"[{tag}] - {date}", body))
        linkdefs.append(f"[{tag}]: {WEB}/-/releases/{tag}")
    else:
        sys.exit("usage: os_changelog.py [unreleased|release <tag>]")

    open(FILE, "w").write(render(preamble, secs, set(linkdefs)))


if __name__ == "__main__":
    main()
