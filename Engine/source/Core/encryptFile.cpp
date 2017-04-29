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
// ---------------------------------------------------------------
#include <string.h>
#include "encryptFile.h"
#include "CPFInterface.h"
#include "HonokaMiku/DecrypterContext.h"

/*
 * To decrypt SIF files regardless of the encryption type, libHonoka (HonokaMiku C89) is used
 */
CDecryptBaseClass::CDecryptBaseClass()
: m_decrypt	(false)
, m_useNew	(false)
, m_header_size(0)
{
	// User context.
	// m_userCtx.members = initValue
}

CDecryptBaseClass::~CDecryptBaseClass()
{
	delete m_dctx;
}

/*!
    @brief  複合化
    @param[in]  void* ptr       暗号化されたデータ
    @param[in]  u32 length      データの長さ
    @return     void
 */
void CDecryptBaseClass::decrypt(void* ptr, u32 length) {
	if(m_dctx)
		m_dctx->decrypt_block(ptr, length);
}

// Uses part of HonokaMiku 
u32 CDecryptBaseClass::decryptSetup(const u8* ptr, const u8* hdr) {
	m_dctx = HonokaMiku::FindSuitable((const char*)ptr, hdr);
	if(m_dctx == NULL)
	{
		DEBUG_PRINT("%s: unsupported encryption scheme", ptr);
		
		m_useNew = false;
		m_decrypt = false;
		m_header_size = 0;

		return 0;
	}
	if(m_dctx->version >= 3)
	{
		if(ptr)
		m_header_size = 16;
		m_dctx->final_setup((const char*)ptr, hdr + 4);
	}
	else
		m_header_size = 4;

	m_useNew = true;
	m_decrypt = true;

	return 1;
}

void CDecryptBaseClass::gotoOffset(u32 offset) {
	// Recompute and update your encryption context if we jump at a certain position into the encrypted stream.
	// gotoOffset is ALWAYS called BEFORE decrypt if a jump in the decoding stream occurs.
	if(m_decrypt)
		m_dctx->goto_offset(offset);
}
