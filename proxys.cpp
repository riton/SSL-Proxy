#include <stdlib.h>
#include <iostream>
#include <string>
#include <sstream>
#include "OSelect.h"
#include <string.h>

using namespace std;

int main(int argc, char *argv[]) {

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

	return(EXIT_SUCCESS);
}

