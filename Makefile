all: boot.iso

boot.bin: boot.asm
	nasm -f bin boot.asm -o boot.bin

boot.img: boot.bin
	dd if=/dev/zero of=boot.img bs=1024 count=1440
	dd if=boot.bin of=boot.img conv=notrunc

boot.iso: boot.img
	mkdir -p iso
	cp boot.img iso/
	genisoimage -o boot.iso -b boot.img -hide boot.img iso/

clean:
	rm -f boot.bin boot.img boot.iso
	rm -rf iso
