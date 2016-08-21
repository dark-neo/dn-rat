
/*
 * Copyright (c) 2014 dark_neo
 * All rights reserved.
 *
 * Source code released under BSD license.
 * See LICENSE file for more details.
 */

/*
 * Server program.
 */

/* Standard C headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>		/* tolower/toupper functions */

#if defined(_WIN32)
#define clrscr() \
	system("cls");
#else
#define clrscr() \
	system("clear");
#endif

/* MULTI-ARCH */
/* Enable debug at compiling time with: -DMULTIARCH_DEBUG flag */
#ifdef __x86_64__
#define uint_t unsigned long long
#elif __i386__
#define uint_t unsigned int
#endif

/* UNIX-like required headers */
#if (!defined(_WIN32) || !defined(_WIN64)) || defined(__CYGWIN__)
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>		/* INADDR_* constants */
#include <sys/socket.h>
#include <sys/types.h>
#else
#define NON_UNIX_REQUIRED_HEADERS
#endif

/* -DDEBUG / -DCHECK_ARGS flag */

#if defined(DEBUG) || defined(CHECK_ARGS)
#define DEBUG_TEST 1
#define debug_print(text, ...) \
		if (DEBUG_TEST) \
			fprintf(stderr, text, __VA_ARGS__);
#endif

#define MAX_CONNECTIONS			 1
#define ERR_MEMORY_ALLOCATION		-1
#define ERR_SOCKET_INIT			-2
#define ERR_LISTEN			-3
#define ERR_ACCEPT_CONNECTION		-4
#define ERR_NO_NULLWRITE_ALLOWED	-5
#define ERR_WRITE			-6
#define ERR_READ			-7
#define ERR_SEND			-8
#define ERR_RECV			-9
#define ERR_BUFFER_COPY			-10
#define ERR_SRV_MSG_COPY		-11
#define ERR_POPEN			-12
#define ERR_FGETS			-13

#define BUFLEN				1024
#define MAX_PORT_LEN			4+1	/* extra byte store '\0' */
#define DEFAULT_PORT			"6666"

/*
 * argv[1] == SERVER_PORT
 */
int
main(int argc, char *argv[])
{
	int		socket_ret = 0;	/* Store socket() return */
	int		accept_ret = 0;	/* Store accept() return */
	ssize_t		size = 0;
	int		listen_ret = 0;	/* Store listen() return */
	char           *send_buff = NULL;	/* Buffer sent to client */
	char           *server_dir[BUFLEN];	/* Server address-and-port */
	char           *command = NULL;	/* Message from client */
	char           *shutdown_msg = "\n***Server shutdown successful***\n";
	struct sockaddr_in serv_addr;	/* Server socket type */
	/* Store IPv4 address of server */
	char		ipstr     [INET_ADDRSTRLEN] = {0};
	char		server_port[MAX_PORT_LEN] = {0};
	unsigned short	serv_port = 0;
	unsigned char	arg_i;
	unsigned char	op;	/* server operation flag */
	FILE           *fstdout = NULL;
	char		buftmp    [BUFLEN] = {0};
	char		cmdoutbuf [BUFLEN] = {0};
	uint_t		i;

	/* Cygwin IS REQUIRED to build source on MS-Window$ */

#ifdef NO_CYGWIN_ON_WINDOWS
	debug_print("\n***YOU MUST USE Cygwin TO BUILD SOURCE***\n");
	return (EXIT_FAILURE);
#endif

#ifdef NON_UNIX_REQUIRED_HEADERS
	debug_print("\n***ONLY UNIX-like OS ARE SUPPORTED***\n")
		return (EXIT_FAILURE);
#endif

	/* Check multi-arch feature */

#if defined(MULTIARCH_DEBUG) && defined(DEBUG)
	debug_print("sizeof uint_t: %u\n", sizeof(uint_t));
	debug_print("sizeof void*: %u\n", sizeof(void *));
#endif

	/* Check command line arguments */

#ifdef CHECK_ARGS
	for (arg_i = 0; arg_i < argc; arg_i++)
		debug_print("Argument %d value: %s\n", arg_i,
			    argv[arg_i]);
#endif

	/* If PORT is not given by args, the PORT will be 6666. */
	if (argc != 2 || strlen(argv[1]) == 0) {
		strcpy(server_port, DEFAULT_PORT);
		/*
		fprintf(stderr, "\nSyntax:\n\t ratserver PORT\n\n");
		return (EXIT_FAILURE);
		*/
	} else if (strcpy(server_port, argv[1]) == NULL) {
		fprintf(stderr, "\nError copying server port. "
			"End of program.\n");
		return (EXIT_FAILURE);
	}
	clrscr();

	serv_port = atoi(server_port);

	/* Allocate memory to sender buffer */
	send_buff = (char *)malloc(sizeof(char) * BUFLEN);
	if (send_buff == NULL) {
		fprintf(stderr, "Error allocating memory to _send_buff_: %d\n",
			ERR_MEMORY_ALLOCATION);
		return ERR_MEMORY_ALLOCATION;
	}
	/*
	 * Allocate memory to command. Do I need pass the buffer-length to
	 * server?
	 */
	/* Not optimized? */
	command = (char *)malloc(sizeof(char) * BUFLEN);
	if (command == NULL) {
		fprintf(stderr, "Error allocating memory to _command_: "
			"%d\n", ERR_MEMORY_ALLOCATION);
		return ERR_MEMORY_ALLOCATION;
	}
	/* Create socket */
	socket_ret = socket(PF_INET, SOCK_STREAM, 0);
	if (socket_ret < 0) {
		fprintf(stderr, "Socket initialization error: %d\n",
			ERR_SOCKET_INIT);
		return ERR_SOCKET_INIT;
	}
	/* Fill serv_addr and send_buff with zeroes */
	memset(&serv_addr, 0, (sizeof(serv_addr) / sizeof(struct sockaddr_in)));
	memset(send_buff, 0, (sizeof(send_buff) / sizeof(char *)));

	/*
	 * Set sockaddr_in fields
	 *
	 * FIELD               DESCRIPTION
	 * ===============================
	 * sin_family          Domain (See `man 2 socket` to complete reference)
	 * sin_addr.s_addr     Internet address (IP)
	 * sin_port            Port number (unsigned short (uint16_t))
	 */
	serv_addr.sin_family = AF_INET;	/* IPv4 protocol    */
	/*
	 * htonl() parameters:
	 *
	 * INADDR_ANY
	 * It is used when you don't need to bind a socket to a specific IP.
	 * Using this value as the address when calling bind(), the socket
	 * accept connections to all the IPs of the machine.
	 *
	 * INADDR_LOOPBACK
	 * It is used when you want to bind a socket to local IP (127.0.0.1).
	 */
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(serv_port);

	/* Server start to listen */
	bind(socket_ret, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

	listen_ret = listen(socket_ret, MAX_CONNECTIONS);
	if (listen_ret < 0) {
		fprintf(stderr, "Failed to listen: %d\n", ERR_LISTEN);
		return (ERR_LISTEN);
	}
	/*
	 * inet_top() is thread-safe and extends the inet_ntoa() function to
	 * support multiple address families. inet_ntoa() is deprecated.
	 *
	 * PARAMETERS: 1st: address family (int) 2nd: address structure (void*)
	 * copied to string buffer 3nd: buffer (char*) 4th: buffer length
	 * (socklen_t)
	 *
	 * I use serv_addr.sin_addr to get server IP.
	 */
	inet_ntop(AF_INET, &serv_addr.sin_addr, ipstr, INET_ADDRSTRLEN);

#ifdef DEBUG
	if (ipstr != NULL)
		printf("\nServer listen at %s:%s\n", ipstr, server_port);
#endif

	strcat(server_dir, ipstr);
	strcat(server_dir, ":");
	strcat(server_dir, server_port);

	/*
	 * Server thread.
	 * Server will loop forever until "shutdown-server" is sent from client.
	 */
	while (1) {
		accept_ret = accept(socket_ret, (struct sockaddr *)NULL, NULL);
		if (accept_ret < 0) {
			fprintf(stderr, "Accepted connection failed: %d\n",
				ERR_ACCEPT_CONNECTION);
			return (ERR_ACCEPT_CONNECTION);
		}
		/* Accept awaiting request. */

#ifdef DEBUG
		printf("\n***Client connected***\n\n");
#endif

		size = send(accept_ret, server_dir, strlen(server_dir), 0);
		if (size < 0) {
			fprintf(stderr, "Send error: %d\n", ERR_SEND);
			return (ERR_SEND);
		}
		op = 0;
		do {
			/* Initialize _buftmp_ with zeroes */
			memset(buftmp, 0, BUFLEN);

			/* Initialize _command_ with zeroes */
			memset(command, 0, BUFLEN);

			/* Initialize _cmdoutbuff_ with zeroes */
			memset(cmdoutbuf, 0, BUFLEN);

			/*
			 * Receive data from client.
			 *
		 	 * ssize_t recv(int sockfd, void *buf, size_t len,
			 *		int flags);
		 	 *
		 	 * sockfd  = socket
		 	 * buf     = data to receive from client
		 	 * len     = buffer length. Use the exact bytes that
		 	 *	     you use to allocate the buffer pointer.
		 	 * flags   = see `man recv` to get a full description
			 *	     of flags.
		 	 *
		 	 * Return value:
		 	 * Number of bytes received or -1 if an error ocurred.
			 * In the event of an error, _errno_ is set to indicate
		 	 * the error. The return value will be 0 when the peer
		 	 * has performed an orderly shutdown.
			 */
			size = recv(accept_ret, buftmp, BUFLEN, 0);
			if (size <= 0) {
				if (size < 0) {
					fprintf(stderr, "\nError receiving data"
					      " from client: %d\n", ERR_RECV);
					return (ERR_RECV);
				}
				op = 1;
			}
			/* convert message to lowercase */
			for (i = 0; i < BUFLEN; i++)
				command[i] = (char)tolower(buftmp[i]);

#ifdef DEBUG
			if (command != NULL) {
				debug_print("\nbuftmp value: %s\n", buftmp);
				debug_print("command value: %s\n", command);
				debug_print("command length: %d\n",
					strlen(command));
			}
#endif

			if (strcmp(command, "shutdown-server") == 0) {
				size = send(accept_ret, shutdown_msg,
					    strlen(shutdown_msg), 0);
				if (size < 0) {
					fprintf(stderr, "\nError sending "
						"server message to client: "
						"%d\n", ERR_SEND);
					return (ERR_SEND);
				}
				return (EXIT_SUCCESS);
			}

#ifdef DEBUG
			printf("\n");
#endif

			fstdout = popen(command, "r");
			if (fstdout == NULL) {
				fprintf(stderr, "\nError calling system "
					"command: %d\n", ERR_POPEN);
				/*
				return (ERR_POPEN);
				*/
			} else {
				while (!feof(fstdout)) {
					if (fgets(cmdoutbuf,
						  sizeof(cmdoutbuf) - 1,
						  fstdout) != NULL);

					/*
				 	 * Send data to client.
				 	 *
		 	 	 	 * ssize_t send(int sockfd, const void *buf, size_t len,
					 *				int flags);
		 	 	 	 *
		 	 	 	 * sockfd  = socket
		 	 	 	 * buf     = buffer to send
		 	 	 	 * len     = buffer length.
		 	 	 	 * flags   = see `man send` to get a full description of
				 	 *           flags.
				 	 *
		 	 	 	 * Return value:
		 	 	 	 * On success, return the number of character sent. On
		 	 	 	 * On error, -1 is returned and _errno_ is set 
					 * appropriately.
		 	 	 	 */
					size = send(accept_ret, cmdoutbuf, strlen(cmdoutbuf), 0);
					if (size < 0) {
						fprintf(stderr, "\nError sending command output to "
							"client: %d\n", ERR_SEND);
						return (ERR_SEND);
					}
				}

				if (fstdout != NULL) {
					fclose(fstdout);
					fstdout = NULL;
				}
			}
		}
		while (!op);

		close(accept_ret);
		sleep(1);
	}

	if (command != NULL) {
		free(command);
		command = NULL;
	}
	if (send_buff != NULL) {
		free(send_buff);
		send_buff = NULL;
	}
	return (EXIT_SUCCESS);
}
