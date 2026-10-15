#!/usr/bin/env bash
# Builds and installs packaging/arch/PKGBUILD in a clean Arch Linux container
# from the committed HEAD of this checkout, then smoke-tests the result.
#
#   scripts/test-pkg.sh            # docker, or podman if docker is missing
#
# Needs network access (base image and packages). Nothing is installed on the
# host.
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
engine="$(command -v docker || command -v podman || true)"
[[ -n "$engine" ]] || { echo "test-pkg: needs docker or podman" >&2; exit 1; }

if [[ -n "$(git -C "$root" status --porcelain)" ]]; then
    echo "test-pkg: note: uncommitted changes are not part of the test (builds HEAD)" >&2
fi

exec "$engine" run --rm -v "$root:/src:ro" archlinux:latest bash -euo pipefail -c '
    pacman -Syu --noconfirm --needed base-devel git >/dev/null
    useradd -m builder
    echo "builder ALL=(ALL) NOPASSWD: ALL" >/etc/sudoers.d/builder
    git config --system --add safe.directory "*"
    cp /src/packaging/arch/PKGBUILD /home/builder/
    chown builder: /home/builder/PKGBUILD
    cd /home/builder
    sudo -u builder ISLAND_SHELL_SOURCE=/src makepkg -si --noconfirm --syncdeps

    echo "== installed files"
    pacman -Ql island-shell-git | grep -vE "/$" | sed "s/^island-shell-git //"
    echo "== island-shell --version"
    island-shell --version
    echo "== islandctl --help (first lines)"
    islandctl --help | head -3
    echo "== offscreen smoke test"
    set +e
    QT_QPA_PLATFORM=offscreen timeout 3 island-shell
    status=$?
    set -e
    [[ $status -eq 124 ]] && echo "smoke: ok (still running at timeout)" || { echo "smoke: exit $status"; exit 1; }
    bash -n /usr/bin/island-shell-run && echo "island-shell-run: syntax ok"
'
