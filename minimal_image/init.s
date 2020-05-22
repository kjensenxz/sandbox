; init.s - simple init for amd64 linux platforms
; simply mounts some filesystems and runs /bin/sh
; Copyright (C) 2017 Kenneth B. Jensen <kj@0x5f3759df.xyz>

; BUILDING:
; $ nasm -f elf64 init.s
; $ ld init.o -o init

BITS 64
CPU x64

section .text
global _start

; make mountpoint & mount fs
%macro mount 3

	mov rax, 0x53   ; sys_mkdir
	mov rdi, %1     ; location
	mov rsi, %3     ; mode
	syscall

	mov rax, 0xA5   ; sys_mount
	xor rdi, rdi    ; device name
	mov rsi, %1     ; location
	mov rdx, %2     ; filesystem type
	xor r10, r10    ; flags
	xor r8, r8      ; data
	syscall
%endmacro

; Copyright (C) 2015-2018 2 Ton Digital
; Stolen from HeavyThing x86_65 assembly language library/showcase programs
; (see sleeps.inc)
; Homepage: https://2ton.com.au/
; Author: Jeff Marrison <jeff@2ton.com.au
; GPLv3 or later. <http://www.gnu.org/licenses/>
%macro sleep 1
	sub rsp, 16
	mov qword [rsp], %1
	mov qword [rsp+8], 0
	mov rdi, rsp
	xor esi, esi
	mov eax, 0x23 ; sys_nanosleep
	syscall
	add rsp, 16
%endmacro


%define O_RDWR 0x02 ; see /usr/include/asm-generic/fcntl.h

_start:
	; mount filesystems
	mount p_path, p_fs, 755o ; /proc
	mount s_path, s_fs, 755o ; /sys
	mount t_path, t_fs, 777o ; /tmp

	; TODO: fork, let parent do init stuff like poking the watchdog
	mov rax, 0x39    ; sys_fork
	syscall

	cmp rax, 0
	je _getty        ; child

_parent:
	mov rax, 0x02    ; sys_open
	mov rdi, w_path  ; path
	mov rsi, O_RDWR  ; flags
	xor rdx, rdx     ; mode
	syscall

	push rax

_watchdog:
	mov rax, 0x01    ; sys_write
	pop rdi          ; file descriptor
	push rdi
	xor rsi, rsi     ; buffer (1 char; '\0')
	mov rdx, 1       ; count
	syscall

	sleep 10
	jmp _watchdog


_getty:
	; exec /bin/getty -n -l /bin/sh 9600 ttyS0
	; this is due to launching straight into /bin/sh will give an error:
	;   "/bin/sh: can't access tty; job control turned off"
	; /dev/tty is a virtual terminal identical to the current terminal. but
	; it isn't setup if you're on a raw tty, e.g. a serial connecton.
	; /dev/tty must be set up by getty... I think. at least, this fixes it.

	; see man 2 execve
	mov rax, 0x3B    ; sys_execve
	mov rdi, init_path ; filename
	mov rsi, init_argv ; argv
	xor rdx, rdx     ; envp, not really necessary
	syscall

	; end of program, /bin/sh exited
	mov rax, 0x3C    ; sys_exit
	mov rdi, 1       ; status (EXIT_FAILURE)
	syscall
	

	
section .data

p_path:
	db "/proc", 0
p_fs:
	db "proc", 0

s_path:
	db "/sys", 0
s_fs:
	db "sysfs", 0

t_path:
	db "/tmp", 0
t_fs:
	db "ramfs", 0

w_path:
	db "/dev/watchdog", 0
w_str:
	db 0


; please fucking kill me it hurts to live
init_path:
	db "/bin/getty", 0
init_arg1:
	db "-n", 0
init_arg2:
	db "-l", 0
init_arg3:
	db "/bin/sh", 0
init_arg4:
	db "9600", 0
init_arg5:
	db "ttyS0", 0
init_argv:
	dq init_path, init_arg1, init_arg2, init_arg3, init_arg4, init_arg5, 0

