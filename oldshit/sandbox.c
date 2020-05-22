#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include "sandbox.h"
#include "config.h"


static int errsv;


int isshrdy(char *, size_t);
vmconf_t *getvmconf(char *, int);
int initvm(char *const *);
int startvm(char **, vmconf_t *);
void usage(void);

void *get_in_addr(struct sockaddr *);

int stopvm(int);

/* Thanks, Beej! */
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


/* to check if shell is ready or not for the initialization to halt the VM.
 * returns -1 on error, 1 on success, and 0 on failure.
 */
int
isshrdy(char *line, size_t len) {
	if (NULL == line || 1 > len) {
		return -1;
	}

	/* shell prompt character, perhaps not the most reliable methodology */
	if ('#' == line[0]) {
		return 1;
	}

	return 0;
}

/* connects via QMP to the running VM, and sends the quit command. */
int
stopvm(int portno) {
	int sock = -1;
	int rv = -1;
	struct addrinfo hints = {0}, *svinfo = NULL, *p = NULL;
	char *service = NULL;
	char buf[256] = {0};

	assert(0 < portno);

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	rv = asprintf(&service, "%d", portno); // requires _GNU_SOURCE
	if (0 > rv) {
		die("no memory in stopvm/asprintf \"itoa\"");
	}

	rv = getaddrinfo("localhost", service, &hints, &svinfo);
	if (0 != rv) {
		die("could not getaddrinfo for localhost");
	}

	for (p = svinfo; p != NULL; p = p->ai_next) {
		sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (0 > sock) {
			perror("error opening socket to localhost. retrying.");
			continue;
		}

		rv = connect(sock, p->ai_addr, p->ai_addrlen);
		if (0 > rv) {
			perror("error connecting to localhost. retrying.");
			close(sock);
			continue;
		}

		break;
	}

	if (p == NULL) {
		die("failed to connect to localhost.");
	}

	freeaddrinfo(svinfo);
	free(service);

	rv = -1;

	while (0 < read(sock, buf, 255)) {
		write(0, buf, 255);
		if (rv == -1) {
			rv = dprintf(sock, 
			             "{\"execute\": \"qmp_capabilities\"}\n"
			);
		}
		else {
			dprintf(sock, "{\"execute\": \"quit\"}\n");
			break;
		}
		memset(buf, 255, sizeof(char));
	}
	
	close(sock);
	return EXIT_SUCCESS;
}

/* fork and start the vm to be connected to via telnet.
 * returns whatever qemu returns.
 */
int
startvm(char **execargv, vmconf_t *conf) {
	pid_t pid;
	int status;
	
	pid = fork();
	if (0 > pid) {
		die("fork");
	}

	else if (0 < pid) { /* parent */
		/* XXX: we can probably chingale some shit into detecting
		 * if qemu is started... and listening on port X.
		 * maybe some sort of notification from qemu... probably QMP.
		 */

		if (0 < waitpid(pid, &status, 0)) {
			printf("Exited successfully.\n");
		}
	}
	else {
		execvp("qemu-system-x86_64", execargv);
	}

	return -1;
}

int
initvm(char *const *execargv) {
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
		/* we need to be able to read to/from the child in order to
		 * send commands to the QEMU monitor and redirect its stdout
		 * to parent for users to view.
		 * XXX: this may be changed to save all of the serial output 
		 * to a file so that it may be re-displayed at a later date.
		 */
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

		rv = execvp("qemu-system-x86_64", execargv);
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
			printf("%s", line);
			if (1 == isshrdy(line, len)) {
				/* ctrl-a c */
				const char qemu_mon_esc[2] = {'\x01', 'c'};
				write(input[1], qemu_mon_esc, 2);
				dprintf(input[1], 
					"stop\n" "savevm start\n" "quit\n"
				);
			}
		} while (0 < nchar);

		fclose(out);
		free(line);
	}

	return EXIT_SUCCESS;
}

int
main(int argc, char **argv) {
	vmconf_t *conf = NULL;
	char **execargv = NULL;
	int i = -1;

	if (argc < 3 ||
	    0 == strncasecmp(argv[1], "-h", 2) ||
	    0 == strncmp(argv[1], "-?", 2)) {
		    usage();
	}

	if (0 == strncasecmp(argv[1], "stop", 4)) {
		stopvm(atoi(argv[2]));
		return EXIT_SUCCESS;
	}
	
	conf = vmconfig(argv[2]);

	if (0 == strncasecmp(argv[1], "init", 4)) {
		execargv = initargv(conf);
	}
	else if (0 == strncasecmp(argv[1], "start", 5)) {
		conf->portno = 5000;
		execargv = startargv(conf);
	}
	else {
		usage();
	}

	if (0 == strncasecmp(argv[1], "init", 4)) {
		initvm(execargv);
	}
	else if (0 == strncasecmp(argv[1], "start", 5)) {
		startvm(execargv, conf);
	}

	/* ...and it's our responsibility to free 
	 * everything we got allocated.
	 */
	for (i = 0; execargv[i] != NULL; ++i) {
		free(execargv[i]);
	}

	free(execargv);
	free(conf->memsz);
	free(conf->kernel);
	free(conf->initrd);
	free(conf->append);
	free(conf->imgfile);
	free(conf);

	return EXIT_SUCCESS;
}

__attribute__ ((noreturn))
void
usage(void) {
	fprintf(stderr,
		"Usage: sandbox [-h?] <init|start|stop> [params]\n"
		"      -h, -?: Show this message.\n"
		"      init vm.config: run the initialization process.\n"
		"      start vm.config: start a virtual machine!\n"
		"      stop qmpportno: stop a virtual machine.\n"
		"sandbox pre-alpha 20191226 (C) Kenneth B. Jensen\n"
	);

	exit(EXIT_SUCCESS);
}
