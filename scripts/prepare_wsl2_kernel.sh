#!/usr/bin/env bash
set -euo pipefail

KERNEL_RELEASE="$(uname -r)"
KERNEL_SERIES="$(printf '%s\n' "$KERNEL_RELEASE" | sed -E 's/^([0-9]+)\.([0-9]+).*/\1.\2/')"
WSL_BRANCH="${WSL_BRANCH:-linux-msft-wsl-${KERNEL_SERIES}.y}"
WSL_COMMIT="${WSL_COMMIT:-}"
KDIR="${KDIR:-$HOME/WSL2-Linux-Kernel}"

require_command() {
	if ! command -v "$1" >/dev/null 2>&1; then
		echo "Missing required command: $1" >&2
		exit 1
	fi
}

if ! grep -qi "microsoft\|wsl" /proc/version; then
	echo "This setup script is intended for WSL2." >&2
	exit 1
fi

require_command git
require_command make
require_command sudo
require_command apt
require_command sed
require_command nproc

echo "WSL2 kernel release: $KERNEL_RELEASE"
echo "Using WSL2 kernel branch: $WSL_BRANCH"
if [ -n "$WSL_COMMIT" ]; then
	echo "Using WSL2 kernel commit: $WSL_COMMIT"
fi
echo "Kernel source/build directory: $KDIR"

echo "[1/5] Install build dependencies"
sudo apt update
sudo apt install -y build-essential flex bison libssl-dev libelf-dev bc dwarves

if [ -e "$KDIR" ] && [ ! -d "$KDIR/.git" ]; then
	echo "KDIR exists but is not a git repository: $KDIR" >&2
	echo "Remove it, choose another KDIR, or provide a valid WSL2-Linux-Kernel checkout." >&2
	exit 1
fi

if [ ! -d "$KDIR/.git" ]; then
	echo "[2/5] Clone WSL2 kernel source"
	git clone --depth=1 --branch "$WSL_BRANCH" \
		https://github.com/microsoft/WSL2-Linux-Kernel.git "$KDIR"
else
	echo "[2/5] Reuse existing WSL2 kernel source"
fi

cd "$KDIR"
git fetch --depth=1 origin "$WSL_BRANCH"
git checkout --detach FETCH_HEAD
if [ -n "$WSL_COMMIT" ]; then
	git fetch --depth=1 origin "$WSL_COMMIT"
	git checkout --detach "$WSL_COMMIT"
fi

echo "[3/5] Prepare kernel config"
if [ ! -f .config ]; then
	if [ -f /proc/config.gz ]; then
		require_command zcat
		zcat /proc/config.gz > .config
	elif [ -f "/boot/config-$KERNEL_RELEASE" ]; then
		cp "/boot/config-$KERNEL_RELEASE" .config
	else
		echo "Cannot find /proc/config.gz or /boot/config-$KERNEL_RELEASE." >&2
		exit 1
	fi
fi

echo "[4/5] Normalize config for this kernel tree"
make LOCALVERSION= olddefconfig

TREE_RELEASE="$(make -s LOCALVERSION= kernelrelease)"
if [ "$TREE_RELEASE" != "$KERNEL_RELEASE" ]; then
	echo "Kernel source release does not match the running WSL2 kernel." >&2
	echo "  running: $KERNEL_RELEASE" >&2
	echo "  source : $TREE_RELEASE" >&2
	echo "Set WSL_BRANCH to a branch/tag, or WSL_COMMIT to a commit that matches the running kernel, then retry." >&2
	exit 1
fi

echo "[5/5] Build kernel tree for external modules"
make LOCALVERSION= -j"$(nproc)"

echo "WSL2 kernel build tree is ready: $KDIR"
