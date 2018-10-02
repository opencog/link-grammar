#ifndef __CLOCK_H__
#define __CLOCK_H__

/**
 *   Time measurement functions
 */

#include <iostream>
#include <ctime>

#include "error.h"

class Clock {
private:
	clock_t  start;
public:
	Clock() {
		reset();
	}

	void reset()
	{
		start = clock();
	}

public:
	double elapsed()
	{
		clock_t stop = clock();
		return ((double)stop-(double)start)/CLOCKS_PER_SEC;
	}

	void print_time(int verbosity_opt, const char *s)
	{
		if (verbosity_opt >= D_USER_TIMES)
		{
			printf("++++ %-36s %7.2f seconds\n", s, elapsed());
		}
	}
};
#endif
