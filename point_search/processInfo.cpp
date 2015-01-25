#include "processInfo.h"


/* gets a handle to current process */
processInfo::processInfo(void)
{
	hProcess = GetCurrentProcess();
}

/* take initial snapshot of working set */
void processInfo::start(void)
{
	memset(&m_stop, 0, sizeof(m_stop));
	if (!GetProcessMemoryInfo(hProcess, &m_start, sizeof(m_start)))
		memset(&m_start, 0, sizeof(m_start));
}

/* takes snapshot of final working set */
void processInfo::stop(void)
{
	if (!GetProcessMemoryInfo(hProcess, &m_stop, sizeof(m_stop)))
		memset(&m_stop, 0, sizeof(m_stop));
}

/* returns how much memory (in MB) used from start to stop, calls stop if not already called */
unsigned long processInfo::usedMB(void)
{
	if (m_stop.WorkingSetSize == 0) stop();
	return static_cast<unsigned long>(m_stop.WorkingSetSize - m_start.WorkingSetSize)/1048576ul;
}
