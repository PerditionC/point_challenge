#pragma once
#ifndef __PROCESS_INFO__
#define __PROCESS_INFO__

#include <Windows.h>
#include <Psapi.h>


class processInfo
{
public:
	/* gets a handle to current process */
	processInfo(void);

	/* take initial snapshot of working set */
	void start(void);

	/* takes snapshot of final working set */
	void stop(void);

	/* returns how much memory (in MB) used from start to stop, calls stop if not already called */
	unsigned long usedMB(void);

private:
	/* process to get information from */
	HANDLE hProcess;
	/* start and stopping process information */
	PROCESS_MEMORY_COUNTERS m_start;
	PROCESS_MEMORY_COUNTERS m_stop;
};

#endif /* __PROCESS_INFO__ */
