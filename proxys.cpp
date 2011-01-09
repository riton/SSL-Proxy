#include <stdlib.h>
#include <iostream>
#include <string>
#include <sstream>
#include "IOSocket.h"
#include <string.h>

using namespace std;

int main(int argc, char *argv[]) {

	IOSocket 		*msock;
	IOSocket 		*client = NULL;
	struct io_buf	buffer;
		

	try {
	
		msock = new IOSocket(IOSOCKET_LISTEN_T, NULL, 12345);

		client = msock->accept();
		client->write("test\n");
		client->read(&buffer);
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

