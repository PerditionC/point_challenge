#pragma once
#ifndef __PS_TIMER__
#define __PS_TIMER__

#include <Windows.h>

class ps_timer
{
public:
	/* initializes, calculates overhead, and begins timing (can be overridden by explicit start() call) */
	ps_timer(boolean autostart = true);

	/* begin/resume timing */
	void start(void);
	/* end/pause timing */
	void stop(void);
	/* clear existing time, ie restart */
	void reset(void);
	/* return how much time passed between start & stop call in microseconds, invokes stop if not already called */
	double elapsed(void);

private:
	/* calculate extra time used to perform the timing calculations & reset() */
	void overhead(void);

	/* internal time keeping implemenation */
	LARGE_INTEGER m_start;
	LARGE_INTEGER m_stop;
	double conversionFactor;
	LONGLONG m_overhead;
	LONGLONG m_count;
};


#endif /* __PS_TIMER__ */
