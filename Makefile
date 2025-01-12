all: boot.bin

boot.o: boot.s
	as -o boot.o boot.s

second.o: second.s
	as -o second.o second.s

boot.bin: boot.o second.o
	ld -Ttext 0x7C00 --oformat binary -o boot.bin boot.o second.o

clean:
	rm -f *.o *.bin

.PHONY: all clean
