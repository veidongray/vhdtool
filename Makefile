CC := gcc
CCFLAGS := -Wall
SRCLINUX := vhdtool-linux.c
SRCWIN := vhdtool-windows.c

vhdtool-all:
	$(CC) $(CCFLAGS) $(SRCLINUX) -o vhdtool-linux
	$(CC) $(CCFLAGS) $(SRCWIN) -o vhdtool-windows

vhdtool-linux:
	$(CC) $(CCFLAGS) $(SRCLINUX) -o $@

vhdtool-windows:
	$(CC) $(CCFLAGS) $(SRCWIN) -o $@

clean-all:
	rm vhdtool-linux vhdtool-windows

clean-linux:
	rm vhdtool-linux

clean-windows:
	rm vhdtool-windows