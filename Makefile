CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -pedantic -O2
LDFLAGS := -pthread
TARGET := sudoku_validator
SRC := main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean