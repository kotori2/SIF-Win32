// ITaskBarList3 progress

#include "TaskbarProgress.h"

#include <Windows.h>
#include <ShObjIdl.h>

namespace TaskbarProgress
{
	HWND WindowHandle;
	
	bool InvalidWindowVersion = false;
	unsigned OriginMaxValue = 100;

	ITaskbarList3* TaskbarList3;
}

bool TaskbarProgress::Activate(HWND hWnd)
{
	if(TaskbarList3)
		return true;

	if(InvalidWindowVersion)
		return false;

	if(IsWindow(hWnd) == false)
		return false;

	OSVERSIONINFOA osinfo;

	osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	GetVersionExA(&osinfo);

	if((osinfo.dwMajorVersion * 100 + osinfo.dwMinorVersion) < 601)
		return InvalidWindowVersion = false;

	// Window valid. Now create
	if(CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, (void**)&TaskbarList3) != S_OK)
		return false;

	WindowHandle = hWnd;
	return true;
}

void TaskbarProgress::Deactivate()
{
	if(TaskbarList3 == NULL)
		return;

	SetValue(0);
	TaskbarList3->Release();

	WindowHandle = NULL;
	TaskbarList3 = NULL;
}

void TaskbarProgress::ProgressGreen()
{
	if(TaskbarList3 == NULL)
		return;

	TaskbarList3->SetProgressState(WindowHandle, TBPF_NORMAL);
}

void TaskbarProgress::ProgressYellow()
{
	if(TaskbarList3 == NULL)
		return;

	TaskbarList3->SetProgressState(WindowHandle, TBPF_PAUSED);
}

void TaskbarProgress::ProgressRed()
{
	if(TaskbarList3 == NULL)
		return;

	TaskbarList3->SetProgressState(WindowHandle, TBPF_ERROR);
}

void TaskbarProgress::ProgressUnknown()
{
	if(TaskbarList3 == NULL)
		return;

	TaskbarList3->SetProgressState(WindowHandle, TBPF_INDETERMINATE);
}

void TaskbarProgress::SetValue(unsigned val, unsigned maxval)
{
	if(TaskbarList3 == NULL)
		return;

	OriginMaxValue = maxval;

	TaskbarList3->SetProgressValue(WindowHandle, val, maxval);
}

void TaskbarProgress::SetValue(unsigned val)
{
	if(TaskbarList3 == NULL)
		return;

	TaskbarList3->SetProgressValue(WindowHandle, val, OriginMaxValue);
}
