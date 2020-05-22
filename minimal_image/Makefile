initramfs.cpio.gz: initramfs/bin/*
	(cd initramfs; find . | cpio -o -H newc; ) | gzip -9 > $(@)

initramfs/bin/init: init.s
	nasm -w+all -g -f elf64 init.s -o init.o
	ld init.o -o $(@)
