#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODULE_PATH="$PROJECT_ROOT/sudoku_module.ko"
DEVICE_PATH="/dev/sudoku"
KERNEL_RELEASE="$(uname -r)"
LOG_PATH="$PROJECT_ROOT/integration-test.log"

cleanup() {
	if grep -q "^sudoku_module " /proc/modules; then
		sudo rmmod sudoku_module
	fi
}

require_command() {
	if ! command -v "$1" >/dev/null 2>&1; then
		echo "Missing required command: $1" >&2
		exit 1
	fi
}

if ! grep -qi "microsoft\|wsl" /proc/version; then
	echo "This integration test is intended for WSL2." >&2
	exit 1
fi

if [ -n "${KDIR:-}" ]; then
	:
elif [ -d "/lib/modules/$KERNEL_RELEASE/build" ]; then
	KDIR="/lib/modules/$KERNEL_RELEASE/build"
else
	KDIR="$HOME/WSL2-Linux-Kernel"
fi
export KDIR

if [ ! -d "$KDIR" ]; then
	echo "KDIR does not exist: $KDIR" >&2
	echo "Run 'make prepare-wsl2-kernel' first, or set KDIR=/path/to/kernel/build." >&2
	exit 1
fi

require_command make
require_command gcc
require_command sudo
require_command stat
require_command insmod
require_command rmmod
require_command grep
require_command dd
require_command od
require_command awk
require_command tee
require_command date
require_command cat

if [ "${SUDOKU_INTEGRATION_LOGGING:-0}" != "1" ]; then
	export SUDOKU_INTEGRATION_LOGGING=1
	exec > >(tee "$LOG_PATH") 2>&1
fi

cd "$PROJECT_ROOT"

echo "WSL2 integration test started: $(date -Iseconds)"
echo "Kernel release: $KERNEL_RELEASE"
echo "KDIR: $KDIR"

echo "[1/6] Build userspace tools"
make userspace

echo "[2/6] Build kernel module"
make

if [ ! -f "$MODULE_PATH" ]; then
	echo "Kernel module was not produced: $MODULE_PATH" >&2
	exit 1
fi

trap cleanup EXIT

echo "[3/6] Load kernel module"
cleanup
sudo insmod "$MODULE_PATH"

if [ ! -c "$DEVICE_PATH" ]; then
	echo "$DEVICE_PATH was not created." >&2
	exit 1
fi

echo "[4/6] Read board state from character device"
sudo dd if="$DEVICE_PATH" of=/tmp/sudoku_state.bin bs=82 count=1 status=none
state_size="$(stat -c %s /tmp/sudoku_state.bin)"
if [ "$state_size" != "82" ]; then
	echo "Expected 82 bytes from $DEVICE_PATH, got $state_size." >&2
	exit 1
fi
empty_cells="$(od -An -v -t u1 /tmp/sudoku_state.bin |
	awk '{ for (i = 1; i <= NF; i++) { seen++; if (seen <= 81 && $i == 0) empty++ } } END { print empty + 0 }')"
if [ "$empty_cells" != "40" ]; then
	echo "Expected generated puzzle to have 40 empty cells, got $empty_cells." >&2
	exit 1
fi

echo "[5/6] Smoke test CLI ioctl commands"
printf "hint\nreset\nnew\nquit\n" | sudo ./sudoku-cli >/tmp/sudoku_cli_smoke.log
grep -q "Goodbye!" /tmp/sudoku_cli_smoke.log
echo "CLI smoke output:"
cat /tmp/sudoku_cli_smoke.log

echo "[6/6] Unload kernel module"
cleanup
trap - EXIT

echo "Recent kernel log entries:"
dmesg | grep "sudoku:" | tail -20 || true

echo "WSL2 integration test passed."
echo "Result log: $LOG_PATH"
