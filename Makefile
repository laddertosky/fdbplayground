CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -g

LDFLAGS := 
LIBS := -lfdb_c -lm -lrt -pthread


basic.bin: basic_ops.o common.o
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

single_get_range.bin: single_get_range.o common.o
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

single_vs_multi_get_range.bin: single_vs_multi_get_range.o common.o
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

read_snapshot.bin: read_snapshot.o common.o
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

tx_conflict.bin: tx_conflict.o common.o
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.bin

.PHONY: clean
