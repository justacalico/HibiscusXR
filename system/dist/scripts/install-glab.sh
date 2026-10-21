#!/usr/bin/env bash
set -euo pipefail

if command -v glab >/dev/null 2>&1; then
  glab --version
  exit 0
fi

version="${GLAB_VERSION:-1.113.0}"
arch="${CI_RUNNER_ARCH:-$(uname -m)}"
case "$arch" in
  arm|armv7l) arch="armv6" ;;
  aarch64|arm64) arch="arm64" ;;
  386|i386) arch="386" ;;
  x86_64|amd64) arch="amd64" ;;
esac

sha256="${GLAB_SHA256:-}"
if [ -z "$sha256" ] && [ "$version" = "1.113.0" ]; then
  case "$arch" in
    386) sha256="ac6d5f792fff41eb23e20dcca944f26652640954cd513991811187269423dd68" ;;
    amd64) sha256="c265589fb2018f310b6a27df9356efee42991f858e7e3a5fa232228a13b47467" ;;
    arm64) sha256="0c4cab3b35f7641e7955492b9491d7070698a958f0dfcb49d989e71bd78793c2" ;;
    armv6) sha256="408ac7c44f361cd8afe6f09a28e00366c97f5c8c70b00e82f1236a6f9c79af89" ;;
    ppc64le) sha256="5667723a88cb1a5deefad9852992a52dce56c1fb6208d283093e863bd73ecd7c" ;;
    s390x) sha256="fd89e3ce5670eabec59af3115ec3549ed7202975f913d1a7a3537446b9c78584" ;;
  esac
fi
if [ -z "$sha256" ]; then
  echo "no pinned sha256 for glab ${version} linux_${arch}, set GLAB_SHA256" >&2
  exit 1
fi

url="https://gitlab.com/gitlab-org/cli/-/releases/v${version}/downloads/glab_${version}_linux_${arch}.tar.gz"
mkdir -p /usr/local/bin
curl -fsSL "$url" -o /tmp/glab.tar.gz
echo "${sha256}  /tmp/glab.tar.gz" | sha256sum -c -
tar -xzf /tmp/glab.tar.gz -C /tmp --strip-components=1
cp /tmp/glab /usr/local/bin/glab
chmod +x /usr/local/bin/glab
glab --version
