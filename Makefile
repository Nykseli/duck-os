all: run

mbr.bin: mbr.asm
	nasm $< -f bin -o $@

os-image.bin: mbr.bin
	cat $^ > $@

run: os-image.bin
	qemu-system-i386 -fda $<

echo: os-image.bin
	xxd $<

clean:
	$(RM) *.bin *.o *.dis
