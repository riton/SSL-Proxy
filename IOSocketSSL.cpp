#include "IOSocketSSL.h"

#define SSL_S_ERROR() ERR_error_string(ERR_peek_last_error(), NULL)

/**
 * @desc Return a well formated error string with SSL last error
 * @param msg Action we were doing when error occurs
 */
const char *IOSocketSSL::new_SSL_error(const char *msg) {

	memset(ssl_err, 0x0, __IOSOCKETSSL_ERR_BUF_LEN);
	snprintf(ssl_err, __IOSOCKETSSL_ERR_BUF_LEN-1, "%s: %s", msg, SSL_S_ERROR());
	ssl_err[__IOSOCKETSSL_ERR_BUF_LEN-1] = '\0';

	return ssl_err;
}

/**
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
	ssl_err = new char[__IOSOCKETSSL_ERR_BUF_LEN];

	try {
		initSSL(keyfile, certfile);
	} catch (const char *e) {
		cerr << "SSL_Initialization failed: " << e << endl;
	}
}

/**
 * @desc Destructor
 */
IOSocketSSL::~IOSocketSSL() {
	/* Cleanup ciphers, algo */
	EVP_cleanup();

	delete ssl_err;
}

/**
 * @desc Initialize OpenSSL lib
 * @throw Char *
 * @param keyfile File to load private key from
 * @param certfile File to load certificate from
 */
void IOSocketSSL::initSSL(const char *keyfile, const char *certfile) {

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

	if (SSL_CTX_use_certificate_file(ctx, certfile, SSL_FILETYPE_PEM) != 1)
		throw(new_SSL_error("loading cert file"));

	if (SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM) != 1)
		throw(new_SSL_error("loading private key file"));

	if (SSL_CTX_check_private_key(ctx) != 1)
		throw(new_SSL_error("verifying private key"));

	init_ssl = true;
}

void IOSocketSSL::write(const struct io_buf &buffer) {};
void IOSocketSSL::write(const char *msg) {};
void IOSocketSSL::read(struct io_buf *buffer) {};

void IOSocketSSL::close() {};

