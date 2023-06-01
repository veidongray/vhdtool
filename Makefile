CC := gcc
CCFLAGS := -Wall
SRC := vhdtool-linux.c

vhdtool:
	$(CC) $(CCFLAGS) $(SRC) -o $@
clean:
	rm vhdtool