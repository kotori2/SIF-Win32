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
#include <stdlib.h>
#include "CSampleProjectEntrance.h"
#include "CKLBJsonItem.h"
#include "SIF_Win32.h"

struct KeyValue_Info
{
	const char* name;
	bool* value;
};

KeyValue_Info BooleanValList[] = {
	{"AllowKeyboard", &SIF_Win32::AllowKeyboard},
	{"AllowTouchscreen", &SIF_Win32::AllowTouchscreen},
	{"DebugMode", &SIF_Win32::DebugMode},
	{"SingleCore", &SIF_Win32::SingleCore},
	{"CloseWindowAsBack", &SIF_Win32::CloseWindowAsBack},
	{"AndroidMode", &SIF_Win32::AndroidMode},
	{"KeepRunningOnError", &SIF_Win32::KeepRunningOnError},
	{"LuaStdin", &SIF_Win32::LuaStdin},
	{NULL, NULL}
};

bool ReadConfiguration()
{
	FILE* fp = NULL;
	char* data = NULL;
	size_t fp_size = 0;
	CKLBJsonItem* node = NULL;

	if(false)
	{
		readconfig_fail:

		if(fp) fclose(fp);
		delete[] data;
		delete node;
		
		return false;
	}

	fp = fopen("SIF-Win32.json", "rb");

	if(fp == NULL) goto readconfig_fail;

	fseek(fp, 0, SEEK_END);

	fp_size = ftell(fp);
	data = new char[fp_size + 1];
	data[fp_size] = 0;

	fseek(fp, 0, SEEK_SET);
	fread(data, 1, fp_size, fp);

	node = CKLBJsonItem::ReadJsonData(data, fp_size);
	
	if(node == NULL) goto readconfig_fail;

	for(CKLBJsonItem* child = node->child();; child = child->next())
	{
		if(child == NULL) break;
		// Check for boolean values first
		{
			KeyValue_Info* list = &BooleanValList[0];

			for(;;list++)
			{
				if(list->name == NULL || list->value == NULL)
					break;

				if(strcmp(list->name, child->key()) == 0 && child->getType() == CKLBJsonItem::J_BOOLEAN)
				{
					*(list->value) = child->getBool();
					break;
				}
			}
		}

		// Now for special case, VirtualIdolKey
		if(strcmp(child->key(), "VirtualKeyIdol") == 0 && child->getType() == CKLBJsonItem::J_ARRAY)
		{
			CKLBJsonItem* idol_list = child->child();

			for(int i = 0; i < 9 && idol_list != NULL; i++)
			{
				int VK = idol_list->getInt();

				if(VK == 0) goto readconfig_fail;

				SIF_Win32::VirtualKeyIdol[i] = VK;
				idol_list = idol_list->next();
			}
		}
	}
	
	fclose(fp);
	delete[] data;
	delete node;

	return true;
}

bool
GameSetup()
{
	CSampleProjectEntrance * pClient = CSampleProjectEntrance::getInstance();

	CPFInterface& pfif = CPFInterface::getInstance();
	pfif.setClientRequest(pClient);

	// Preload fonts
	klb_assert(pfif.platform().registerFont("MotoyaLMaru W3 mono", "asset://MTLmr3m.ttf", true), "[LoveLive base] GAME FONT MTLmr3m.ttf NOT INSTALLED");
	//klb_assert(pfif.platform().registerFont("CurrencySymbols", "asset://CurrencySymbols.ttf", false), "[LoveLive base] GAME FONT CurrencySymbols.ttf NOT INSTALLED");

	// Load config
	klb_assert(ReadConfiguration(), "Unable to read or parse configuration file: SIF-Win32.json");

	return true;
}

CSampleProjectEntrance::CSampleProjectEntrance() : CKLBGameApplication() {}
CSampleProjectEntrance::~CSampleProjectEntrance() {}

CSampleProjectEntrance *
CSampleProjectEntrance::getInstance()
{
    static CSampleProjectEntrance instance;
    return &instance;
}


bool
CSampleProjectEntrance::initLocalSystem(CKLBAssetManager& mgrAsset)
{
	return true;
}

void
CSampleProjectEntrance::localFinish()
{
}
