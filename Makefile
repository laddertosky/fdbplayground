CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -g

LDFLAGS := 
LIBS := -lfdb_c -lm -lrt -pthread


basic.bin: basic_ops.o common.o
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.bin

.PHONY: clean
