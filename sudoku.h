/* sudoku.h — shared definitions between kernel module and userspace CLI */
#ifndef SUDOKU_H
#define SUDOKU_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

/* ioctl magic number */
#define SUDOKU_IOC_MAGIC  'S'

/* ioctl commands */
#define SUDOKU_NEW_GAME   _IO(SUDOKU_IOC_MAGIC, 0)   /* load fresh puzzle   */
#define SUDOKU_RESET      _IO(SUDOKU_IOC_MAGIC, 1)   /* restore to puzzle   */
#define SUDOKU_HINT       _IO(SUDOKU_IOC_MAGIC, 2)   /* reveal one cell     */

/* read() returns exactly this many bytes */
#define SUDOKU_BOARD_BYTES  82   /* 81 cell values + 1 status byte */

/* status byte values (byte index 81 of read buffer) */
#define SUDOKU_STATUS_IN_PROGRESS  0
#define SUDOKU_STATUS_WIN          1

#endif /* SUDOKU_H */
