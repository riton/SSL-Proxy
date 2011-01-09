#include "IOSocket.h"

/**
 * @desc Private constructor for clients socket
 * This constructor is used by the accept() method
 * to create a new client socket
 * @param const int socket Client socket file descriptor
 * @return IOSocket *
 */
IOSocket::IOSocket(const int &socket) {

	socket_t = IOSOCKET_CONNECT_T;
	sock = socket;
	connected = true;

	/* Reset stats */
	memset(&stats, 0x0, sizeof(stats));

	/* Set start time */
	stats.client.startTime = time(NULL);
}

IOSocket *IOSocket::accept() {

	int 				c_sock = 0;
	struct sockaddr		addr;
	socklen_t			addrlen = sizeof(addr);

again:
	c_sock = ::accept(sock, &addr, &addrlen);
	if (c_sock < 0) {
		if (errno != EINTR)
			throw("accept error");
		goto again;
	}

	return new IOSocket(c_sock);
}

IOSocket::IOSocket(const socket_type sock_t, const char *host, const int port) {

	socket_t = sock_t;
	this->port = port;
	memset(&stats, 0x0, sizeof(stats));

	sock = ::socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		throw("Socket creation error");

	switch (socket_t) {
		case IOSOCKET_CONNECT_T:
			connectToServer(host, port);
			break;
		case IOSOCKET_LISTEN_T:
			bindSocket(port);
			break;
		default:
			throw("Invalid socket type");
	}
}

/**
 * @desc Getter for Socket file descriptor
 * @return int socket
 */
int IOSocket::getFd(void) {
	return sock;
}

void IOSocket::bindSocket(const int &port) {

	struct sockaddr_in 	sin;
	int					rc = 0;
	int					reuse_addr = 1;

	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	rc = bind(sock, (struct sockaddr *)&sin, sizeof(sin));
	if (rc < 0)
		throw("bind error");

	/* Set reusable */
	rc = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse_addr, sizeof(reuse_addr));
	if (rc < 0)
		throw("setsockopt error");

	rc = listen(sock, 5);
	if (rc < 0)
		throw("listen error");

	connected = true;
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
	stats.client.startTime = time(NULL);
	connected = true;
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
		stats.client.bytesSent += written; /* Update stats */

		if (buffer.length - offset < IOSOCKET_NET_BUF_SIZE)
			to_write = buffer.length - offset;

	} while (offset != buffer.length);
}

void IOSocket::write(const char *msg) {

	struct io_buf buffer;
	strncpy(buffer.content, msg, ::strlen(msg));
	buffer.length = ::strlen(msg);
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
	stats.client.bytesReceived += has_read; /* Update stats */
}

void IOSocket::close() {
	if (connected) {
		::shutdown(SHUT_RDWR, sock);
		::close(sock);
		stats.client.endTime = time(NULL); /* Set endTime */
	}
}

