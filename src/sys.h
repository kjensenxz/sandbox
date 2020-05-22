#ifndef _SYS_H_
#define _SYS_H_

typedef struct {
	long long int serial;
	char *type;
	int qmp_sock;
	int console_sock;
} vm_t;

int sys_vmstart(char *); // config name
int sys_vmstop(int); // console socket number
int sys_vmreset(int); // console socket number
int sys_vmstatus(int); // console socket number

#endif /* _SYS_H_ */
