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

bool CRYPTOFunc::AES128_Decrypt(const char* CipherText, size_t CipherLength, const char* Key, size_t KeyLength, char* Out, int& OutLength)
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

	// The first 16 bytes in CyperText is actually initialization vector.
	// The CipherLength is the actual length of the ciphered text, aka strlen(CipherText) - 16,
	// so we can use this length directly.

	// In the EVP_DecryptInit_ex the fifth param is IV, so we pass in the CipherText
	if (EVP_DecryptInit_ex(evp, EVP_aes_128_cbc(), NULL, (const u8*)Key, (const u8*)CipherText) == 0)
		goto evp_fail;

	// In the EVP_DecryptUpdate the forth param is ciphered text, so we pass in the CipherText + 16
	if(EVP_DecryptUpdate(evp, (u8*)Out, &OutLength, (const u8*)CipherText + 16, CipherLength) == 0)
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
	int plaintext_length = 0;
	size_t padding_length = 0;

	ciphertext = luaL_checklstring(L, 1, &cipher_length);
	key = luaL_checklstring(L, 2, &key_length);
	plaintext = new char[cipher_length + 1];

	memset(plaintext, 0, cipher_length + 1);


	if(AES128_Decrypt(ciphertext, cipher_length, key, key_length, plaintext, plaintext_length) == false)
		lua_pushnil(L);
	else
	{
		padding_length = *(plaintext + plaintext_length - 1);
		lua_pushlstring(L, plaintext, plaintext_length - padding_length);
	}

	delete[] plaintext;
	return 1;
}
