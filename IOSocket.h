#ifndef __IOSOCKET_H
#define __IOSOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <sstream>
#include "IOSelect.h"

using namespace std;

#define IOSOCKET_NET_BUF_SIZE	4096

/**
 * Opaque handler for network
 * transfert. Just buffer content
 * is transfered, not the whole struct
 */
struct io_buf {
	char 	*content;
	size_t	length;
};

/**
 * Struct for keeping
 * track of connection statistics
 */
struct io_stat {
	time_t		startTime;
	time_t		endTime;
	long long	bytesSent;
	long long	bytesReceived;
};

/**
 * For Reading / Writing stats
 */
#define	OP_READ		0
#define	OP_WRITE	1

class IOSocket {

	private:
		int			sock;
		int			port;
		char		*host;
		bool		connected;
		bool		client;

		/* Select */
		IOSelect	*select;

		/* Remote */
		int			remote_port;
		char		remote_addr[16];

		fd_set		rfds;
		fd_set		wfds;

		/* For statistics */
		struct io_stat	stats;
		
		/* Methods */
		void connectToServer(const char *host, const int &port);
		void bindSocket(const int &port);

	public:
		IOSocket(const char *host, const int port, const int listen);

		/* Getter */
		int getFd(void);

		/* I/O */
		void write(const struct io_buf &buffer);
		void read(struct io_buf &buffer);
	
};

#endif
