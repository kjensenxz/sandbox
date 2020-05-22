#define _GNU_SOURCE // depends on asprintf for now, a GNU extension...
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "sandbox.h"
#include "config.h"

void disp_vmconf(vmconf_t *);


void
disp_vmconf(vmconf_t *conf) {
	DEBUG("entering disp_vmconf");

	assert(NULL != conf);
	assert(0 < conf->portno);
	assert(NULL != conf->memsz);
	assert(NULL != conf->kernel);
	assert(NULL != conf->initrd);
	assert(NULL != conf->append);
	assert(NULL != conf->imgfile);


	DEBUG("vmconf_t: port:%d memsz:%s kernel:%s initrd:%s append:%s "
	      "imgfile:%s",
	       conf->portno, conf->memsz, conf->kernel, conf->initrd, 
	       conf->append, conf->imgfile
	);

	return;
}

/* vmconfig will allocate a vmconf_t for parsing into argv for execvp.
 * it expects a path; may be changed to a FILE and expect the caller to
 * open the config file.
 * anyways, free after use.
 * call it first; load up the empty fields (like portno!)
 * XXX: will need to be updated for seperate snapshots
 */
vmconf_t *
vmconfig(char *path) {
	assert(NULL != path);
	DEBUG("vmconfig() entered with path '%s'", path);

	vmconf_t *conf = calloc(1, sizeof(vmconf_t));
	FILE *fcfg = NULL;

	ssize_t nread = -1;
	size_t n = 0;
	char *line = NULL;
	char *pwd = NULL;

	pwd = getcwd(NULL, 256);
	if (NULL == pwd) {
		die("shit went bad in vmconfig trying to calloc for pwd\n");
		return NULL;
	}

	fcfg = fopen(path, "r");
	if (NULL == fcfg) {
		fprintf(stderr, "unable to open %s in dir %s in vmconfig.\n",
		        path, pwd
		);
		free(pwd);
		return NULL;
	}

	while (0 < (nread = getline(&line, &n, fcfg))) {
		// comments start with '#' at beginning of line.
		if (line[0] == '#') {
			continue;
		}
		
		if (0 == strncmp(line, "memsz: ", 7)) {
			// chomp the \n; strndup will replace it with \0.
			// we don't want \n in a string prepared for execvp.
			conf->memsz = strndup(line+7, strlen(line+7)-1);
		}
		else if (0 == strncmp(line, "kernel: ", 8)) {
			conf->kernel = strndup(line+8, strlen(line+8)-1);
		}
		else if (0 == strncmp(line, "initrd: ", 8)) {
			conf->initrd = strndup(line+8, strlen(line+8)-1);
		}
		else if (0 == strncmp(line, "append: ", 8)) {
			conf->append = strndup(line+8, strlen(line+8)-1);
		}
		else if (0 == strncmp(line, "imgfile: ", 9)) {
			conf->imgfile = strndup(line+9, strlen(line+9)-1);
		}
	}

	free(pwd);
	free(line);
	fclose(fcfg);

	printf("returning conf...");
	return conf;
}

/* ...
 * This function allocates a char * array for use with execvp to run qemu from
 * sandbox.c.
 * yes, it's ugly.
 * XXX: may need to be updated for seperate snapshots.
 */
char **execvpconf(vmconf_t *, char **);
char *init_template[] = {
	"qemu-system-x86_64", "-nographic",
	"-m", "%s", "memsz",
	"%s", "imgfile",
	"-serial", "mon:stdio",
	"-kernel", "%s", "kernel",
	"-initrd", "%s", "initrd",
	"-append", "%s", "append",
	"-watchdog", "i6300esb",
	"-watchdog-action", "poweroff",
	NULL
};
#define INIT_LEN 18

char *start_template[] = {
	"qemu-system-x86_64", "-nographic",
	"-m", "%s", "memsz",
	"%s", "imgfile",
	"-serial", "chardev:serio",
	"-chardev",
	"socket,id=serio,port=%d,host=localhost,server,nowait,telnet",
	"-loadvm", "start",
	"-qmp", "tcp:localhost:4444,server,nowait",
	"-device", "pvpanic",
	"-watchdog", "i6300esb",
	"-watchdog-action", "poweroff",
	NULL
};
#define START_LEN 20

char **
initargv(vmconf_t *conf) {
	return execvpconf(conf, init_template);
}

char **
startargv(vmconf_t *conf) {
	return execvpconf(conf, start_template); 
}

char **
execvpconf(vmconf_t *conf, char **template) {
	assert(NULL != conf);
	assert(NULL != template);
	assert(template == start_template || template == init_template);

	char **argv = NULL;
	int argc = 0;
	int i = 0;

	if (template == start_template) {
		argv = calloc(START_LEN, sizeof(char *));
	}
	else if (template == init_template) {
		argv = calloc(INIT_LEN, sizeof(char *));
	}
	else {
		return NULL;
	}

	while (NULL != template[i]) {
		assert(NULL != template[i]);
		// lol, https://gist.github.com/HoX/abfe15c40f2d9daebc35
		// (CCBYSA4.0)
		if (NULL != strstr(template[i], "%s")) {
			if (0 == strcmp(template[i+1], "memsz")) {
				argv[argc++] = strdup(conf->memsz);
			}
			else if (0 == strcmp(template[i+1], "imgfile")) {
				argv[argc++] = strdup(conf->imgfile);
			}
			else if (0 == strcmp(template[i+1], "kernel")) {
				argv[argc++] = strdup(conf->kernel);
			}
			else if (0 == strcmp(template[i+1], "initrd")) {
				argv[argc++] = strdup(conf->initrd);
			}
			else if (0 == strcmp(template[i+1], "append")) {
				argv[argc++] = strdup(conf->append);
			}
			++i;
		}

		else if (NULL != strstr(template[i], "%d")) {
			asprintf(&argv[argc], template[i], conf->portno);
			++argc;
		}

		else {
			argv[argc++] = strdup(template[i]);
		}
		++i;
	}

	return argv;
}
