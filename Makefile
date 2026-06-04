CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -pedantic -O2
LDFLAGS := -pthread
KDIR    ?= $(HOME)/WSL2-Linux-Kernel

# kernel module target
obj-m += sudoku_module.o

.PHONY: all userspace clean

# default: build the kernel module
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# build the userspace sudoku validator (main.c) and the CLI client
userspace: sudoku_validator sudoku-cli

sudoku_validator: main.c
	$(CC) $(CFLAGS) main.c -o sudoku_validator $(LDFLAGS)

sudoku-cli: sudoku-cli.c sudoku.h
	$(CC) $(CFLAGS) sudoku-cli.c -o sudoku-cli

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f sudoku_validator sudoku-cli
