/* 
   Copyright 2013 KLab Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include "SIF_Win32.h"
#include "assert_klb.h"
#include "CWin32Platform.h"
#include <Windows.h>
#include <stdio.h>

void assertFunction(int line, const char* file, const char* msg,...) {
	CWin32Platform& platform = dynamic_cast<CWin32Platform&>(CPFInterface::getInstance().platform());
	va_list	argp;
	char pszBuf [1024];
	char log	[1024];
	HWND hWnd = &platform ? platform.get_hWnd() : NULL;

	va_start(argp, msg);
	vsprintf_s( pszBuf,1024, msg, argp);
	va_end(argp);

	sprintf_s( log,1024, "Assert l.%i in %s : \n%s\n",line, file, pszBuf); 

	MessageBox(hWnd , log, "Assert", MB_OK | MB_ICONEXCLAMATION);

	if(SIF_Win32::KeepRunningOnError == false)
		if(IsDebuggerPresent())
			DebugBreak();
		else
			exit(1);
}

void msgBox(char* log) {
	MessageBox( NULL, log, "Sinc Error", MB_OK);
}
