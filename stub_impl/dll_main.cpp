#define WIN32_LEAN_AND_MEAN
#include <windows.h>

extern "C" BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}
