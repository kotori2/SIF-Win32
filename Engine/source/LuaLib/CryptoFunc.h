#ifndef _CRYPTO_SIF_Win32
#define _CRYPTO_SIF_Win32

#include "ILuaFuncLib.h"

class CRYPTOFunc: public ILuaFuncLib
{
private:
	CRYPTOFunc();
public:
	CRYPTOFunc(DEFCONST* );
	virtual ~CRYPTOFunc();
	
	void addLibrary();
	
	// TODO more
	static bool AES128_Decrypt(const char* Ciphertext, size_t CipherLength, const char* Key, size_t KeyLength, char* Out, int &OutLength);
	static int AES128_Decrypt_Lua(lua_State* L);
};
#endif
