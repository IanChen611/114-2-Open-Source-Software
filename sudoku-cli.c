/*
 * sudoku-cli.c — userspace CLI client for the sudoku kernel module
 *
 * Usage:
 *   sudo ./sudoku-cli
 *
 * In-game commands:
 *   row col val   — place a digit (1-9) at the given cell (1-indexed)
 *   row col 0     — clear a cell
 *   hint          — reveal one empty cell
 *   reset         — restore the original puzzle
 *   new           — start a new game
 *   quit / exit   — exit the program
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "sudoku.h"

#define DEVICE_PATH "/dev/sudoku"
#define SIZE        9

/* ── board rendering ─────────────────────────────────────────────────── */

static void print_board(const uint8_t *cells, uint8_t status)
{
	int r, c;

	printf("\n");
	printf("    1 2 3   4 5 6   7 8 9\n");
	printf("  +-------+-------+-------+\n");

	for (r = 0; r < SIZE; r++) {
		printf("%d | ", r + 1);
		for (c = 0; c < SIZE; c++) {
			uint8_t v = cells[r * SIZE + c];
			if (v == 0)
				printf(". ");
			else
				printf("%d ", v);
			if (c == 2 || c == 5)
				printf("| ");
		}
		printf("\n");
		if (r == 2 || r == 5)
			printf("  +-------+-------+-------+\n");
	}
	printf("  +-------+-------+-------+\n");

	if (status == SUDOKU_STATUS_WIN)
		printf("\n  🎉  Congratulations — Sudoku solved!\n");

	printf("\n");
}

/* ── device helpers ──────────────────────────────────────────────────── */

/* Read board state from /dev/sudoku.  Returns 0 on success, -1 on error. */
static int read_board(int fd, uint8_t buf[SUDOKU_BOARD_BYTES])
{
	ssize_t n;

	/* char device always returns current state; no lseek needed */
	n = read(fd, buf, SUDOKU_BOARD_BYTES);
	if (n != SUDOKU_BOARD_BYTES) {
		perror("read /dev/sudoku");
		return -1;
	}
	return 0;
}

/* Send move "row col val" (0-indexed) to /dev/sudoku. */
static int send_move(int fd, int row, int col, int val)
{
	char msg[32];
	ssize_t len = snprintf(msg, sizeof(msg), "%d %d %d\n", row, col, val);

	if (write(fd, msg, len) < 0) {
		if (errno == EPERM)
			printf("  ✗  That cell is fixed — you cannot change it.\n");
		else if (errno == EINVAL)
			printf("  ✗  Invalid move — conflicts with row, column, or box.\n");
		else
			perror("write /dev/sudoku");
		return -1;
	}
	return 0;
}

/* ── main loop ───────────────────────────────────────────────────────── */

int main(void)
{
	int fd;
	uint8_t buf[SUDOKU_BOARD_BYTES];
	char line[64];

	fd = open(DEVICE_PATH, O_RDWR);
	if (fd < 0) {
		fprintf(stderr,
			"Cannot open %s: %s\n"
			"Make sure the module is loaded:  sudo insmod sudoku_module.ko\n",
			DEVICE_PATH, strerror(errno));
		return EXIT_FAILURE;
	}

	printf("╔══════════════════════════════╗\n");
	printf("║   Sudoku  — Kernel Edition   ║\n");
	printf("╚══════════════════════════════╝\n");
	printf("Commands:  row col val  |  hint  |  reset  |  new  |  quit\n");
	printf("           (row/col are 1-indexed; val 0 clears a cell)\n");

	/* initial board display */
	if (read_board(fd, buf) == 0)
		print_board(buf, buf[SIZE * SIZE]);

	while (1) {
		printf("sudoku> ");
		fflush(stdout);

		if (!fgets(line, sizeof(line), stdin))
			break;

		/* strip trailing newline */
		line[strcspn(line, "\n")] = '\0';

		/* ── quit ── */
		if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0)
			break;

		/* ── hint ── */
		if (strcmp(line, "hint") == 0) {
			if (ioctl(fd, SUDOKU_HINT, 0) < 0)
				perror("ioctl SUDOKU_HINT");
			else
				printf("  💡  Hint applied.\n");
			goto refresh;
		}

		/* ── reset ── */
		if (strcmp(line, "reset") == 0) {
			if (ioctl(fd, SUDOKU_RESET, 0) < 0)
				perror("ioctl SUDOKU_RESET");
			else
				printf("  ↺  Board reset to original puzzle.\n");
			goto refresh;
		}

		/* ── new game ── */
		if (strcmp(line, "new") == 0) {
			if (ioctl(fd, SUDOKU_NEW_GAME, 0) < 0)
				perror("ioctl SUDOKU_NEW_GAME");
			else
				printf("  ✦  New game started.\n");
			goto refresh;
		}

		/* ── move: "row col val" ── */
		{
			int row, col, val;

			if (sscanf(line, "%d %d %d", &row, &col, &val) == 3) {
				if (row < 1 || row > SIZE ||
				    col < 1 || col > SIZE ||
				    val < 0 || val > SIZE) {
					printf("  ✗  Out of range. "
					       "row/col: 1-%d, val: 0-%d\n",
					       SIZE, SIZE);
				} else {
					/* convert to 0-indexed before sending */
					send_move(fd, row - 1, col - 1, val);
				}
				goto refresh;
			}
		}

		/* ── unknown command ── */
		if (strlen(line) > 0)
			printf("  ?  Unknown command: \"%s\"\n", line);
		continue;

refresh:
		if (read_board(fd, buf) == 0)
			print_board(buf, buf[SIZE * SIZE]);
	}

	close(fd);
	printf("Goodbye!\n");
	return EXIT_SUCCESS;
}
