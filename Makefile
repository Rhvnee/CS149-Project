CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c11 -D_POSIX_C_SOURCE=200809L
TARGET  = fms
SRC     = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
	rm -rf fs_root

run: all
	./$(TARGET)

.PHONY: all clean run
