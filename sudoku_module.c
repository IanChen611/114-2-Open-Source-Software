/*
 * sudoku_module.c — Linux kernel module implementing a Sudoku game
 *
 * Exposes /dev/sudoku as a character device.
 *
 *   read()   → 82 bytes: [0..80] board cell values (0 = empty, 1-9 = digit)
 *                         [81]   status byte (0 = in progress, 1 = win)
 *
 *   write()  → "row col val\n"  (0-indexed row/col; val 1-9 or 0 to clear)
 *
 *   ioctl()  → SUDOKU_NEW_GAME  : load a fresh puzzle
 *              SUDOKU_RESET     : restore board to original puzzle state
 *              SUDOKU_HINT      : fill in one empty cell from the solution
 */

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include "sudoku.h"

#define DEVICE_NAME "sudoku"
#define CLASS_NAME  "sudoku"
#define SIZE        9

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chen Yi-Yang & Du Yong-Lin");
MODULE_DESCRIPTION("Sudoku game as a Linux kernel character device");
MODULE_VERSION("1.0");

/* ── game state ─────────────────────────────────────────────────────── */

static uint8_t board[SIZE][SIZE];       /* current game state              */
static uint8_t puzzle[SIZE][SIZE];      /* original given cells (0=empty)  */
static uint8_t solution[SIZE][SIZE];    /* full correct solution            */
static uint8_t fixed_cells[SIZE][SIZE]; /* 1 = given cell, cannot change   */
static DEFINE_MUTEX(game_mutex);

/* hardcoded puzzle — the classic "newspaper" Sudoku */
static const uint8_t default_puzzle[SIZE][SIZE] = {
	{5, 3, 0, 0, 7, 0, 0, 0, 0},
	{6, 0, 0, 1, 9, 5, 0, 0, 0},
	{0, 9, 8, 0, 0, 0, 0, 6, 0},
	{8, 0, 0, 0, 6, 0, 0, 0, 3},
	{4, 0, 0, 8, 0, 3, 0, 0, 1},
	{7, 0, 0, 0, 2, 0, 0, 0, 6},
	{0, 6, 0, 0, 0, 0, 2, 8, 0},
	{0, 0, 0, 4, 1, 9, 0, 0, 5},
	{0, 0, 0, 0, 8, 0, 0, 7, 9}
};

static const uint8_t default_solution[SIZE][SIZE] = {
	{5, 3, 4, 6, 7, 8, 9, 1, 2},
	{6, 7, 2, 1, 9, 5, 3, 4, 8},
	{1, 9, 8, 3, 4, 2, 5, 6, 7},
	{8, 5, 9, 7, 6, 1, 4, 2, 3},
	{4, 2, 6, 8, 5, 3, 7, 9, 1},
	{7, 1, 3, 9, 2, 4, 8, 5, 6},
	{9, 6, 1, 5, 3, 7, 2, 8, 4},
	{2, 8, 7, 4, 1, 9, 6, 3, 5},
	{3, 4, 5, 2, 8, 6, 1, 7, 9}
};

/* ── validation helpers ─────────────────────────────────────────────── */

static int validate_row(int row)
{
	uint8_t seen[SIZE + 1] = {};
	int c;

	for (c = 0; c < SIZE; c++) {
		uint8_t v = board[row][c];

		if (!v)
			continue;
		if (v > SIZE || seen[v]++)
			return 0;
	}
	return 1;
}

static int validate_col(int col)
{
	uint8_t seen[SIZE + 1] = {};
	int r;

	for (r = 0; r < SIZE; r++) {
		uint8_t v = board[r][col];

		if (!v)
			continue;
		if (v > SIZE || seen[v]++)
			return 0;
	}
	return 1;
}

static int validate_box(int box)
{
	uint8_t seen[SIZE + 1] = {};
	int sr = (box / 3) * 3;
	int sc = (box % 3) * 3;
	int r, c;

	for (r = sr; r < sr + 3; r++) {
		for (c = sc; c < sc + 3; c++) {
			uint8_t v = board[r][c];

			if (!v)
				continue;
			if (v > SIZE || seen[v]++)
				return 0;
		}
	}
	return 1;
}

/*
 * is_move_valid - check if placing val at (row, col) violates any constraint.
 * The cell must currently be 0.  We temporarily place val, validate, restore.
 */
static int is_move_valid(int row, int col, uint8_t val)
{
	int ok;

	board[row][col] = val;
	ok = validate_row(row) &&
	     validate_col(col) &&
	     validate_box((row / 3) * 3 + col / 3);
	board[row][col] = 0;
	return ok;
}

static uint8_t get_status(void)
{
	int r, c;

	for (r = 0; r < SIZE; r++)
		for (c = 0; c < SIZE; c++)
			if (board[r][c] != solution[r][c])
				return SUDOKU_STATUS_IN_PROGRESS;
	return SUDOKU_STATUS_WIN;
}

/* ── game control ───────────────────────────────────────────────────── */

static void load_new_game(void)
{
	int r, c;

	for (r = 0; r < SIZE; r++) {
		for (c = 0; c < SIZE; c++) {
			puzzle[r][c]      = default_puzzle[r][c];
			solution[r][c]    = default_solution[r][c];
			board[r][c]       = default_puzzle[r][c];
			fixed_cells[r][c] = (default_puzzle[r][c] != 0) ? 1 : 0;
		}
	}
	pr_info("sudoku: new game loaded\n");
}

/* ── char device bookkeeping ─────────────────────────────────────────── */

static int           major_num;
static struct class  *sudoku_class;
static struct device *sudoku_dev;
static struct cdev    sudoku_cdev;

/* ── file operations ─────────────────────────────────────────────────── */

static int sudoku_open(struct inode *inode, struct file *file)
{
	pr_info("sudoku: device opened\n");
	return 0;
}

static int sudoku_release(struct inode *inode, struct file *file)
{
	pr_info("sudoku: device closed\n");
	return 0;
}

/*
 * sudoku_read - copy board state to userspace.
 * Returns SUDOKU_BOARD_BYTES (82): bytes 0-80 are cell values, byte 81 is
 * the status (SUDOKU_STATUS_IN_PROGRESS or SUDOKU_STATUS_WIN).
 */
static ssize_t sudoku_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	uint8_t tmp[SUDOKU_BOARD_BYTES];
	int r, c;

	/* char device: ignore ppos, always return current board state */
	if (count < SUDOKU_BOARD_BYTES)
		return -EINVAL;

	mutex_lock(&game_mutex);
	for (r = 0; r < SIZE; r++)
		for (c = 0; c < SIZE; c++)
			tmp[r * SIZE + c] = board[r][c];
	tmp[SIZE * SIZE] = get_status();
	mutex_unlock(&game_mutex);

	if (copy_to_user(buf, tmp, SUDOKU_BOARD_BYTES))
		return -EFAULT;

	return SUDOKU_BOARD_BYTES;
}

/*
 * sudoku_write - accept a move from userspace.
 * Format: "row col val\n"  (0-indexed row/col, val 1-9; val 0 clears cell)
 * Returns -EPERM  if the target cell is a given (fixed) cell.
 * Returns -EINVAL if the move violates a Sudoku constraint.
 */
static ssize_t sudoku_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *ppos)
{
	char kbuf[32];
	int row, col, val;
	size_t len = min(count, sizeof(kbuf) - 1);

	if (copy_from_user(kbuf, buf, len))
		return -EFAULT;
	kbuf[len] = '\0';

	if (sscanf(kbuf, "%d %d %d", &row, &col, &val) != 3)
		return -EINVAL;
	if (row < 0 || row >= SIZE || col < 0 || col >= SIZE)
		return -EINVAL;
	if (val < 0 || val > SIZE)
		return -EINVAL;

	mutex_lock(&game_mutex);

	if (fixed_cells[row][col]) {
		mutex_unlock(&game_mutex);
		pr_warn("sudoku: cannot modify fixed cell (%d,%d)\n", row, col);
		return -EPERM;
	}

	if (val != 0 && !is_move_valid(row, col, (uint8_t)val)) {
		mutex_unlock(&game_mutex);
		pr_warn("sudoku: invalid move (%d,%d)=%d\n", row, col, val);
		return -EINVAL;
	}

	board[row][col] = (uint8_t)val;
	pr_info("sudoku: placed %d at (%d,%d)\n", val, row, col);

	mutex_unlock(&game_mutex);
	return (ssize_t)count;
}

/*
 * sudoku_ioctl - control commands.
 *   SUDOKU_NEW_GAME : reload the default puzzle
 *   SUDOKU_RESET    : restore board to original puzzle (clears user moves)
 *   SUDOKU_HINT     : fill the first empty non-fixed cell from the solution
 */
static long sudoku_ioctl(struct file *file, unsigned int cmd,
			 unsigned long arg)
{
	int r, c, found;

	switch (cmd) {
	case SUDOKU_NEW_GAME:
		mutex_lock(&game_mutex);
		load_new_game();
		mutex_unlock(&game_mutex);
		break;

	case SUDOKU_RESET:
		mutex_lock(&game_mutex);
		for (r = 0; r < SIZE; r++)
			for (c = 0; c < SIZE; c++)
				board[r][c] = puzzle[r][c];
		pr_info("sudoku: board reset to original puzzle\n");
		mutex_unlock(&game_mutex);
		break;

	case SUDOKU_HINT:
		mutex_lock(&game_mutex);
		found = 0;
		for (r = 0; r < SIZE && !found; r++) {
			for (c = 0; c < SIZE && !found; c++) {
				if (!fixed_cells[r][c] && board[r][c] == 0) {
					board[r][c] = solution[r][c];
					pr_info("sudoku: hint at (%d,%d)=%d\n",
						r, c, solution[r][c]);
					found = 1;
				}
			}
		}
		if (!found)
			pr_info("sudoku: no empty cells remain for hint\n");
		mutex_unlock(&game_mutex);
		break;

	default:
		return -ENOTTY;
	}
	return 0;
}

static const struct file_operations sudoku_fops = {
	.owner          = THIS_MODULE,
	.open           = sudoku_open,
	.release        = sudoku_release,
	.read           = sudoku_read,
	.write          = sudoku_write,
	.unlocked_ioctl = sudoku_ioctl,
};

/* ── module init / exit ─────────────────────────────────────────────── */

static int __init sudoku_init(void)
{
	dev_t dev;
	int ret;

	ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
	if (ret < 0) {
		pr_err("sudoku: alloc_chrdev_region failed (%d)\n", ret);
		return ret;
	}
	major_num = MAJOR(dev);

	cdev_init(&sudoku_cdev, &sudoku_fops);
	sudoku_cdev.owner = THIS_MODULE;

	ret = cdev_add(&sudoku_cdev, dev, 1);
	if (ret < 0) {
		pr_err("sudoku: cdev_add failed (%d)\n", ret);
		unregister_chrdev_region(dev, 1);
		return ret;
	}

	/* kernel 6.4+ dropped the THIS_MODULE argument from class_create */
	sudoku_class = class_create(CLASS_NAME);
	if (IS_ERR(sudoku_class)) {
		ret = PTR_ERR(sudoku_class);
		pr_err("sudoku: class_create failed (%d)\n", ret);
		cdev_del(&sudoku_cdev);
		unregister_chrdev_region(dev, 1);
		return ret;
	}

	sudoku_dev = device_create(sudoku_class, NULL, dev, NULL, DEVICE_NAME);
	if (IS_ERR(sudoku_dev)) {
		ret = PTR_ERR(sudoku_dev);
		pr_err("sudoku: device_create failed (%d)\n", ret);
		class_destroy(sudoku_class);
		cdev_del(&sudoku_cdev);
		unregister_chrdev_region(dev, 1);
		return ret;
	}

	load_new_game();
	pr_info("sudoku: module loaded, /dev/%s ready (major=%d)\n",
		DEVICE_NAME, major_num);
	return 0;
}

static void __exit sudoku_exit(void)
{
	dev_t dev = MKDEV(major_num, 0);

	device_destroy(sudoku_class, dev);
	class_destroy(sudoku_class);
	cdev_del(&sudoku_cdev);
	unregister_chrdev_region(dev, 1);
	pr_info("sudoku: module unloaded\n");
}

module_init(sudoku_init);
module_exit(sudoku_exit);
