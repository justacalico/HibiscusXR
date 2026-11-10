#!/usr/bin/env bash
# guest-user.sh <root> - the staged guest rootfs's login story:
# the hibiscus account baked into passwd/group/shadow by the same
# mkuser the boot-time hibiscus-user.service runs, plus a console
# getty that drops straight into it instead of a login prompt - the
# shell takes the display when it is staged, and a console that still
# comes up lands in a usable session.
set -euo pipefail
root="${1:-}"
[ -n "$root" ] || { echo "usage: guest-user.sh <root>" >&2; exit 2; }
here=$(cd "$(dirname "$0")" && pwd)
sh "$here/../../system/linux/users/mkuser" "$root"
mkdir -p "$root/etc/systemd/system/getty@.service.d"
cat > "$root/etc/systemd/system/getty@.service.d/autologin.conf" <<'EOF'
[Service]
ExecStart=
ExecStart=-/usr/sbin/agetty --autologin hibiscus --noclear %I $TERM
EOF
