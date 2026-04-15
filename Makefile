CFLAGS = -Wall

.PHONY: all memtest

all: minesweeper

memtest: minesweeper
	valgrind ./minesweeper --memtest
