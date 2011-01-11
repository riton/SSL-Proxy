#include "IOSocketSSL.h"

#define SSL_S_ERROR() ERR_error_string(ERR_get_error(), NULL)

/**
 * @desc Return a well formated error string with SSL last error
 * @param msg Action we were doing when error occurs
 */
const char *IOSocketSSL::new_SSL_error(const char *msg) {

	TRACE_CALL();

	memset(ssl_err, 0x0, __IOSOCKETSSL_ERR_BUF_LEN);
	snprintf(ssl_err, __IOSOCKETSSL_ERR_BUF_LEN-1, "%s: %s", msg, SSL_S_ERROR());
	ssl_err[__IOSOCKETSSL_ERR_BUF_LEN-1] = '\0';

	return ssl_err;
}

void IOSocketSSL::init_internals(const bool force_server_method = false) {

	TRACE_CALL();

	ssl_err = new char[__IOSOCKETSSL_ERR_BUF_LEN];
	handshake_done = false;

	//@throw char *e 
	initSSL(force_server_method);
}

/**
 * @throw Const char *e
 * @param sock_t Socket Type
 * @param host Host to connect/bind to
 * @param port Port to connect/bind to
 * @param keyfile Keyfile to read private key from
 * @param certfile File to read certificate from
 */
IOSocketSSL::IOSocketSSL(	const socket_type sock_t, 
							const char *host,
							const int port,
							const char *keyfile,
							const char *certfile) : IOSocket(sock_t, host, port)
{
	TRACE_CALL();

	setKeyFile(keyfile);
	setCertFile(certfile);

	init_internals();
}

void IOSocketSSL::setKeyFile(const char *k) {

	size_t	len = 0;

	TRACE_CALL();

	len = sizeof(key_file);
	strncpy(key_file, k, len - 1);
	key_file[len-1] = '\0';
}

void IOSocketSSL::setCertFile(const char *k) {

	size_t	len = 0;

	TRACE_CALL();

	len = sizeof(cert_file);
	strncpy(cert_file, k, len - 1);
	cert_file[len-1] = '\0';
}

/**
 * @desc Private constructor that uses IOSocket private constructor
 * @throw Const char *e
 * @param fd File descriptor of previously accepted socket
 * @param keyfile File to read key from
 * @param certfile File to read cert from
 */
IOSocketSSL::IOSocketSSL(	const int &fd,
							const char *keyfile,
							const char *certfile) : IOSocket(fd) 
{
	TRACE_CALL();

	setKeyFile(keyfile);
	setCertFile(certfile);
	init_internals(true);
}

/**
 * @desc Destructor
 */
IOSocketSSL::~IOSocketSSL() {

	TRACE_CALL();

	if (ssl) {
		SSL_free(ssl);
		ssl = NULL;
	}

	if (ctx) {
		SSL_CTX_free(ctx);
		ctx = NULL;
	}

	/* Cleanup ciphers, algo */
	EVP_cleanup();

	delete ssl_err;
	ssl_err = NULL;
}

SSL *IOSocketSSL::getSSL() {
	return ssl;
}

const char *IOSocketSSL::getCipher() {
	return SSL_get_cipher(ssl);
}

/**
 * @desc Initialize OpenSSL lib
 * @throw Char *
 */
void IOSocketSSL::initSSL(const bool force_server_method = false) {

	TRACE_CALL();

	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();
	OpenSSL_add_ssl_algorithms();

	switch (socket_t) {
		case IOSOCKET_LISTEN_T:
			ctx = SSL_CTX_new(SSLv3_server_method());
			break;

		case IOSOCKET_CONNECT_T:
			if (force_server_method)
				ctx = SSL_CTX_new(SSLv3_server_method());
			else
				ctx = SSL_CTX_new(SSLv3_client_method());
			break;

		default:
			/* Shouln't happen, mother class constructor should have thrown
			 * an exception earlier */
			throw("Unknown socket type");
			break;
	}

	/**
	 * Should use a loop on ERR_get_error()
	 * to retrieve the whole error process
	 */
	if (ctx == NULL)
		throw(new_SSL_error("initializing ssl context"));

	if (SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM) != 1)
		throw(new_SSL_error("loading cert file"));

	if (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) != 1)
		throw(new_SSL_error("loading private key file"));

	if (SSL_CTX_check_private_key(ctx) != 1)
		throw(new_SSL_error("verifying private key"));

	/* Don't bother with renego */
	SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);

	init_ssl = true;
}

/**
 * @desc Accept a new client connection from master socket
 * @return IOSocket * Client connection
 */
IOSocketSSL *IOSocketSSL::accept() {

	int 				c_sock = 0;
	struct sockaddr		addr;
	socklen_t			addrlen = sizeof(addr);
	IOSocketSSL			*client = NULL;

	TRACE_CALL();

again:
	c_sock = ::accept(sock, &addr, &addrlen);
	if (c_sock < 0) {
		if (errno != EINTR)
			throw("accept error");
		goto again;
	}

	if (stats.server.accepted++ == 0)
		connected = true; // Used to close connection when leaving

	/* Create a new IOSocketSSL to handle client handshake */
	try {
		client = new IOSocketSSL(c_sock, key_file, cert_file);
	} catch (const char *e) {
		throw(new_SSL_error("creating client SSL socket"));
	}

	/* Initialize SSL handshake */
	try {
		client->acceptSSL();
	} catch (const char *e) {
		stringstream ss;
		ss << new_SSL_error("acceptSSL on client socket") << "(" << e << ")";
		throw(ss.str().c_str());
	}

	cout << "Algorithme utilise : " <<  client->getCipher() << endl;

	return client;
}

void IOSocketSSL::acceptSSL() {
	
	TRACE_CALL();

	ssl = SSL_new(ctx);
	if (ssl == NULL) 
		throw(new_SSL_error("creating new SSL object"));

	/* Set File Descriptor for our SSL object */
	if (SSL_set_fd(ssl, sock) != 1)
		throw(new_SSL_error("Setting SSL object FD"));

	/* On attend notre connexion SSL */
	if (SSL_accept(ssl) != 1)
		throw(new_SSL_error("Waiting for SSL handshake"));

	handshake_done = true;
}

size_t IOSocketSSL::writeSSL(const struct io_buf &buffer) {

	size_t	written = 0;
	int		retry = 0;
	bool	eintr = false; // true if eintr was catched

	TRACE_CALL();

	if (!handshake_done)
		throw("SSL_accept()/SSL_connect() not called");

try_again:
	written = SSL_write(ssl, ((char *) buffer.content), buffer.length);
	
	
	if (written < 0) {
		try {
			eintr = throwSSLError(written);
		} catch (const char *e) {
			stringstream ss;
			ss << new_SSL_error("SSL_write error") << "(" << e << ")";
			throw(ss.str().c_str());
		}

		if (eintr) {
			if (retry++ < IOSOCKET_MAX_RETRY)
				goto try_again;
			else {
				stringstream ss;
				ss << new_SSL_error("SSL_write error") << "(Too many attempts)";
				throw(ss.str().c_str());
			}
		}
	}

	return written;
}

size_t IOSocketSSL::readSSL(struct io_buf *buffer) {

	size_t	written = 0;
	int		retry = 0;
	bool	eintr = false; // true if eintr was catched

	TRACE_CALL();

	if (!handshake_done)
		throw("SSL_accept()/SSL_connect() not called");

try_again:

	written = SSL_read(ssl, (char *) buffer->content, buffer->length);

	if (written < 0) {
		try {
			eintr = throwSSLError(written);
		} catch (const char *e) {
			stringstream ss;
			ss << new_SSL_error("SSL_read error") << "(" << e << ")";
			throw(ss.str().c_str());
		}

		if (eintr) {
			if (retry++ < IOSOCKET_MAX_RETRY)
				goto try_again;
			else {
				stringstream ss;
				ss << new_SSL_error("SSL_read error") << "(Too many attempts)";
				throw(ss.str().c_str());
			}
		}
	}

	return written;
}

size_t IOSocketSSL::write(const struct io_buf &buffer) {

	size_t			written = 0;
	size_t			offset = 0;
	size_t			to_write = buffer.length;
	struct io_buf 	buf;
	
	TRACE_CALL();

	if (to_write > IOSOCKET_NET_BUF_SIZE)
		to_write = IOSOCKET_NET_BUF_SIZE;

	do {

		::memcpy(buf.content, buffer.content + offset, to_write);
		buf.length = to_write;

		try {
			written = writeSSL(buf);
		} catch (const char *e) {
			stringstream ss;
			ss << new_SSL_error("writeSSL error") << "(" << e << ")";
			throw(ss.str().c_str());
		}

		offset += written;
		stats.client.bytesSent += written; /* Update stats */

		if (buffer.length - offset < IOSOCKET_NET_BUF_SIZE)
			to_write = buffer.length - offset;

	} while (offset != buffer.length);

	return offset;
}

size_t IOSocketSSL::read(struct io_buf *buffer) {

	size_t			has_read = 0;
	
	TRACE_CALL();

	try {
		has_read = readSSL(buffer);
	} catch (const char *e) {
		stringstream ss;
		ss << new_SSL_error("readSSL error") << "(" << e << ")";
		throw(ss.str().c_str());
	}

	stats.client.bytesReceived += has_read; /* Update stats */

	return has_read;
}

/**
 * @desc Throw the correct SSL error message
 * @throw char *
 * @return True if EINTR was catched and syscall should be retried
 */
bool IOSocketSSL::throwSSLError(const int error) {

	int err = error;

	TRACE_CALL();

	switch (SSL_get_error(ssl, err)) {
		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
			throw(new_SSL_error("SSL_CTX_set_mode() should have fixed this... STRANGE !"));
			break;

		case SSL_ERROR_ZERO_RETURN:
			/* Connection closed */
			throw(new_SSL_error("Connection closed by remote host"));
			break;

		case SSL_ERROR_SYSCALL:
			if (err == -1) {
				if (errno == EINTR)
					return true;
			}
			break;

		case SSL_ERROR_SSL:
			throw(new_SSL_error("Internal library error"));
			break;

		default:
			throw(new_SSL_error("Unknown error"));
			break;
	}
	return false;
}

//void IOSocketSSL::read(struct io_buf *buffer) {};

void IOSocketSSL::close() {

	TRACE_CALL();

	// Close / shutdown and set endTime
	IOSocket::close();

	if (ssl) {
		SSL_free(ssl);
		ssl = NULL;
	}

	if (ctx) {
		SSL_CTX_free(ctx);
		ctx = NULL;
	}
}

