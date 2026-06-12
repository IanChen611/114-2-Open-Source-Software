CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -pedantic -O2
LDFLAGS := -pthread
KDIR    ?= $(shell if [ -d /lib/modules/$$(uname -r)/build ]; then echo /lib/modules/$$(uname -r)/build; else echo $(HOME)/WSL2-Linux-Kernel; fi)
KBUILD_FLAGS := LOCALVERSION=

# kernel module target
obj-m += sudoku_module.o

.PHONY: all userspace prepare-wsl2-kernel integration-test clean

# default: build the kernel module
all:
	$(MAKE) -C $(KDIR) M=$(PWD) $(KBUILD_FLAGS) modules

# build the userspace sudoku validator (main.c) and the CLI client
userspace: sudoku_validator sudoku-cli

prepare-wsl2-kernel:
	bash scripts/prepare_wsl2_kernel.sh

sudoku_validator: main.c
	$(CC) $(CFLAGS) main.c -o sudoku_validator $(LDFLAGS)

sudoku-cli: sudoku-cli.c sudoku.h
	$(CC) $(CFLAGS) sudoku-cli.c -o sudoku-cli

integration-test:
	bash scripts/wsl2_integration_test.sh

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) $(KBUILD_FLAGS) clean
	rm -f sudoku_validator sudoku-cli
