#include <stdlib.h>
#include <iostream>
#include <string>
#include <sstream>
#include <string.h>
#include "IOSocketSSL.h"

using namespace std;

#define PATH_TO_SSL "/home/riton/Sources/C++/proxy_ssl/client_server_ssl_c/cert"
#define KEY_FILE PATH_TO_SSL "/ftptcp.key"
#define CERT_FILE PATH_TO_SSL "/ftptcp.crt"

int main(int argc, char *argv[]) {

	IOSocketSSL 		*msock;
	IOSocketSSL 		*client = NULL;
	struct io_buf		buffer;

	cout << "Using key file: " << KEY_FILE << endl;
	cout << "Using cert file: " << CERT_FILE << endl;
		

	try {
	
		msock = new IOSocketSSL(IOSOCKET_LISTEN_T,
								NULL,
								12345,
								KEY_FILE,
								CERT_FILE);

		client = msock->accept();
/*		client->write("test\n");
		client->read(&buffer);*/
		client->close();
		cout << "Read from socket:: " << buffer.content << endl;

	} catch (const char *e) {
		cerr << e << ": " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	} 

	{
		io_stat *stat = &(client->stats);
		cout << "Socket_Stats:" << endl;
		cout << "\tSocket_In: " << stat->client.bytesReceived << endl;
		cout << "\tSocket_out: " << stat->client.bytesSent << endl;
		cout << "\tDuration: " << stat->client.endTime - stat->client.startTime << endl;
	}


	return(EXIT_SUCCESS);
}

