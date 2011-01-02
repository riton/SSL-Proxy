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
	
		cout << "Read from socket:: " << buffer.content << endl;

	} catch (const char *e) {
		cerr << e << ": " << strerror(errno) << endl;
	}


	return(EXIT_SUCCESS);
}

