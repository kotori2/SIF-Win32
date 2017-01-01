#include <Windows.h>

#define EXPORT_MODE

namespace TaskbarProgress
{
	// Init
	EXPORT_MODE bool Activate(HWND hWnd);
	EXPORT_MODE void Deactivate();
	
	// 4 states
	EXPORT_MODE void ProgressGreen();
	EXPORT_MODE void ProgressYellow();
	EXPORT_MODE void ProgressRed();
	EXPORT_MODE void ProgressUnknown();
	
	// Progress
	EXPORT_MODE void SetValue(unsigned Value, unsigned MaxValue);
	EXPORT_MODE void SetValue(unsigned Value);
}
