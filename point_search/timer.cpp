#include "timer.h"
#include <stdio.h>

ps_timer::ps_timer(boolean autostart)
{
	/* get maximum resolution of timer */
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	/* get value to multiple by to convert performance counter to ms */
	conversionFactor = 1000.0 / static_cast<double>(frequency.QuadPart);

	/* get approx calling overhead, also resets running total */
	overhead();

	/* begin timing? */
	if (autostart) start();
}
 

/* extra time used to perform the timing calculations, including function call */
void ps_timer::overhead(void)
{
	/* time not doing anything to approx overhead of timing */
	start();
	stop();
	m_overhead = m_stop.QuadPart - m_start.QuadPart;

	/* ensure this time not included in future timings */
	reset();
}


/* begin/resume timing */
void ps_timer::start(void)
{
	/* reset timer and get initial value */
	QueryPerformanceCounter(&m_start);
	m_stop.QuadPart = 0;
}

/* end/pause timing */
void ps_timer::stop(void)
{
	QueryPerformanceCounter(&m_stop);
	m_count += m_stop.QuadPart - m_start.QuadPart;
}

/*  clear existing time, ie restart */
void ps_timer::reset(void)
{
	/* init running count, total counts between all start()-stop() calls */
	m_count = 0ul;	
}

/* return how much time passed between start & stop call in microseconds */
double ps_timer::elapsed(void)
{
	/* call stop if not already done */
	if (m_stop.QuadPart == 0) stop();
	/* validate internal state, returning 0.0 if invalid */
	if ((m_overhead < 0ul) || (m_count < 0ul)) return 0.0;
	/* return time from start to stop minus overhead of the timing */
	register LONGLONG totalCounts = m_count - m_overhead;
	if (totalCounts <= 0) return 0.0;
	//printf("%lu counts * %f = %f==%.4f\n", totalCounts, conversionFactor, (totalCounts * conversionFactor), (totalCounts * conversionFactor));
	return totalCounts * conversionFactor;
}

