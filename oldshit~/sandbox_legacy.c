#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#define die(e) do {                                             \
	errsv = errno;                                          \
	fprintf(stderr, "%s: %s\n", e, strerror(errsv));        \
	exit(EXIT_FAILURE);                                     \
} while (0)

typedef struct vmconf {
	char *mem;
	char *kernel;
	char *initrd;
	char *append;
	char *diskfile;
} vmconf_t;

int isready(char *, size_t);
char **vmconf(char *, int);
int initvm(char **);
int startvm(char **);
void usage(void);

// TODO: make sure dmesg is silence from console
// TODO: holy shit this code is AWFUL!

int errsv;

/* simply checks for shell prompt.
 * returns 1 on ready, 0 on not, -1 on error.
 */
int
isready(char *line, size_t len) {
	if (NULL == line || 1 > len) {
		return -1;
	}

	if (line[0] == '#') {
		return 1;
	}

	return 0;
}

char *const start_template[] = {
	"qemu-system-x86_64", "-m", "", // memsize
	"-chardev", "socket,id=serio,port=%d,host=localhost,server,nowait,telnet",
	"-serial", "chardev:serio", "-nographic",
	"", // backing_image
	NULL 
};

char *const init_template[] = {
	"qemu-system-x86_64", "-m", "", // memsize
	"", "-serial", "mon:stdio", "-nographic", // backing_image
	"-kernel", "",
	"-initrd", "",
	"-append", "",
	NULL
};

/* return a formatted template. */

#define _VM_INIT_  0x01
#define _VM_START_ 0x02
char **vmconf(char *file, int type) {
	char **str = NULL;

	FILE *config = NULL;
	
	ssize_t nchar = -1; 
	char *line = NULL;
	size_t nline = -1;

	char *const *template = NULL;

	int i = -1;
	
	i = access(file, F_OK | R_OK);
	if (0 != i) {
		die("access");
	}
	config = fopen(file, "r");
	if (config == NULL) {
		die("unable to open config file");
	}

	switch (type) {
		case _VM_INIT_:
			str = malloc(14 * sizeof(char *));
			template = init_template;
			break;

		case _VM_START_:
			str = malloc(10 * sizeof(char *));
			template = start_template;
			break;

		default:
			return NULL;
	}

	// so for _VM_INIT_, we need ALL lines.
	// for _VM_START_, we need TWO lines.
	for (i = 0; template[i] != NULL; ++i) {
		if (template[i][0] != '') {
			str[i] = strdup(template[i]);
			continue;
		}

		nchar = getline(&line, &nline, config);
		if (1 > nchar || NULL == line) {
			die("error while reading config file");
		}
		str[i] = malloc((nchar - 1) * sizeof(char));
		strncpy(str[i], line, nchar-1);
//		free(line);
	}
	str[i] = NULL;

	return str;
}

// start vm.
int startvm(char **_argv) {
	int i;
	char *temp;
	for (i = 0; _argv[i] != NULL; ++i) {
		if (_argv[i][0] == '') {
//			temp = strdup((char *)(_argv[i]+1));
			printf("%s\n", _argv[i]);
			
		}
	}
	return -1;
}


/* XXX
 * holy shit what the fuck are you doing
 * so initvm takes the parameters filled in by vmconf into init_template.
 * and then literally just execs it.
 * oh, and it creates a bi-directional pipe to redirect this function's
 * stdout to its child qemu's stdin, and its child qemu's stdout to the
 * parent stdout, mostly for debugging and parsing.
 * XXX
 */

/* initvm takes an ID and starts a VM corresponding to the offset in argps.
 * it allows the VM to boot and start a shell, then saves the guest in a
 * "hot" state to the disk for it to be ready for later use.
 * initvm is more of a one-off function than it is for regular use.
 */
int initvm(char **_argv) {
	char *const argv[] = {
		"qemu-system-x86_64", "-m", "256M", // memsize
		"/home/kbjensen/prog/sandbox/minimal_image/minimal_image.qcow2", "-serial", "mon:stdio", "-nographic", // backing_image
		"-kernel", "/home/kbjensen/prog/sandbox/minimal_image/bzImage",
		"-initrd", "/home/kbjensen/prog/sandbox/minimal_image/initramfs.cpio.gz",
		"-append", "'init=/bin/init console=ttyS0,9600n8 loglevel=7'",
		NULL
	};
	pid_t pid;
	int output[2] = {0};
	int input[2] = {0};
	int rv = -1;

	rv = pipe(output);
	if (0 > rv) {
		die("pipe");
	}
	rv = pipe(input);
	if (0 > rv) {
		die("pipe");
	}

	pid = fork();
	if (0 > pid) {
		die("fork");
	}

	if (0 == pid) {
		rv = dup2(output[1], STDOUT_FILENO);
		if (0 > rv) {
			die("dup2 in parent");
		}
		rv = dup2(input[0], STDIN_FILENO);
		if (0 > rv) {
			die("dup2 in parent");
		}

		rv = close(output[0]);
		if (0 > rv) {
			die("close pipe[0] in parent");
		}
		rv = close(output[1]);
		if (0 > rv) {
			die("close pipe[1] in parent");
		}

		rv = close(input[0]);
		if (0 > rv) {
			die("close pipe[0] in parent");
		}
		rv = close(input[1]);
		if (0 > rv) {
			die("close pipe[0] in parent");
		}

		rv = execvp("qemu-system-x86_64", argv);
		die("execvp");
	}
	else {
		close(output[1]);
		close(input[0]);

		FILE *out = fdopen(output[0], "r");
		char *line = NULL;
		size_t len = 0;
		ssize_t nchar = -1;
		do {
			nchar = getline(&line, &len, out);
			if (1 > nchar) {
				die("getline");
			}
			printf("%s", line);
			if (1 == isready(line, len)) {
				const char qemu_mon_esc[2] = {'\x01', 'c'};
				write(input[1], qemu_mon_esc, 2);
				dprintf(input[1], "stop\n" "savevm start\n" "quit\n");
			}
		} while (0 < rv);
	}

	return EXIT_SUCCESS;
}

void usage(void) {
	fprintf(stderr, "sandbox alpha build 000\n"
	        "Copyright (C) 2019 Kenneth B. Jensen\n"
	        "usage: sandbox [-h] command [cmd options]\n"
	        "\n"
	        "\t-h\tdisplay this help and exit.\n"
	        "\n"
	        "Command syntax:\n"
	        "  init file.config\n"
	        "  start file.config\n"
	        "\n"
	);
}

int
main(int argc, char **argv) {
	char **x;
	if (3 > argc || 0 == strncasecmp(argv[1], "-h", 2)) {
		usage();
		return EXIT_FAILURE;
	}
	if (3 <= argc && 0 == strncasecmp(argv[1], "init", 4)) {
		return initvm(NULL);
	}
	if (3 <= argc && 0 == strncasecmp(argv[1], "start", 5)) {
		x = vmconf(argv[2], _VM_START_);
		return startvm(x);
	}

	return EXIT_SUCCESS;
}

