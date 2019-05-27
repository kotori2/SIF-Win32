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
#include <string>

#include "CKLBLuaLibCONV.h"
#include "CKLBUtility.h"
#include <stdexcept>

static CKLBLuaLibCONV libdef(0);

// Source: http://stackoverflow.com/a/5291537
static const char b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char reverse_table[128] = {
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
   64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
   64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64
};

std::string base64_decode(const std::string &ascdata)
{
   using ::std::string;
   string retval;
   const string::const_iterator last = ascdata.end();
   int bits_collected = 0;
   unsigned int accumulator = 0;

   for (string::const_iterator i = ascdata.begin(); i != last; ++i) {
      const int c = *i;
      if (c == ' ' || c == '=') {
         // Skip whitespace and padding. Be liberal in what you accept.
         continue;
      }
      if ((c > 127) || (c < 0) || (reverse_table[c] > 63)) {
         throw std::invalid_argument("This contains characters not legal in a base64 encoded string.");
      }
      accumulator = (accumulator << 6) | reverse_table[c];
      bits_collected += 6;
      if (bits_collected >= 8) {
         bits_collected -= 8;
         retval += (char)((accumulator >> bits_collected) & 0xffu);
      }
   }
   return retval;
}

CKLBLuaLibCONV::CKLBLuaLibCONV(DEFCONST * arrConstDef) : ILuaFuncLib(arrConstDef) {}
CKLBLuaLibCONV::~CKLBLuaLibCONV() {}

int LuaBase64Decode(lua_State* L);

// Lua関数の追加
void
CKLBLuaLibCONV::addLibrary()
{
	addFunction("CONV_Lua2Json",     CKLBLuaLibCONV::lua2json);
	addFunction("CONV_Json2Lua",     CKLBLuaLibCONV::json2lua);
	addFunction("CONV_JsonFile2Lua", CKLBLuaLibCONV::jsonfile2lua);
	addFunction("CONV_base64_decode",LuaBase64Decode);
}

int LuaBase64Decode(lua_State* L)
{
	std::string decoded = base64_decode(luaL_checkstring(L, 1));

	lua_pushlstring(L, decoded.c_str(), decoded.length());
	return 1;
}

int
CKLBLuaLibCONV::lua2json(lua_State * L)
{
	CLuaState lua(L);

	lua.retValue(1);
	u32 size; // Ignored here.
	const char * json = CKLBUtility::lua2json(lua,size);
	lua.pop(1);
	lua.retString(json);
	KLBDELETEA(json);
	return 1;
}

int
CKLBLuaLibCONV::json2lua(lua_State * L)
{
	CLuaState lua(L);
	const char * json = lua.getString(1);
	CKLBUtility::json2lua(lua, json, strlen(json));
	return 1;
}

int
CKLBLuaLibCONV::jsonfile2lua(lua_State * L)
{
	CLuaState lua(L);

	const char * asset = lua.getString(1);
	IReadStream * pStream;

	IPlatformRequest& pltf = CPFInterface::getInstance().platform();
	pStream = pltf.openReadStream(asset, pltf.useEncryption());
	if(!pStream || pStream->getStatus() != IReadStream::NORMAL) {
		delete pStream;
		lua.retNil();
		return 1;
	}
	int size = pStream->getSize();
	u8 * buf = KLBNEWA(u8, size + 1);
	if(!buf) {
		delete pStream;
		lua.retNil();
		return 1;
	}
	pStream->readBlock((void *)buf, size);
	delete pStream;

	buf[size] = 0;
	const char * json = (const char *)buf;

	CKLBUtility::json2lua(lua, json, size);
	KLBDELETEA(buf);

	return 1;
}
