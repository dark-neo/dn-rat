
/*
 * Copyright (c) 2014 dark_neo
 * All rights reserved.
 *
 * Source code released under BSD license.
 * See LICENSE file for more details.
 */

/*
 * Client program.
 */

/* Standard C headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#include <netinet/in.h>
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
			fprintf(stdout, text, __VA_ARGS__);
#endif

#if defined (_WIN32)
#define clrscr() \
	system("cls");
#else
#define clrscr() \
	system("clear");
#endif

#define ERR_MEMORY_ALLOCATION		-1
#define ERR_MEMORY_REALLOCATION		-2
#define ERR_SOCKET_INIT			-3
#define ERR_CONNECTION			-4
#define ERR_SEND			-5
#define ERR_RECV			-6
#define ERR_FPUTS			-7
#define ERR_READ			-8
#define ERR_NO_NULLWRITE_ALLOWED	-9
#define ERR_WRITE			-10

#define BUFLEN				1024
#define MAX_IP_LEN			15+1	/* store '\0' */
#define MAX_PORT_LEN			4+1	/* store '\0' */
#define DEFAULT_SERVER_PORT		6666

/*
 * argv[1] == SERVER_IP
 * argv[2] == SERVER_PORT
 */
int
main(int argc, char *argv[])
{
	int		socket_ret = 0;	/* Store socket() return  */
	uint_t		input = 0;	/* Store input key char   */
	uint_t		i = 0;	/* Byte counter */
	ssize_t		serverreply_sz = 0;
	ssize_t		send_ret = 0;	/* Return value of send() */
	ssize_t		recv_ret = 0;	/* Return value of recv() */
	char		server_ip [MAX_IP_LEN] = "127.0.0.1";
	uint_t		server_port = 0;
	char		server_reply[BUFLEN] = {0};
	char           *buffer = NULL;	/* Merge inputted chars   */
	struct sockaddr_in serv_addr;	/* Socket structure       */
	unsigned char	arg_i = 0;

	/* Cygwin IS REQUIRED to build source on MS-Window$ */

#ifdef  NO_CYGWIN_ON_WINDOWS
	debug_print("\***YOU MUST USE Cygwin TO BUILD SOURCE***\n");
	return (EXIT_FAILURE);
#endif

#ifdef NON_UNIX_REQUIRED_HEADERS
	debug_print("\n***ONLY UNIX-like OS ARE SUPPORTED***\n");
	return (EXIT_FAILURE);
#endif

	/* Check multi-arch feature */

#if defined(MULTIARCH_DEBUG) && defined(DEBUG)
	debug_print("sizeof uint_t: %u\n", sizeof(uint_t));
	debug_print("sizeof void*: %u\n", sizeof(void *));
#endif

	/* Check command line arguments */

#ifdef CHECK_ARGS
	printf("argc value: %d\n", argc);
	for (arg_i = 0; arg_i < argc; arg_i++)
		debug_print("argv[%d] value: %s\n", arg_i, argv[arg_i]);
#endif

#ifdef DEBUG
	printf("argc: %d\n", argc);
#endif

	/*
	 * Server IP is given from args.
	 * Server PORT can be given from args.
	 */
	if ((argc == 1 || argc > 3) || strlen(argv[1]) == 0) {
		fprintf(stderr, "\nSyntax:\n\t ratclient SERVER_IP "
			"[SERVER_PORT]\n\n");
		return (EXIT_FAILURE);
	} else if (argc == 3 && strlen(argv[2]) != 0)
		server_port = atoi(argv[2]);
	else
		server_port = DEFAULT_SERVER_PORT;

	/*
	 * IP format:
	 * 	255.255.255.254	--> 15 characters length
	 *
	 * Minimal given IP format:
	 * 	127.0.0.1	--> 9 characters length
	 */
	if (strlen(argv[1]) < 9) {
		fprintf(stderr, "\nIP FORMAT ERROR: XXX.[XX]X.[XX]X.[XX]X\n\n");
		return (EXIT_FAILURE);
	}
	if (strcpy(server_ip, argv[1]) == NULL) {
		fprintf(stderr, "\nError copying server IP."
			"End of program.\n");
		return (EXIT_FAILURE);
	}
	clrscr();
	printf("\n");

#ifdef DEBUG
	debug_print("Server IP address args: %s\n", server_ip);
	debug_print("Server IP port args: %s\n", argv[1]);
	debug_print("Server IP port int convert: %d\n\n", server_port);
#endif

	/* Allocate 1 byte to buffer and initialize buffer to zeroes. */
	buffer = (char *)calloc(1, sizeof(char));
	if (buffer == NULL) {
		fprintf(stderr, "Error allocating memory to 'buffer': %d\n",
			ERR_MEMORY_ALLOCATION);
		return (ERR_MEMORY_ALLOCATION);
	}
	/*
	 * ABOUT memset FUNCTION
	 * ---------------------
	 * It a function-like way to fill any buffer a.k.a array to zeroes
	 * without use any for-loop form. memset equals to:
	 *	for (i = 0; i < sizeof(buffer)/sizeof(buffer_type); i++)
	 *		buffer[i] = 0;
	 */
	/* fill _server_reply_ with zeroes */
	memset(server_reply, 0, sizeof(server_reply));

	/* Create socket */
	if ((socket_ret = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "\nError creating socket: %d\n",
			ERR_SOCKET_INIT);
		return (ERR_SOCKET_INIT);
	}
	/*
	 * Set sockaddr_in fields
	 *
	 * FIELD                 DESCRIPTION
	 * =================================
	 * sin_family            Domain (See `man 2 socket` for reference)
	 * sin_addr.s_addr       Internet address (IP)
	 * sin_port              Port number
	 */
	serv_addr.sin_family = AF_INET;	/* IPv4 protocol  */
	serv_addr.sin_addr.s_addr = inet_addr(server_ip);
	serv_addr.sin_port = htons(server_port);

	/* Start server connection */
	if (connect(socket_ret, (struct sockaddr *)&serv_addr,
		    sizeof(serv_addr)) < 0) {
		fprintf(stderr, "\nError while stablish connection: %d\n",
			ERR_CONNECTION);
		return (ERR_CONNECTION);
	}
	/* Buffer filling */

#ifdef DEBUG
	debug_print("sizeof(buffer) before keyboard input: %lu\n",
		    sizeof(buffer));
#endif

	/* Read server connection message */
	serverreply_sz = read(socket_ret, server_reply,
			      sizeof(server_reply) - 1);
	if (serverreply_sz > 0) {

#ifdef DEBUG
		debug_print("serverreply_sz: %lu\n\n", serverreply_sz);
#endif

		server_reply[serverreply_sz] = 0;
		printf("Server IP and port:\n\t");
		if (fputs(server_reply, stdout) == EOF) {
			fprintf(stderr, "\nfputs() error: %d\n", ERR_FPUTS);
			return (ERR_FPUTS);
		}
	} else {
		fprintf(stderr, "\nRead error: %d\n", ERR_READ);
		return (ERR_READ);
	}

	printf("\n\nEnter the command to send the server:\n");
	printf("Type 'quit' to end communication.\n");
	printf("Type 'shutdown-server' to kill remote server.\n\n");
	while (1) {
		printf("> ");

		i = 0;
		while ((input = getchar()) != 10) {
			/* FIRST RE-ALLOCATE, THEN STORE */
			i++;
			buffer = (char *)realloc(buffer, sizeof(char) * i);
			if (buffer == NULL) {
				fprintf(stderr, "Error re-allocating memory on "
					"buffer: %d\n", ERR_MEMORY_ALLOCATION);
				return (ERR_MEMORY_REALLOCATION);
			}
			/* Add the byte to PREVIOUS index */
			buffer[i - 1] = input;
		}

		buffer[i] = '\0';

#ifdef DEBUG
		if (buffer != NULL) {
			debug_print("\nbuffer value: %s\n", buffer);
			debug_print("sizeof(buffer): %lu\n",
				    sizeof(buffer));
			debug_print("sizeof(char*): %lu\n",
				    sizeof(char *));
			/**
			 * WROOOOOONG!!!!!!!!! char * MUST use strlen()
			 * function to get the length
			 */
			/*
			debug_print("length of buffer: %lu\n",
			sizeof(buffer)/sizeof(buffer[0]));
			*/
			debug_print("length of buffer: %lu\n\n",
				    strlen(buffer));
		}
#endif

		/* Break loop and free resources */
		if (strcmp(buffer, "quit") == 0)
			break;

		/* Send data to server */
		send_ret = send(socket_ret, buffer, BUFLEN, 0);
		if (send_ret < 0) {
			fprintf(stderr, "Error send data to server: %d\n",
				ERR_SEND);
			return (ERR_SEND);
		}
		/* Fill up input buffer with zero */
		memset(buffer, 0, strlen(buffer));

		/* Receive data from server */
		memset(server_reply, 0, sizeof(server_reply));
		recv_ret = recv(socket_ret, server_reply, sizeof(server_reply),
				0);
		if (recv_ret < 0) {
			fprintf(stderr, "\nServer reply failed: %d\n",
				ERR_RECV);
			return (ERR_RECV);
		} else {
			server_reply[recv_ret] = '\0';
			printf("\n---Server response---\n%s\n\n", server_reply);
		}
		/* END_OF_WHILE */
	}

	/* Close current connection */
	close(socket_ret);

	if (buffer != NULL) {
		free(buffer);
		buffer = NULL;
	}
	printf("\n");

	return (EXIT_SUCCESS);
}
