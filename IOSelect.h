#ifndef __OSELECT_H
#define __OSELECT_H

#include <sys/select.h>
#include <sys/time.h>
#include <algorithm>
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

#endif
