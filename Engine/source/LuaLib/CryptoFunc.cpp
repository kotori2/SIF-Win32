#include "CryptoFunc.h"

#include <openssl/aes.h>
#include <openssl/evp.h>

static ILuaFuncLib::DEFCONST luaConst[] = {
	{0, 0}
};

static CRYPTOFunc libdef(luaConst);

CRYPTOFunc::CRYPTOFunc(DEFCONST* a): ILuaFuncLib(a) {}
CRYPTOFunc::~CRYPTOFunc() {}

void CRYPTOFunc::addLibrary()
{
	addFunction("CRYPTO_decrypt_aes_128_cbc", CRYPTOFunc::AES128_Decrypt_Lua);
}

bool CRYPTOFunc::AES128_Decrypt(const char* Ciphertext, size_t CipherLength, const char* Key, size_t KeyLength, char* Out, int& OutLength)
{
	EVP_CIPHER_CTX* evp;

	if(!(evp = EVP_CIPHER_CTX_new()))
		return false;

	if(false)
	{
		evp_fail:
		EVP_CIPHER_CTX_free(evp);
		return false;
	}

	if(EVP_DecryptInit_ex(evp, EVP_aes_128_cbc(), NULL, (const u8*)Key, NULL) == 0)
		goto evp_fail;

	if(EVP_DecryptUpdate(evp, (u8*)Out, &OutLength, (const u8*)Ciphertext, CipherLength) == 0)
		goto evp_fail;

	EVP_CIPHER_CTX_free(evp);
	return true;
}

int CRYPTOFunc::AES128_Decrypt_Lua(lua_State* L)
{
	const char* ciphertext  = NULL;
	const char* key = NULL;
	char* plaintext = NULL;
	size_t cipher_length = 0;
	size_t key_length = 0;
	int plaintext_len = 0;

	ciphertext = luaL_checklstring(L, 1, &cipher_length);
	key = luaL_checklstring(L, 2, &key_length);
	plaintext = new char[cipher_length + 1];

	memset(plaintext, 0, cipher_length + 1);

	if(AES128_Decrypt(ciphertext, cipher_length, key, key_length, plaintext, plaintext_len) == false)
		lua_pushnil(L);
	else
		lua_pushlstring(L, plaintext, plaintext_len);

	delete[] plaintext;
	return 1;
}
