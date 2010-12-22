#include "OSelect.h"
#include <iostream>
#include <string.h>
#include <errno.h>

OSelect::OSelect() {
	FD_ZERO(&set);
}

/**
 * This constructor set up max fd
 * and feed internal list
 */
OSelect::OSelect(const int fds[]) {

	int				pi = NULL;
	int				max_fd = -1;

	OSelect();
	for (pi = 0; fds[pi] >= 0; pi++) {
		if (fds[pi] > max_fd)
			max_fd = fds[pi];

		this->fds.push_front(fds[pi]);
	}

	this->max_fd = max_fd;
}

void OSelect::add(const int &fd) {
	if (fd > max_fd)
		max_fd = fd;
	return fds.push_front(fd);
}

void OSelect::remove(const int &fd) {
	return fds.remove(fd);
}

size_t OSelect::count(void) {
	return fds.size();
}

list<int> OSelect::can_read(const struct timeval *time) {

	list<int>::iterator	it;
	list<int>			readable;
	struct timeval 		timeout;
	int					code = 0;

again:
	/* Timeout could be modified by select() */
	memcpy(&timeout, time, sizeof(timeout));

	FD_ZERO(&set);
	for (it = fds.begin(); it != fds.end(); it++)
		FD_SET(*it, &set);

	code = select(max_fd+1, &set, NULL, NULL, &timeout);
	if (code < 0) {
		if (errno != EINTR) {
			throw("Select error");
		}
		goto again;
	}

	for (it = fds.begin(); it != fds.end(); it++) {
		if (FD_ISSET(*it, &set))
			readable.push_front(*it);
	}

	return readable;
}

list<int> OSelect::can_read(const int seconds) {
	struct timeval timeout = {seconds, 0};
	return can_read(&timeout);
}

list<int> OSelect::can_write(const struct timeval *time) {

	list<int>::iterator	it;
	list<int>			writable;
	struct timeval 		timeout;
	int					code = 0;

again:
	/* Timeout could be modified by select() */
	memcpy(&timeout, time, sizeof(timeout));

	FD_ZERO(&set);
	for (it = fds.begin(); it != fds.end(); it++)
		FD_SET(*it, &set);

	code = select(max_fd+1, NULL, &set, NULL, &timeout);
	if (code < 0) {
		if (errno != EINTR) {
			throw("Select error");
		}
		goto again;
	}

	for (it = fds.begin(); it != fds.end(); it++) {
		if (FD_ISSET(*it, &set))
			writable.push_front(*it);
	}

	return writable;
}

list<int> OSelect::can_write(const int seconds) {
	struct timeval timeout = {seconds, 0};
	return can_read(&timeout);
}
