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
#include <stdafx.h>
#include <Windows.h>

#include <openssl/sha.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include "GameEngine.h"
#include "SIF_Win32.h"

namespace SIF_Win32
{
	bool AllowKeyboard = true;
	unsigned char VirtualKeyIdol[9] = {
		52,		// 4
		82,		// R
		70,		// F
		86,		// V
		66,		// B
		78,		// N
		74,		// J
		73,		// I
		57,		// 9
	};
	bool AllowTouchscreen = true;
	bool DebugMode = false;
	bool SingleCore = false;
	bool CloseWindowAsBack = false;
	bool AndroidMode = false;
	bool ChikaIcon = true;
	bool KeepRunningOnError = false;
	bool LuaStdin = false;
}

static HANDLE lock;

bool OneInstanceCheck()
{
	char hashed_path[MAX_PATH];
	char exedir[MAX_PATH];
	char hash[SHA256_DIGEST_LENGTH * 2 + 1];

	GetModuleFileNameA(NULL, exedir, MAX_PATH);

	{
		char* last_occured = NULL;

		for(char* x = exedir; *x != 0; x++)
			if(*x == '\\')
				last_occured = x;

		if(last_occured)
			*last_occured = 0;

		unsigned char temp_hash[SHA256_DIGEST_LENGTH];
		char* target_hash = hash;

		SHA256((unsigned char*)exedir, strlen(exedir), temp_hash);

		for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
		{
			sprintf(target_hash, "%02x", int(temp_hash[i]));
			target_hash += 2;
		}
	}

	sprintf(hashed_path, "Global\\SIF-Win32__%s", hash);

	lock = CreateMutexA(NULL, 0, hashed_path);

	if(GetLastError() == ERROR_ALREADY_EXISTS)
	{
		CloseHandle(lock);
		return false;
	}

	return true;
}

int main(int argc, char* argv[])
{
	if(!OneInstanceCheck())
	{
		MessageBoxA(NULL, "Only one instance of the application is allowed", "SIF-Win32", MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	if(argc > 1)
	{
		if(strcmpi(argv[1], "-config") == 0)
		{
			MessageBoxA(NULL, "TODO: Show config window", "Unimplemented", MB_OK | MB_ICONEXCLAMATION);
			return 0;
		}
	}

	ERR_load_crypto_strings();
	OPENSSL_add_all_algorithms_noconf();

	int ret = GameEngineMain(argc, argv);

	CloseHandle(lock);
	return ret;
}
