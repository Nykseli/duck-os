all: run

kernel.bin: kernel_entry.o kernel.o
	i686-elf-ld -m elf_i386 -o $@ -Ttext 0x1000 $^ --oformat binary

kernel_entry.o: kernel_entry.asm
	nasm $< -f elf -o $@

kernel.o: kernel.c
	i686-elf-gcc -m32 -ffreestanding -c $< -o $@ -fno-PIC

kernel.dis: kernel.bin
	ndisasm -b 32 $< > $@

mbr.bin: mbr.asm
	nasm $< -f bin -o $@

os-image.bin: mbr.bin kernel.bin
	cat $^ > $@

run: os-image.bin
	qemu-system-i386 -monitor stdio -fda $<

echo: os-image.bin
	xxd $<

clean:
	$(RM) *.bin *.o *.dis
