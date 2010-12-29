#include "IOSocket.h"


IOSocket::IOSocket(const char *host, const int port, const int listen) {

	int	fd[2] = {0, 0};

	/* Are we a client or a server ? */
	client = listen ? false : true;

	/* No timeout */
	timeouted = 0;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		throw("Socket creation error");

	if (client)
		connectToServer(host, port);
	else
		bindSocket(port);

	/* Setup select */
	fd[0] = (int) sock;
	fd[1] = -1;
	select = new IOSelect(fd);

	/* Reset stats */
	memset(&stats, 0x0, sizeof(stats));
}

/**
 * @desc Getter for Socket file descriptor
 * @return int socket
 */
int IOSocket::getFd(void) {
	return sock;
}

void IOSocket::bindSocket(const int &port) {

	struct sockaddr_in 	sin = {0};
	int					rc = 0;
	int					reuse_addr = 1;

	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	rc = bind((int) sock, (struct sockaddr *)&sin, sizeof(sin));
	if (rc < 0)
		throw("Unable to bind local sock");

	/* Set reusable */
	rc = setsockopt((int) sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse_addr, sizeof(reuse_addr));
	if (rc < 0)
		throw("Unable to set REUSEADDR");

}

void IOSocket::connectToServer(const char *hostname, const int &port) {

	struct sockaddr_in	sin = {0};
	struct hostent		*hp;
	int					rc = 0;

	hp = gethostbyname(hostname);
	if (hp == NULL) 
		throw("Hostname resolution failed");

	memcpy(&(sin.sin_addr.s_addr), hp->h_addr, hp->h_length);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

again:
	rc = connect((int) sock, (struct sockaddr *) &sin, sizeof(sin));
	if (rc < 0) {
		if (errno != EINTR)
			throw("Unable to connect to host");
		goto again;
	}

	/* Store start Time */
	stats.startTime = time(NULL);
}

void IOSocket::write(const struct io_buf &buffer) {

	int		written = 0;
	size_t	offset = 0;
	size_t	to_write = buffer.length;

	if (to_write > IOSOCKET_NET_BUF_SIZE)
		to_write = IOSOCKET_NET_BUF_SIZE;

	do {
		written = ::write((int) sock, ((char *) buffer.content) + offset, to_write);
		if (written < 0) {
			if (errno != EINTR)
				throw("Write error");
			continue;
		}

		offset += written;

		if (buffer.length - offset < IOSOCKET_NET_BUF_SIZE)
			to_write = buffer.length - offset;

	} while (offset != buffer.length);
}

void IOSocket::write(const struct io_buf &buffer, const struct timeval *timeout) {

	list<int>	writable;

	try {
		writable = select->can_write(timeout);
	} catch(IOSelectTimeout &t) {
		if (timeouted++)
			throw ios_base::failure("Write operation timeouted");
	}

	return write(buffer);
}

void IOSocket::read(struct io_buf *buffer) {

	int		has_read = 0;

	do {
		has_read = ::read(sock, (char *) buffer->content, IOSOCKET_NET_BUF_SIZE);
		if (has_read < 0) {
			if (errno != EINTR)
				throw("Read error");
			continue;
		}
		break;
	} while (1);

	buffer->length = has_read;
}

void IOSocket::read(struct io_buf *buffer, const struct timeval *timeout) {

	list<int>	readable;

	try {
		readable = select->can_read(timeout);
	} catch(IOSelectTimeout &t) {
		if (timeouted++)
			throw ios_base::failure("Read operation timeouted");
	}

	return read(buffer);
}


