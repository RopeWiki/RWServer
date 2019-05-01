//#include <types.h>
//#include <time.h>
//#include <stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
//#include <unistd.h>
#include <fcntl.h>
//#include <getopt.h>
#include <ws2tcpip.h>
#include "stdafx.h"


#define FCGI_LISTENSOCK_FILENO 0

#define UNIX_PATH_LEN 108

typedef unsigned short int sa_family_t;

struct sockaddr_un {
	sa_family_t	sun_family;              /* address family AF_LOCAL/AF_UNIX */
	char		sun_path[UNIX_PATH_LEN]; /* 108 bytes of socket address     */
};

/* Evaluates the actual length of `sockaddr_un' structure. */
#define SUN_LEN(p) ((size_t)(((struct sockaddr_un *) NULL)->sun_path) \
				   + strlen ((p)->sun_path))


int fcgi_spawn_connection(char *appPath, char **appArgv, char *addr, unsigned short port, const char *unixsocket, int fork_count, int child_count, int pid_fd, int nofork) {
	SOCKET fcgi_fd;
	int socket_type, rc = 0;

	struct sockaddr_un fcgi_addr_un;
	struct sockaddr_in fcgi_addr_in;
	struct sockaddr *fcgi_addr;

	socklen_t servlen;

    WORD  wVersion;
    WSADATA wsaData;

    if (child_count < 2) {
		child_count = 5;
	}

	if (child_count > 256) {
		child_count = 256;
	}


	if (unixsocket) {
		memset(&fcgi_addr, 0, sizeof(fcgi_addr));

		fcgi_addr_un.sun_family = AF_UNIX;
		strcpy(fcgi_addr_un.sun_path, unixsocket);

#ifdef SUN_LEN
		servlen = SUN_LEN(&fcgi_addr_un);
#else
		/* stevens says: */
		servlen = strlen(fcgi_addr_un.sun_path) + sizeof(fcgi_addr_un.sun_family);
#endif
		socket_type = AF_UNIX;
		fcgi_addr = (struct sockaddr *) &fcgi_addr_un;
	} else {
		fcgi_addr_in.sin_family = AF_INET;
                if (addr != NULL) {
                        fcgi_addr_in.sin_addr.s_addr = inet_addr(addr);
                } else {
                        fcgi_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
                }
		fcgi_addr_in.sin_port = htons(port);
		servlen = sizeof(fcgi_addr_in);

		socket_type = AF_INET;
		fcgi_addr = (struct sockaddr *) &fcgi_addr_in;
	}

    /*
     * Initialize windows sockets library.
     */
    wVersion = MAKEWORD(2,0);
    if (WSAStartup( wVersion, &wsaData )) {
		fprintf(stderr, "%s.%d: error %d starting Windows sockets\n",
			__FILE__, __LINE__, WSAGetLastError());
    }

	if (-1 == (fcgi_fd = socket(socket_type, SOCK_STREAM, 0))) {
		fprintf(stderr, "%s.%d\n",
			__FILE__, __LINE__);
		return -1;
	}

	if (-1 == connect(fcgi_fd, fcgi_addr, servlen)) {
		/* server is not up, spawn in  */
		int val;

		if (unixsocket) unlink(unixsocket);

		close(fcgi_fd);

		/* reopen socket */
		if (-1 == (fcgi_fd = socket(socket_type, SOCK_STREAM, 0))) {
			fprintf(stderr, "%s.%d\n",
				__FILE__, __LINE__);
			return -1;
		}

		val = 1;
		if (setsockopt(fcgi_fd, SOL_SOCKET, SO_REUSEADDR, 	(const char*)&val, sizeof(val)) < 0) {
			fprintf(stderr, "%s.%d\n",
				__FILE__, __LINE__);
			return -1;
		}

		/* create socket */
		if (-1 == bind(fcgi_fd, fcgi_addr, servlen)) {
			fprintf(stderr, "%s.%d: bind failed: %s\n",
				__FILE__, __LINE__,
				strerror(errno));
			return -1;
		}

		if (-1 == listen(fcgi_fd, 1024)) {
			fprintf(stderr, "%s.%d: fd = -1\n",
				__FILE__, __LINE__);
			return -1;
		}


		while (fork_count-- > 0) {

			PROCESS_INFORMATION pi;
			STARTUPINFO si;

			ZeroMemory(&si,sizeof(STARTUPINFO));
			si.cb = sizeof(STARTUPINFO);
			si.dwFlags = STARTF_USESTDHANDLES;
			si.hStdOutput = INVALID_HANDLE_VALUE;
			si.hStdInput  = (HANDLE)fcgi_fd;
			si.hStdError  = INVALID_HANDLE_VALUE;
			if (!CreateProcess(NULL,appPath,NULL,NULL,TRUE,	CREATE_NO_WINDOW,NULL,NULL,&si,&pi)) {
				fprintf(stderr, "%s.%d: CreateProcess failed\n",
					__FILE__, __LINE__);
				return -1;
			} else {
				fprintf(stdout, "%s.%d: child spawned successfully: PID: %lu\n",
					__FILE__, __LINE__,
					pi.dwProcessId);
				CloseHandle(pi.hThread);
			}

		}

	} else {
		fprintf(stderr, "%s.%d: socket is already used, can't spawn\n",
			__FILE__, __LINE__);
		return -1;
	}


	closesocket(fcgi_fd);

	return rc;
}


void show_version () {
	char *b = "spawn-fcgi-win32 - spawns fastcgi processes\n";
	write(1, b, strlen(b));
}

void show_help () {
	char *b =
"Usage: spawn-fcgi [options] -- <fcgiapp> [fcgi app arguments]\n"
"\n"
"spawn-fcgi-win32 - spawns fastcgi processes\n"
"\n"
"Options:\n"
" -f <fcgiapp> filename of the fcgi-application\n"
" -a <addr>    bind to ip address\n"
" -p <port>    bind to tcp-port\n"
" -s <path>    bind to unix-domain socket\n"
" -C <childs>  (PHP only) numbers of childs to spawn (default 5)\n"
" -F <childs>  numbers of childs to fork (default 1)\n"
" -P <path>    name of PID-file for spawed process\n"
" -n           no fork (for daemontools)\n"
" -v           show version\n"
" -h           show this help\n"
"(root only)\n"
" -c <dir>     chroot to directory\n"
" -u <user>    change to user-id\n"
" -g <group>   change to group-id\n"
;
	write(1, b, strlen(b));
}


int main(int argc, char **argv) {
	char *fcgi_app = NULL, *unixsocket = NULL, *pid_file = NULL,
                *addr = NULL;
	char **fcgi_app_argv = { NULL };
	unsigned short port = 0;
	int child_count = 5;
	int fork_count = 1;
	int pid_fd = -1;
	int nofork = 0;
	int o;
	struct sockaddr_un un;

	while (-1 != (o = getopt(argc, argv, "c:f:g:hna:p:u:vC:F:s:P:"))) {
		switch(o) {
		case 'f': fcgi_app = optarg; break;
		case 'a': addr = optarg;/* ip addr */ break;
		case 'p': port = strtol(optarg, NULL, 10);/* port */ break;
		case 'C': child_count = strtol(optarg, NULL, 10);/*  */ break;
		case 'F': fork_count = strtol(optarg, NULL, 10);/*  */ break;
		case 's': unixsocket = optarg; /* unix-domain socket */ break;
		case 'n': nofork = 1; break;
		case 'P': pid_file = optarg; /* PID file */ break;
		case 'v': show_version(); return 0;
		case 'h': show_help(); return 0;
		default:
			show_help();
			return -1;
		}
	}

	if (optind < argc) {
		fcgi_app_argv = &argv[optind];
	}

	if ((fcgi_app == NULL && fcgi_app_argv == NULL) || (port == 0 && unixsocket == NULL)) {
		show_help();
		return -1;
	}

	if (unixsocket && port) {
		fprintf(stderr, "%s.%d: %s\n",
			__FILE__, __LINE__,
			"either a unix domain socket or a tcp-port, but not both\n");

		return -1;
	}

	if (unixsocket && strlen(unixsocket) > sizeof(un.sun_path) - 1) {
		fprintf(stderr, "%s.%d: %s\n",
			__FILE__, __LINE__,
			"path of the unix socket is too long\n");

		return -1;
	}

	if (pid_file &&
	    (-1 == (pid_fd = open(pid_file, O_WRONLY | O_CREAT | O_EXCL | O_TRUNC)))) {
		struct stat st;
		if (errno != EEXIST) {
			fprintf(stderr, "%s.%d: opening pid-file '%s' failed: %s\n",
				__FILE__, __LINE__,
				pid_file, strerror(errno));

			return -1;
		}

		/* ok, file exists */

		if (0 != stat(pid_file, &st)) {
			fprintf(stderr, "%s.%d: stating pid-file '%s' failed: %s\n",
				__FILE__, __LINE__,
				pid_file, strerror(errno));

			return -1;
		}

		/* is it a regular file ? */

		if (!S_ISREG(st.st_mode)) {
			fprintf(stderr, "%s.%d: pid-file exists and isn't regular file: '%s'\n",
				__FILE__, __LINE__,
				pid_file);

			return -1;
		}

		if (-1 == (pid_fd = open(pid_file, O_WRONLY | O_CREAT | O_TRUNC))) {
			fprintf(stderr, "%s.%d: opening pid-file '%s' failed: %s\n",
				__FILE__, __LINE__,
				pid_file, strerror(errno));

			return -1;
		}
	}

    return fcgi_spawn_connection(fcgi_app, fcgi_app_argv, addr, port, unixsocket, fork_count, child_count, pid_fd, nofork);
}
