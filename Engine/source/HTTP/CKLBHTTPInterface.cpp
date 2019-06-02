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
#include "CKLBHTTPInterface.h"
#include "CKLBUtility.h"
#include <string.h>
#include <ctype.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
;

#ifdef USE_NEW_CURL_WRAPPER

#include "curl.h"

// Prototypes
int strncmpi(const char* str1, const char* str2, int len);

// static
bool CKLBHTTPInterface::initHTTPLib()
{
	return curl_global_init(CURL_GLOBAL_ALL) == CURLE_OK;
}

// static
void CKLBHTTPInterface::releaseHTTPLib() {
	curl_global_cleanup();
}

//_______________________________________________________________________
//  Thread
//_______________________________________________________________________

// static
s32 CKLBHTTPInterface::HTTPConnectionThread(void * /*hThread*/, void * data) 
{
	((CKLBHTTPInterface*)data)->download();
	return 1;
}

extern char sessionKey[64];
extern char* XMC_Force;

#ifdef _M_IX86
#elif _M_AMD64
#else
#error Unsupported arch
#endif
void CKLBHTTPInterface::download() {
	m_threadStop = 0;
	m_pCurl      = curl_easy_init();
	if(m_pCurl)
	{
		curl_slist* headerlist = NULL;
		if (m_post) {
			headerlist = curl_slist_append(headerlist, "Expect:");
		}

		for (u32 n = 0; n < m_headerEntryCount; n++) {
			printf("Hdr : %s\n",m_headerEntry[n]);
			headerlist = curl_slist_append(headerlist, m_headerEntry[n]);
		}

		curl_httppost* formpost = NULL;
		curl_httppost* lastptr  = NULL;
		if (m_post && m_postForm) {
			for(u32 i = 0; m_postForm[i]; i++) {
				char * formItem = (char*)m_postForm[i]; // We need to put back as writable.

				// split into two strings.
				// Search for first "=" and patch with 0.
				char * ptr = formItem;
				while (*ptr != 0 && *ptr != '=') { ptr++; }

				if (*ptr != 0) {
					size_t content_len = strlen(&ptr[1]);
					*ptr = 0;

					// DEBUG_PRINT("Form : %s = %s\n",formItem,&ptr[1]);
					fputs("Form : ", stdout);
					fputs(formItem, stdout);
					fputs(" = ", stdout);
					puts(&ptr[1]);

					curl_formadd(
						&formpost,
						&lastptr,
						CURLFORM_COPYNAME, formItem,
						CURLFORM_CONTENTSLENGTH, content_len,
						CURLFORM_COPYCONTENTS, &ptr[1],
						CURLFORM_END
					);

					if(strncmpi(formItem, "request_data", 12) == 0)
					{
						// Create X-Message-Code header
						u8* hash;
						char temp[41];
						char xmc[64];

						
						hash = HMAC(EVP_sha1(), sessionKey, 32, reinterpret_cast<const u8*>(ptr + 1), content_len, NULL, NULL);
						for(int i = 0; i < 20; i++)
						{
							temp[i * 2 + 1] = 32;
							sprintf(temp + i * 2, "%02x", hash[i]);
						}
						DEBUG_PRINT("Calculated X-Message-Code: %s", temp);
						sprintf(xmc, "X-Message-Code: %s", temp);

						curl_slist_append(headerlist, xmc);
					}
					// Restore array in case of reuse...
					*ptr = '=';
				}
			}
		}

		curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headerlist);
		if (m_post) {
			curl_easy_setopt(m_pCurl, CURLOPT_HTTPPOST, formpost);
		}

		curl_easy_setopt(m_pCurl, CURLOPT_URL,				m_url			);
		curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA,		(void*)this		);
		curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION,	write_func		);
//		curl_easy_setopt(m_pCurl, CURLOPT_READFUNCTION,		my_read_func	);
		curl_easy_setopt(m_pCurl, CURLOPT_NOPROGRESS,		0L				);
		curl_easy_setopt(m_pCurl, CURLOPT_NOSIGNAL,			1				);
		curl_easy_setopt(m_pCurl, CURLOPT_PROGRESSFUNCTION,	progress_func	);
		curl_easy_setopt(m_pCurl, CURLOPT_PROGRESSDATA,		(void*)this		);
		curl_easy_setopt(m_pCurl, CURLOPT_WRITEHEADER,		(void*)this		);
 		curl_easy_setopt(m_pCurl, CURLOPT_HEADERFUNCTION,	headerReceive_func);
		// curl_easy_setopt(m_pCurl, CURLOPT_ACCEPT_ENCODING,	"gzip,deflate"); // I'm too lazy to decompress it later
		CURLcode res = curl_easy_perform(m_pCurl);
		if (res == CURLE_OK) {
			curl_easy_getinfo (m_pCurl, CURLINFO_RESPONSE_CODE, &m_errorCode);
			// WARNING : IN THAT ORDER, because of multithreading, flag set LAST, after everything else.
			m_receivedData	= m_buffer;
			m_receivedSize	= m_writeIndex;
			m_bDataComplete = true;
		} else {
			// printf("HTTP FAIL\n");
			
			// In some cases, the server cut the connection, resulting in a CURL error
			// But there is 0 byte of data and the error code is valid.
			// In this case, we allow the upper layer to consider returning a safe error code.
			if ((this->m_receivedSize == 0) && (
				((m_tmpErrorCode >= 500) && (m_tmpErrorCode <= 599)) || (m_tmpErrorCode == 204)
				)) {
				m_errorCode = m_tmpErrorCode;
			}
		}

		if (formpost) {
			curl_formfree(formpost);
		}

		if (headerlist) {
			curl_slist_free_all(headerlist);
		}

		// always cleanup
		curl_easy_cleanup(m_pCurl);
		m_pCurl = NULL;

		if (m_bDownload) {
			// Close file anyway
			if (m_pTmpFile) {
				delete m_pTmpFile; // No macro, get alloc from porting layer.
				m_pTmpFile = NULL;
			}
		}
	}
	m_threadStop = 1;
}

int strncmpi(const char* str1, const char* str2, int len) {
	while ((*str1 != 0) && (*str2 != 0) && (tolower(*str1++)) == (tolower(*str2++)) && (len-- != 0)) {
	}

	if ((len == 0) || ((*str1 == 0) && (*str2 == 0))) {
		return 0;
	} else {
		str1--;
		str2--;
		return tolower(*str1) < tolower(*str2) ? -1 : +1; 
	}
}

// static
size_t CKLBHTTPInterface::headerReceive_func( void *ptr, size_t size, size_t nmemb, void *userdata) {
	u32 totalSize = size * nmemb;
	const char* data = (const char*)ptr;
	if (strncmpi("Maintenance:", data, 12/*Maintenance: 1*/) == 0) {
		data+=12; // Skip Maintenance
		while ((*data != '\r') && (*data != '\n') && (*data != 0)) {
			if (*data == '1') {
				((CKLBHTTPInterface*)userdata)->m_maintenance = true;
				break;
			}
			data++;
		}
	}

	if (strncmpi("status_code: ",data, 13) == 0) {
		int code = 0;
		while ((*data >= '0') && (*data <= '9')) {
			code = (code * 10) + (*data);
			data++;
		}
		((CKLBHTTPInterface*)userdata)->m_tmpErrorCode = code;
	}

	if (strncmpi("version_up: ", data, 12) == 0)
	{
		((CKLBHTTPInterface*)userdata)->m_versionup = data[12] == '1';
	}

	if (strncmpi("Server-Version:", data, 15/*Server-Version*/) == 0) {
		u32 lineSize = size * nmemb;	// Full Size
		lineSize -= 15;					// Remove Server-Version
		data += 15;
		
		// skip : and space before and after
		while (*data == ' ') {
			data++;
			lineSize--;
		}

		char* mem =	(char*)CKLBUtility::copyMem(data, lineSize + 1);
		if (mem) {
			mem[lineSize] = 0;
			((CKLBHTTPInterface*)userdata)->m_pServerVersion = mem;
			IPlatformRequest& pForm = CPFInterface::getInstance().platform();
			pForm.logging("HTTPInterface::get Server-Version %s %8X",mem);

		}
	}
	if (!((CKLBHTTPInterface*)userdata)->m_stopThread) {
		return totalSize;
	} else {
		return 0xFFFFFFFF;
	}
}

bool CKLBHTTPInterface::hasHeader(const char* header, const char** value) {
	if (strcmp("Server-Version", header) == 0) {
		// IPlatformRequest& pForm = CPFInterface::getInstance().platform();
		// pForm.logging("HTTPInterface::hasHeader %s %8X",header,m_pServerVersion);
		if (value) {
			// pForm.logging("Value:%s",m_pServerVersion);
			*value = m_pServerVersion;
		}
		return (m_pServerVersion != NULL);
	} else {
		klb_assertAlways("Does not support other header for now than 'Server-Version'");
		return false;
	}
}


// static
int CKLBHTTPInterface::progress_func(	void* ctx, 
										double total,		// dltotal
										double dl,			// dlnow
										double /*ultotal*/,	
										double /*ulnow*/)
{
	u64 uiTotal = (u64)total;
	u64 uiDownl = (u64)dl;
	((CKLBHTTPInterface*)ctx)->progress(uiTotal, uiDownl);
	if (!((CKLBHTTPInterface*)ctx)->m_stopThread) {
		return 0;
	} else {
		return -1;
	}
}

void CKLBHTTPInterface::progress(u64 /*total*/, u64 download) {
	m_receivedSize = download;
}

// static
size_t CKLBHTTPInterface::write_func(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	return ((CKLBHTTPInterface*)userdata)->write(ptr, size, nmemb);
}

size_t CKLBHTTPInterface::write(char* ptr, size_t size, size_t nmemb)
{
	bool noErr = false;

	u64 blockSize   = size * nmemb;
	u64 oldByteSize = m_writeIndex;
	u64 newByteSize = oldByteSize + blockSize;	// Optimize with fixed size allocation ?

	if (m_pTmpFile && (m_bothFileAndMem == false)) {
		if (m_pTmpFile->writeTmp(ptr, blockSize) == blockSize) {
			noErr = true;
		}
	} else {
		u8* pNewBuff = KLBNEWA(u8, newByteSize);
		if (pNewBuff) {
			if (m_buffer) {
				memcpy(pNewBuff, m_buffer, oldByteSize);
				KLBDELETEA(m_buffer);
			}
			memcpy(&pNewBuff[oldByteSize],	ptr, blockSize);
			m_buffer		= pNewBuff;
			m_writeIndex	= newByteSize;
			if (m_pTmpFile) {
				if (m_pTmpFile->writeTmp(ptr, blockSize) == blockSize) {
					noErr = true;
				}
			} else {
				noErr = true;
			}
		}
	}

	if (noErr) {
		return blockSize;
	} else {
		return 0;
	}
}

//_______________________________________________________________________
//  Object
//_______________________________________________________________________

CKLBHTTPInterface::CKLBHTTPInterface()
: m_errorCode       (-1)
, m_bDataComplete   (false)
, m_bDownload       (false)
, m_pTmpFile        (NULL)
, m_receivedSize    (0)
, m_writeIndex      (0)
, m_receivedData    (NULL)
, m_thread          (NULL)
, m_buffer          (NULL)
, m_bothFileAndMem  (true)
, m_headers         (NULL)
, m_headerEntry     (NULL)
, m_headerEntryLen  (NULL)
, m_hdrlen          (0)
, m_headerEntryCount(0)
, m_post            (false)
, m_url             (NULL)
, m_pCurl           (NULL)
, m_postForm        (NULL)
, m_pServerVersion  (NULL)
, m_maintenance     (false)
, m_threadStop      (0)
, m_stopThread      (false)
, m_tmpErrorCode    (-1)
{
	init();
}

// virtual
CKLBHTTPInterface::~CKLBHTTPInterface()
{
	IPlatformRequest& pForm = CPFInterface::getInstance().platform();

	// True : thread still working
	bool result = false;
 	if (m_thread) {
		int status;
		result = pForm.watchThread(m_thread, &status);
		m_stopThread = true;
		while (m_threadStop == 0) {
		}
	}

	// May use result here...

	if (m_thread) {
		pForm.deleteThread(m_thread);
		m_thread = NULL;
	}

	/* NEVER DO THAT, thread may be still alive.
	if (m_pCurl) {
		curl_easy_cleanup(m_pCurl);	
		m_pCurl = NULL;
	} */

	clear();
}

// 追加HTTPヘッダの持
bool CKLBHTTPInterface::setHeader(const char ** headers)
{
	m_headerEntryCount = 0;
	while (headers[m_headerEntryCount++]) { }
	m_headerEntryCount--;

	if (m_headerEntryCount) {
		m_headerEntry		= KLBNEWA(const char*, m_headerEntryCount);
		m_headerEntryLen	= KLBNEWA(u32, m_headerEntryCount);
        if ((!m_headerEntry) || (!m_headerEntryLen)) { return false; }

		int len = 1;	// 終端コードサイズは先に設宁E
		for(int i = 0; headers[i]; i++) {
			m_headerEntryLen[i] = strlen(headers[i]);
			len += m_headerEntryLen[i] + 2;
		}

		char * buf = KLBNEWA(char, len);
		if(!buf) return false;

		KLBDELETEA(m_headers);

		m_headers = buf;

		for(int i = 0; headers[i]; i++) {
			len = m_headerEntryLen[i];
			strcpy(buf, headers[i]);
			m_headerEntry[i] = buf;
			buf += len;
//			strcpy(buf, "\r\n"); Replace the chars by end of string.
			*buf++ = 0;
		}
		*buf = 0;

		m_hdrlen = buf - m_headers;
	}

	return true;
}

// 持フォーム値のURLencode
char * CKLBHTTPInterface::setForm(const char ** postForm)
{
	// DEBUG_PRINT("HTTPInterface::setForm");

	// 持されpostForm から、POST斁を生成する
	// postForm は URLencodeされておらず個頁が連結されて態
	/*
	char * basebuf = retbuf;
	if(!basebuf) return NULL;
	*/

	u32 i = 0;
	while (postForm[i++]) { }

	m_postForm = KLBNEWA(const char*, i);

	if (m_postForm) {
		for(u32 n = 0; postForm[n]; n++) {
			const char * formItem = postForm[n];
			m_postForm[n] = CKLBUtility::copyString(formItem);
		}
		m_postForm[i-1] = NULL;
	}

	return NULL;
}

void CKLBHTTPInterface::reuse() {
	// DEBUG_PRINT("HTTPInterface::reuse");
	clear();
	init();
}

void CKLBHTTPInterface::init() {
	m_errorCode         = -1;
	m_bDataComplete	    = false;
	m_bDownload         = false;
	m_pTmpFile          = NULL;
	m_receivedSize      = 0;
	m_writeIndex        = 0;
	m_receivedData      = NULL;
	m_thread            = NULL;
	m_buffer            = NULL;
	m_bothFileAndMem    = true;
	m_headers           = NULL;
	m_headerEntry       = NULL;
	m_headerEntryLen    = NULL;
	m_hdrlen            = 0;
	m_headerEntryCount  = 0;
	m_post              = false;
	m_url               = NULL;
	m_pCurl             = NULL;
	m_postForm          = NULL;
	m_pServerVersion    = NULL;
	m_maintenance       = false;
	m_threadStop        = 0;
	m_stopThread        = false;
}

void CKLBHTTPInterface::clear() {
	// Force file closing if necessary.
	delete m_pTmpFile;
	m_pTmpFile = NULL;

	KLBDELETEA(m_url);
	KLBDELETEA(m_buffer);
	KLBDELETEA(m_headers);
	KLBDELETEA(m_headerEntry);
	KLBDELETEA(m_headerEntryLen);
	KLBDELETEA(m_pServerVersion);

	if (m_postForm) {
		u32 i = 0;
		while (m_postForm[i]) {
			KLBDELETEA(m_postForm[i]);
			i++;
		}
		KLBDELETEA(m_postForm);
		m_postForm = NULL;
	}

	m_url				= NULL;
	m_buffer			= NULL;
	m_headers			= NULL;
	m_headerEntry		= NULL;
	m_headerEntryLen	= NULL;
	m_pServerVersion	= NULL;

	init();
}

// GET発衁
bool CKLBHTTPInterface::httpGET(const char * url, bool isProxy = false)
{
	klb_assert(isProxy==false,"Proxy Not supported");

	m_post	= false;
	KLBDELETEA(m_url);
	m_url = CKLBUtility::copyString(url);

    IPlatformRequest& pForm = CPFInterface::getInstance().platform();
	//pForm.logging("HTTPInterface::httpGET");
	m_thread = pForm.createThread(CKLBHTTPInterface::HTTPConnectionThread, this);
	return (m_thread != NULL);
}

// POST発衁
bool CKLBHTTPInterface::httpPOST(const char * url, bool isProxy = false)
{
	klb_assert(isProxy==false,"Proxy Not supported");

	m_post = true;
	KLBDELETEA(m_url);
	m_url = CKLBUtility::copyString(url);

    IPlatformRequest& pForm = CPFInterface::getInstance().platform();
	// pForm.logging("HTTPInterface::httpPost");
	m_thread = pForm.createThread(CKLBHTTPInterface::HTTPConnectionThread, this);
	return (m_thread != NULL);
}

// ダウンロード保存パス名を持し、ダウンロードモードでの動作を開始する
// NULL持で通常のオンメモリモードでの動作に戻めE
bool CKLBHTTPInterface::setDownload(const char * path) 
{
	m_bDownload = false;
	if(path) {
		// ダウンロードファイル名 file://external/ 以下パスで与えられねばならな
		m_pTmpFile = CPFInterface::getInstance().platform().openTmpFile(path);
		if(m_pTmpFile) {
			m_bDownload = true;
		}
	}
	return m_bDownload; 
}

// 受信リソースの取征
u8* CKLBHTTPInterface::getRecvResource() {
	return m_receivedData;
}

// 現在の受信サイズ
s64 CKLBHTTPInterface::getSize()
{
	return m_receivedSize;
}

// 受信スチEタス
bool CKLBHTTPInterface::httpRECV()
{
	return m_bDataComplete;
}

// httpのstate取征2013.2.13  追加
int CKLBHTTPInterface::getHttpState()
{
	return m_errorCode;
}

#else

#endif
