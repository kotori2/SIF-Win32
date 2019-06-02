﻿/* 
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
#ifndef CKLBLuaLibASSET_h
#define CKLBLuaLibASSET_h

#include "ILuaFuncLib.h"

class CKLBLuaLibASSET : public ILuaFuncLib
{
private:
	CKLBLuaLibASSET();
public:
	CKLBLuaLibASSET(DEFCONST * arrConstDef);
	virtual ~CKLBLuaLibASSET();

	void addLibrary();

	static void cmdGetImageSize	(const char* asset_name, s32* pReturnWidth, s32* pReturnHeight);
	static void cmdGetBoundSize	(const char* asset_name, float* pReturnWidth, float* pReturnHeight);
	static void cmdGetAssetInfo	(const char* asset_name, s32* pReturnImgWidth, s32* pReturnImgHeight, float* pReturnBoundWidth, float* pReturnBoundHeight);
private:
	static s32	luaGetImageSize		(lua_State * L);
	static s32	luaGetBoundSize		(lua_State * L);
	static s32	luaGetAssetInfo		(lua_State * L);
	static s32	luaDelExternal		(lua_State * L);
	static s32	luaGetExternalFree	(lua_State * L);
	static s32	luaGetFileList		(lua_State * L);
	static s32	luaRegisterNotFound	(lua_State * L);
	static s32	luaSetPlaceHolder	(lua_State * L);
	static s32	luaGetNMAssetSize   (lua_State * L);
	static s32	luaGetNMAsset       (lua_State * L);
	static s32	luaSetNMAsset       (lua_State * L);
	static s32	checkUncompleteUnzip(lua_State * L);
	static s32	enableTextureBorderPatch(lua_State * L);
};


#endif // CKLBLuaLibASSET_h
