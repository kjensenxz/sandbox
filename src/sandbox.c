#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>

#include "sandbox.h"
//#include "config.h"
#include "net.h"
#include "str.h"
//#include "requests.h"


// XXX: should probably be a unix socket defined by a config file somewhere... 
#define CMDPORT "1337"

int s_broker(int);


static inline void 
add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size) {
    // If we don't have room, add more space in the pfds array
    if (*fd_count == *fd_size) {
        *fd_size *= 2; // Double it

        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

    (*fd_count)++;
}
static inline void
del_from_pfds(struct pollfd pfds[], int i, int *fd_count) {
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count-1];

    (*fd_count)--;
}

void func(int signum)
{
    wait(NULL);
}
#define lnwrite(FD, STR) write(FD, STR, strlen(STR))
#define clntsnd(STR) lnwrite(csock, STR "\n")

/* s_broker() calls functions and does I/O with the client. */
int
s_broker(int csock) {
	char *buf = NULL;
	char *sv = NULL;
	int rv = -1;

	buf = calloc(64, sizeof(char));
	if (NULL == buf) {
		return -1;
	}
	rv = recv(csock, buf, 63, 0);
	if (0 > rv) {
		ERR("s_broker(): recv() returned %d: %s", rv, strerror(errno));
		free(buf);
		return -1;
	}
	else if (0 == rv) {
		WARN("s_broker(): client disconnected.");
		free(buf);
		return -1;
	}

	int i = -1;
	char **toks = str_tokenize(buf);

	if (NULL == toks) {
		ERR("s_broker(): str_tokenize() failed: %s", strerror(errno));
		return -1;
	}

	free(buf);
	buf = r_handle(toks);

	if (buf == NULL) {
		clntsnd("GOODBYE")
		ERR("s_broker(): r_handle() failed: %s", strerror(errno));
		free(toks);
		return -1;
	}
	
	free(buf);
	free(toks);


	sleep(10);
	INFO("Leaving s_broker()");
	return 0;
}

int
s_loop(int ssock) {
	int i = -1;
	int rv = -1;

	int csock = -1;

	int fd_count = 1;
	int fd_size = 5;
	int num_events = -1;
	struct pollfd *pfds = NULL;
	pfds = calloc(fd_size, sizeof(struct pollfd *));

	/* we need to poll the listening socket too */
	pfds[0].fd = ssock;
	pfds[0].events = POLLIN;
	fd_count = 1;
	
	while (num_events = poll(pfds, fd_count, -1)) {
		DEBUG("main(): poll(): %d events", num_events);

		if (0 >= num_events) {
			ERR("main(): poll() returned %d: %s", num_events,
			    strerror(errno));
			continue;
		}
		for (i = 0; i < fd_count; ++i) {
			DEBUG("main(): checking event # %d", i);
			if (!(pfds[i].revents & POLLIN)) {
				/* nothing to see here, folks, move along */
				assert(1);
				continue;
			}

			/* new connection */
			if (ssock == pfds[i].fd) {
				csock = net_accept(ssock);
				DEBUG("main(): new fd %d", csock);
				add_to_pfds(&pfds, csock, &fd_count, &fd_size);
				continue;
			}

			/* client has data */
			assert(0 < i);
			csock = pfds[i].fd;
			DEBUG("%d main(): data: fd %d",getpid(), csock);
			switch (rv = fork()) {
				case -1:
					die("main(): bad fork!");
				case 0: // child
					free(pfds);

					rv = s_broker(csock);

					INFO("main(): child %d exiting"
					" with %d.", getpid(), rv);

					close(csock);
					exit(rv);
				default: // parent
					INFO("main(): child %d started.", rv);
					signal(SIGCHLD, func);

					DEBUG("closing fd %d", csock);
					close(csock);
					del_from_pfds(pfds, i, &fd_count);

					break;
			}
			
		}
	}
	free(pfds);
	return EXIT_SUCCESS;
}

int
main(int argc, char **argv) {
	int rv = -1;
	int ssock = -1;

	INFO("sandboxd started. PID %d.", getpid());

	ssock = net_sslisten("localhost", CMDPORT);
	if (0 > ssock) {
		die("main(): n_sslisten() failed to listen.");
		exit(EXIT_FAILURE);
	}
	DEBUG("main(): n_sslisten() returned fd %d", ssock);

	// TODO: print listening socket
	

	rv = s_loop(ssock);


	return EXIT_SUCCESS;
}
