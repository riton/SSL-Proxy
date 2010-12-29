#ifndef __OSELECT_H
#define __OSELECT_H

#include <sys/select.h>
#include <sys/time.h>
#include <algorithm>
#include <sstream>
#include <list>
using namespace std;

#define OSELECT_MAX_FDS	10

class IOSelect {

	private:
		fd_set				set;
		list<int>			fds;
		int					max_fd;
	
	public:
		IOSelect(const int fds[]);
		IOSelect();

		list<int> can_read(const struct timeval *timeout);
		list<int> can_read(const int seconds);
		list<int> can_write(const struct timeval *timeout);
		list<int> can_write(const int seconds);
		void add(const int &fd);
		void remove(const int &fd);
		size_t count(void);
};

class IOSelectTimeout {

	private:
		string			msg;
		struct timeval	*timeout;

	public:

	IOSelectTimeout(const struct timeval *timeout) {
		stringstream ss;

		this->timeout = (struct timeval *) timeout;

		ss << timeout->tv_sec;
		ss << "." << timeout->tv_usec;
		ss << " seconds without I/O" << endl;
		msg = ss.str();
	}

	const char *what() const throw() {
		return msg.c_str();
	}
};



#endif
