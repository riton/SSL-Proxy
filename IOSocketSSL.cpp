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

void IOSocketSSL::init_internals() {

	TRACE_CALL();

	ssl_err = new char[__IOSOCKETSSL_ERR_BUF_LEN];
	handshake_done = false;

	//@throw char *e 
	initSSL();
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
	size_t	len = 0;

	TRACE_CALL();

	len = sizeof(key_file);
	strncpy(key_file, keyfile, len - 1);
	key_file[len-1] = '\0';

	len = sizeof(cert_file);
	strncpy(cert_file, certfile, len - 1);
	cert_file[len-1] = '\0';

	init_internals();
}

/**
 * @desc Private constructor that uses IOSocket private constructor
 * @throw Const char *e
 * @param ctxs SSL context of master socket
 * @param fd File descriptor of previously accepted socket
 */
IOSocketSSL::IOSocketSSL(	const SSL_CTX *ctxs,
							const int &fd) : IOSocket(fd) 
{
	TRACE_CALL();
	/* We must allocate ourself with malloc()
	 * for we don't use OpenSSL internals */
	ctx = (SSL_CTX *) malloc(sizeof(SSL_CTX));
	memcpy(ctx, ctxs, sizeof(SSL_CTX));
	init_internals();
}

/**
 * @desc Destructor
 */
IOSocketSSL::~IOSocketSSL() {

	TRACE_CALL();

	/* Cleanup ciphers, algo */
	EVP_cleanup();

	delete ssl_err;
}

/**
 * @desc Initialize OpenSSL lib
 * @throw Char *
 */
void IOSocketSSL::initSSL() {
	
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
		client = new IOSocketSSL(ctx, c_sock);
	} catch (const char *e) {
		throw(new_SSL_error("creating client SSL socket"));
	}

	/* Initialize SSL handshake */
	try {
		client->acceptSSL();
	} catch (const char *e) {
		throw(new_SSL_error("acceptSSL on client socket"));
	}

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

void IOSocketSSL::write(const struct io_buf &buffer) {};
void IOSocketSSL::write(const char *msg) {};
void IOSocketSSL::read(struct io_buf *buffer) {};

void IOSocketSSL::close() {};

