#ifndef __IOSOCKETSSL_H
#define __IOSOCKETSSL_H

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
#include <list>
#include <sstream>
#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "IOSocket.h"

using namespace std;

	
#if 0

	/* On cree notre socket */
	sock = createListenSocket(12345);


	/* On attend un client */
	if( (ssock = accept( sock, (struct sockaddr *) &client, (socklen_t *) &sizeofclient )) < 0 ){
		fprintf(stderr,"accept :: %s\n",strerror(errno));
		exit(EXIT_FAILURE);
	}	

	printf("Connection etablie\n");

	/* On cree notre contexte SSL pour notre connexion */
	if( (ssl = SSL_new(ctx)) == NULL ){
		perror("SSL_new");
		exit(EXIT_FAILURE);
	}	

	/* On associe notre objet SSL a notre socket */
	SSL_set_fd(ssl, ssock);

	/* On attend notre connexion SSL */
	if( SSL_accept(ssl) < 0 ){
		perror("SSL_accept");
		exit(EXIT_FAILURE);
	}	
#endif


/**
 * SSL Error buffer length
 */
#define __IOSOCKETSSL_ERR_BUF_LEN	2048

/**
 * Max Cert/Key file name length
 */
#define __IOSOCKETSSL_MAX_FILENAME_LEN	1024

/**
 * @desc This class aims to provide easy access to SSL socket function
 * and to provide a socket factory for server functionnalities, with SSL layer
 */
class IOSocketSSL: public IOSocket {

	/**
	 * Attributes
	 */
	private:
		/* Context handles certs, algo, etc... */
		SSL_CTX		*ctx;
		/* Real SSL connection object */
		SSL			*ssl;

		bool		init_ssl;
		bool		handshake_done;
		char		*ssl_err;
		char		cert_file[__IOSOCKETSSL_MAX_FILENAME_LEN];
		char		key_file[__IOSOCKETSSL_MAX_FILENAME_LEN];

	/**
	 * Methods
	 */
	private:
		IOSocketSSL(const SSL_CTX *ctxs, const int &fd); 
		void init_internals();
		const char *new_SSL_error(const char *e);
		void initSSL();
		void acceptSSL();

	public:
		IOSocketSSL(const socket_type sock_t, const char *host, const int port, const char *keyfile, const char *certfile);
		IOSocketSSL *accept();
		~IOSocketSSL();

		/* Getter */
		SSL *getSSL(void);

		/* I/O */
		virtual void write(const struct io_buf &buffer);
		virtual void write(const char *msg);
		virtual void read(struct io_buf *buffer);

		virtual void close();
};

#endif
