all: boot.iso

boot.o: boot.s
	as -o boot.o boot.s

boot.bin: boot.o
	ld -Ttext 0x7C00 --oformat binary -o boot.bin boot.o

boot.img: boot.bin
	dd if=/dev/zero of=boot.img bs=1024 count=1440
	dd if=boot.bin of=boot.img conv=notrunc

boot.iso: boot.img
	mkdir -p iso
	cp boot.img iso/
	genisoimage -o boot.iso -b boot.img -hide boot.img iso/

clean:
	rm -f boot.o boot.bin boot.img boot.iso
	rm -rf iso
