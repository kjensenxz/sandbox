#!/bin/sh
# not to be used while sandbox is running.

qemu-system-x86_64 -nographic \
	-m 256M -watchdog i6300esb -watchdog-action poweroff \
	-serial mon:stdio \
	-kernel /home/kbjensen/prog/sandbox/minimal_image/bzImage \
	-initrd /home/kbjensen/prog/sandbox/minimal_image/initramfs.cpio.gz \
	-append "init=/bin/init console=ttyS0,115200n8 loglevel=7"


#	-qmp tcp:localhost:4444 -serial chardev:serio \
#	-chardev socket,id=serio,port=5000,host=localhost,server,nowait,telnet \

