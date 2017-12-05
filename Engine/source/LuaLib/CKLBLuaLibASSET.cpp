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
#include "CKLBLuaLibASSET.h"
#include "CKLBUtility.h"
#include "CWin32PathConv.h"
#include "dirent.h"
#include "DownloadQueue.h"
#include  <time.h>

int assetSize = 0;
char aes_key_client[32] = "";
char sessionKey[32] = "";

void pirntHex(char a[], int len) {
	for (int i = 0; i < len; i++) {
		printf("%x", a[i]);
	}
	printf("\n");
}

static ILuaFuncLib::DEFCONST luaConst[] = {
//	{ "DBG_M_SWITCH",	DBG_MENU::M_SWITCH },
	{ 0, 0 }
};

static CKLBLuaLibASSET libdef(luaConst);

CKLBLuaLibASSET::CKLBLuaLibASSET(DEFCONST * arrConstDef) : ILuaFuncLib(arrConstDef) {}
CKLBLuaLibASSET::~CKLBLuaLibASSET() {}

int get_asset_path_if_not_found(lua_State* L);
int asset_start_download(lua_State* L);
int asset_kill_download(lua_State* L);
//int DownloadClient(lua_State* L);

// Lua関数の追加
void
CKLBLuaLibASSET::addLibrary()
{
	addFunction("ASSET_getImageSize",		CKLBLuaLibASSET::luaGetImageSize);
	addFunction("ASSET_getBoundSize",		CKLBLuaLibASSET::luaGetBoundSize);
	addFunction("ASSET_getAssetInfo",		CKLBLuaLibASSET::luaGetAssetInfo);
	addFunction("ASSET_delExternal",		CKLBLuaLibASSET::luaDelExternal);
	addFunction("ASSET_getExternalFree",	CKLBLuaLibASSET::luaGetExternalFree);
	addFunction("ASSET_getFileList",		CKLBLuaLibASSET::luaGetFileList);
	addFunction("ASSET_registerNotFound",	CKLBLuaLibASSET::luaRegisterNotFound);
	addFunction("ASSET_setPlaceHolder",		CKLBLuaLibASSET::luaSetPlaceHolder);
	addFunction("ASSET_getAssetPathIfNotExist",get_asset_path_if_not_found);
	addFunction("ASSET_startDownload",		asset_start_download);
	addFunction("ASSET_killDownload",		asset_kill_download);
	addFunction("Asset_getNMAssetSize",     CKLBLuaLibASSET::luaGetNMAssetSize);
	addFunction("Asset_getNMAsset",         CKLBLuaLibASSET::luaGetNMAsset); 
	addFunction("Asset_setNMAsset",         CKLBLuaLibASSET::luaSetNMAsset);
	addFunction("checkUncompleteUnzip",     CKLBLuaLibASSET::checkUncompleteUnzip);
}

s32
CKLBLuaLibASSET::luaGetImageSize(lua_State * L)
{
	CLuaState lua(L);
	int argc = lua.numArgs();
	if(argc != 1) {
		lua.retNil();
		lua.retNil();
		return 2;
	}

	const char * asset_name = lua.getString(1);
	u32 handle;
	CKLBImageAsset * pAsset = (CKLBImageAsset *)CKLBUtility::loadAssetScript(asset_name, &handle);
	if(!pAsset) {
		lua.retNil();
		lua.retNil();
		return 2;
	}

	SKLBRect * rect = pAsset->getSize();

	s32 width = rect->getWidth();
	s32 height = rect->getHeight();

	CKLBDataHandler::releaseHandle(handle);

	lua.retInt(width);
	lua.retInt(height);
	return 2;	
}

s32
CKLBLuaLibASSET::luaGetBoundSize(lua_State * L)
{
	CLuaState lua(L);
	int argc = lua.numArgs();
	if(argc != 1) {
		lua.retNil();
		lua.retNil();
		return 2;
	}
	const char * asset_name = lua.getString(1);
	u32 handle;
	CKLBImageAsset * pAsset = (CKLBImageAsset *)CKLBUtility::loadAssetScript(asset_name, &handle);
	if(!pAsset) {
		lua.retNil();
		lua.retNil();
		return 2;
	}
	float width = pAsset->m_boundWidth;
	float height = pAsset->m_boundHeight;

	CKLBDataHandler::releaseHandle(handle);

	lua.retDouble(width);
	lua.retDouble(height);
	return 2;	

}

s32
CKLBLuaLibASSET::luaGetAssetInfo(lua_State * L)
{
	CLuaState lua(L);
	int argc = lua.numArgs();
	if(argc != 1) {
		lua.retNil();
		lua.retNil();
		lua.retNil();
		lua.retNil();
		return 4;
	}
	const char * asset_name = lua.getString(1);
	u32 handle;
	CKLBImageAsset * pAsset = (CKLBImageAsset *)CKLBUtility::loadAssetScript(asset_name, &handle);
	if(!pAsset) {
		lua.retNil();
		lua.retNil();
		lua.retNil();
		lua.retNil();
		return 4;
	}
	SKLBRect * rect = pAsset->getSize();
	s32 img_width = rect->getWidth();
	s32 img_height = rect->getHeight();
	float bound_width = pAsset->m_boundWidth;
	float bound_height = pAsset->m_boundHeight;

	CKLBDataHandler::releaseHandle(handle);

	lua.retInt(img_width);
	lua.retInt(img_height);
	lua.retDouble(bound_width);
	lua.retDouble(bound_height);

	return 4;
}

s32
CKLBLuaLibASSET::luaDelExternal(lua_State * L)
{
	CLuaState lua(L);
	int argc = lua.numArgs();
	if(argc != 1) {
		lua.retBool(false);
		return 1;
	}
	const char * asset_name = lua.getString(1);
	bool res = CPFInterface::getInstance().platform().removeFileOrFolder(asset_name);
	lua.retBool(res);
	return 1;
}

s32
CKLBLuaLibASSET::luaGetExternalFree(lua_State * L)
{
	CLuaState lua(L);
	int argc = lua.numArgs();
	if(argc != 0) {
		lua.retInt(0);
		return 1;
	}
	s32 res = (s32)CPFInterface::getInstance().platform().getFreeSpaceExternalKB(); // Never return more than 0xFFFFFF
	lua.retInt(res);
	return 1;
}

s32 CKLBLuaLibASSET::luaGetFileList(lua_State* L)
{
	CWin32PathConv& pc = CWin32PathConv::getInstance();
	const char* path = pc.fullpath(lua_tolstring(L, 1, NULL) + 7);

	DIR* dir;
	dirent* ent;
	int i = 0;

	lua_newtable(L);

	if(path)
	{
		if(dir = opendir(path))
		{
			while(ent = readdir(dir))
			{
				lua_pushinteger(L, ++i);
				lua_newtable(L);
				lua_pushstring(L, "name");
				lua_pushstring(L, ent->d_name);
				lua_rawset(L, -3);
				lua_rawset(L, -3);
			}

			closedir(dir);
		}
	}

	delete[] path;
	return 1;
}

s32 CKLBLuaLibASSET::luaRegisterNotFound(lua_State* L)
{
	CKLBAssetManager::getInstance().setAssetNotFound(luaL_checklstring(L, 1, NULL));

	return 0;
}

s32 CKLBLuaLibASSET::luaSetPlaceHolder(lua_State* L)
{
	
	CKLBAssetManager::getInstance().setPlaceHolder(luaL_checklstring(L, 1, NULL));
	return 0;
}

void CKLBLuaLibASSET::cmdGetImageSize(const char* asset_name, s32* pReturnWidth, s32* pReturnHeight)
{
	u32 handle;
	CKLBImageAsset * pAsset = (CKLBImageAsset *)CKLBUtility::loadAssetScript( asset_name, &handle);
	if(pAsset)
	{
		SKLBRect * rect = pAsset->getSize();

		*pReturnWidth = rect->getWidth();
		*pReturnHeight = rect->getHeight();

		CKLBDataHandler::releaseHandle(handle);
	}
}

void CKLBLuaLibASSET::cmdGetBoundSize(const char* asset_name, float* pReturnWidth, float* pReturnHeight)
{
	u32 handle;
	CKLBImageAsset * pAsset = (CKLBImageAsset *)CKLBUtility::loadAssetScript( asset_name, &handle);
	if(pAsset)
	{
		*pReturnWidth = pAsset->m_boundWidth;
		*pReturnHeight = pAsset->m_boundHeight;

		CKLBDataHandler::releaseHandle(handle);
	}
}

void CKLBLuaLibASSET::cmdGetAssetInfo(const char* asset_name, s32* pReturnImgWidth, s32* pReturnImgHeight, float* pReturnBoundWidth, float* pReturnBoundHeight)
{
	u32 handle;
	CKLBImageAsset * pAsset = (CKLBImageAsset *)CKLBUtility::loadAssetScript( asset_name, &handle);
	if(pAsset) {
		SKLBRect * rect = pAsset->getSize();
		
		*pReturnImgWidth = rect->getWidth();
		*pReturnImgHeight = rect->getHeight();
		*pReturnBoundWidth = pAsset->m_boundWidth;
		*pReturnBoundHeight = pAsset->m_boundHeight;

		CKLBDataHandler::releaseHandle(handle);
	}
}

int get_asset_path_if_not_found(lua_State* L)
{
	CLuaState lua(L);
	IPlatformRequest& ir = CPFInterface::getInstance().platform();

	lua.print_stack();

	const char* asset_path = lua.getString(1);
	char* asset_newpath = new char[strlen(asset_path) + 14];

	if(strstr(asset_path, ".mp3") || strstr(asset_path, ".ogg"))
	{
		klb_assertAlways("ASSET_getAssetPathIfNotExist : Never use a .ogg or .mp3 extension. Audio Asset have none, automatically detected inside.");
	}
	
	sprintf(asset_newpath, "asset://%s", asset_path);

	// Test asset path
	{
		char mp3_path[MAX_PATH];
		const char* path;
		bool temp;

		if(false)
		{
			asset_ok:
			delete[] path;
			lua.retNil();
			goto return_end;
		}

		if(path = ir.getFullPath(asset_newpath, &temp)) goto asset_ok;

		sprintf(mp3_path, "%s.mp3", asset_newpath);

		if(path = ir.getFullPath(mp3_path, &temp)) goto asset_ok;

		memset(mp3_path, 0, 4);
		sprintf(mp3_path, "%s.ogg", asset_newpath);

		if(path = ir.getFullPath(mp3_path, &temp)) goto asset_ok;
	}

	lua.retString(asset_newpath);

	return_end:
	delete[] asset_newpath;

	return 1;
}

/* args:
 1. callback
 2. file target
 3. download link
*/
int asset_start_download(lua_State* L)
{
	CLuaState lua(L);

	lua.print_stack();
	MicroDownload::Queue(lua.getString(1), lua.getString(2), lua.getString(3));
	lua.retBool(true);

	return 1;
}

int asset_kill_download(lua_State* L)
{
	CLuaState lua(L);

	DEBUG_PRINT("ASSET_killDownload");
	lua.print_stack();
	MicroDownload::DeleteAll();

	return 0;
}

s32
CKLBLuaLibASSET::luaGetNMAssetSize(lua_State* L)
{
	DEBUG_PRINT("ASSET_GetNMAssetSize");
	CLuaState lua(L);
	lua.print_stack();
	int count = lua.getInt(1);
	char* result = (char*)malloc(count + 1);
	for (int i = 0; i < count; i++) {
		result[i] = rand() % 60 + 'A';
	}
	result[count] = '\0';
	pirntHex(result, count);
	assetSize = count;
	memcpy(aes_key_client, result, 32);
	lua.retString(result);
	return 1;
}

s32
CKLBLuaLibASSET::luaGetNMAsset(lua_State* L)
{
	DEBUG_PRINT("ASSET_GetNMAsset");
	CLuaState lua(L);
	lua.print_stack();
	char key_base[] = "eit4Ahph4aiX4ohmephuobei6SooX9xo";
	lua.retString(key_base);
	return 1;
}

s32
CKLBLuaLibASSET::luaSetNMAsset(lua_State* L)
{
	DEBUG_PRINT("ASSET_SetNMAsset");
	CLuaState lua(L);
	lua.print_stack();
	const char* a = lua.getString(1);
	const char* b = lua.getString(2);
	char *result = (char *)malloc(assetSize + 1);
	for (int i = 0; i < assetSize; i++) {
		result[i] = a[i] ^ b[i];
	}
	result[assetSize] = 0;
	memcpy(sessionKey, result, assetSize);
	lua.retString(result);
	return 1;
}

s32
CKLBLuaLibASSET::checkUncompleteUnzip(lua_State* L)
{
	DEBUG_PRINT("checkUncompleteUnzip");
	CLuaState lua(L);
	lua.print_stack();
	return 0;
}
