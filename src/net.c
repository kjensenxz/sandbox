#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "sandbox.h"
#include "net.h"

#define BACKLOG 10

/* get sockaddr, IPv4 or IPv6. thanks, Beej! */
void *
net_getinaddr(struct sockaddr *sa) {
	assert(sa->sa_family == AF_INET || sa->sa_family == AF_INET6);

	if (AF_INET == sa->sa_family) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

/* bind to, listen on, and return stream socket or -1 on failure. */
int
net_sslisten(char *host, char *service) {
	int ssocket = -1;
	struct addrinfo hints = {0}, *svinfo = NULL, *p = NULL;
	int yes = 1;
	int rv = -1;

	assert(NULL != host);
	assert(NULL != service);
	DEBUG("entered net_sslisten(%s, %s)", host, service);

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	rv = getaddrinfo(NULL, service, &hints, &svinfo);
	if (0 != rv) {
		FATAL("net_sslisten(): getaddrinfo() failed: %s",
		      gai_strerror(rv)
		);
		return -1;
	}

	for (p = svinfo; NULL != p; p = p->ai_next) {
		ssocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (0 > ssocket) {
			WARN("net_sslisten(): socket() failed: %s",
				strerror(errno)
			);
			continue;
		}
		rv = setsockopt(ssocket, SOL_SOCKET, SO_REUSEADDR, 
		                &yes, sizeof(yes)
		);

		if (0 > rv) {
			close(ssocket);
			WARN("net_sslisten(): setsockopt() failed: %s",
			        strerror(errno)
			);
			continue;
		}

		rv = bind(ssocket, p->ai_addr, p->ai_addrlen);
		if (0 > rv) {
			close(ssocket);
			WARN("net_sslisten(): bind() failed: %s",
				strerror(errno)
			);
			continue;
		}

		break;
	}
	if (NULL == p) {
		freeaddrinfo(svinfo);
		FATAL("n_sslisten(): failed to bind to %s:%s: %s",
		        host, service, strerror(errno)
		);
		return -1;
	}

	rv = listen(ssocket, BACKLOG);
	if (0 > rv) {
		freeaddrinfo(svinfo);
		FATAL("n_sslisten(): failed to listen on %s:%s. %s",
		        host, service, strerror(errno)
		);
		return -1;
	}

	freeaddrinfo(svinfo);
	return ssocket;
}

int
net_accept(int ssock) {
	struct sockaddr_storage clnt_addr = {0};
	socklen_t addrlen = {0};

	int csock = -1;
	char clntIP[INET6_ADDRSTRLEN] = {0};

	DEBUG("entered net_accept(%d)", ssock);

	assert(0 <= ssock);

	addrlen = sizeof(clnt_addr);
	csock = accept(ssock, (struct sockaddr *)&clnt_addr, &addrlen);
	
	if (0 > csock) {
		ERR("net_accept(): accept() returned %d: %s", csock,
		    strerror(errno)
		);
		return -1;
	}

	INFO("net_accept(): new connection from %s, fd %d", 
	     inet_ntop(clnt_addr.ss_family,
                       net_getinaddr((struct sockaddr*)&clnt_addr),
                       clntIP, INET6_ADDRSTRLEN
	     ), csock
	);

	return csock;
}
