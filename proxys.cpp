#include <stdlib.h>
#include <iostream>
#include <string>
#include <sstream>
#include "IOSelect.h"
#include "IOSocket.h"
#include <string.h>

using namespace std;

int main(int argc, char *argv[]) {

#if 0
	OSelect	*ioselect = NULL;
	int fd[3];

	memset(&fd, 0x0, sizeof(fd));
	fd[0] = 0;
	fd[1] = 1;
	fd[2] = -1;

	ioselect = new OSelect(fd);

	list<int> readable = ioselect->can_read(5);

	cout << "Size: " << readable.size() << endl;

	for (list<int>::iterator it = readable.begin(); it != readable.end(); it++)
		cout << "Readable: " << *it << endl;
#endif

	IOSocket *sock = NULL;
	io_buf buffer;
	int	p = 12345;
	int pn = htonl(p);

	buffer.content = (char *) &pn;
	buffer.length = sizeof(p);

	cout << "Length: " << sizeof(p) << endl;

	try {
		sock = new IOSocket("127.0.0.1", 12345, 0);
		sock->write(buffer);
	} catch (const char *e) {
		char *s = strerror(errno);
		cerr << "ERROR: " << e << " (" << s << ")" << endl;
	}

	return(EXIT_SUCCESS);
}

