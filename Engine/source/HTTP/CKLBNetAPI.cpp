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
#include "CKLBNetAPI.h"
#include "CKLBLuaEnv.h"
#include "CKLBUtility.h"
#include "CKLBJsonItem.h"
#include "CPFInterface.h"
#include "CKLBNetAPIKeyChain.h"
#include "SIF_Win32.h"
#include <time.h>
#include <ctype.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#pragma comment(lib, "libeay32.lib")

;

static int fail_times = 0;

bool authkey = false;

char iv[32] = "";
extern char sessionKey[32];
extern RSA * RSA_Public_Key;
extern char aesKeyClient[32];
char sessionKey_[32];
bool sp = false;

enum {
	// Command Values
	NETAPI_STARTUP,				// start new account
	NETAPI_LOGIN,				// login to account
	NETAPI_LOGOUT,				// possibility unused
	NETAPI_SEND,				// send JSON packet
	NETAPI_CANCEL,				// selected session cancel
	NETAPI_CANCEL_ALL,			// abort all connection
	NETAPI_WATCH_MAINTENANCE,	// check for maintenance?
	NETAPI_DEBUG_HDR,			// unknwon
	NETAPI_GEN_CMDNUMID,		// create commandNum string in data. possibility unused
};

static IFactory::DEFCMD cmd[] = {
	{ "NETAPI_STARTUP",					NETAPI_STARTUP },
	{ "NETAPI_LOGIN",					NETAPI_LOGIN },
	{ "NETAPI_LOGOUT",					NETAPI_LOGOUT },
	{ "NETAPI_SEND",						NETAPI_SEND },
	{ "NETAPI_CANCEL",					NETAPI_CANCEL },
	{ "NETAPI_CANCEL_ALL",				NETAPI_CANCEL_ALL },
	{ "NETAPI_WATCH_MAINTENANCE",		NETAPI_WATCH_MAINTENANCE },
	{ "NETAPI_DEBUG_HDR",				NETAPI_DEBUG_HDR },
	{ "NETAPI_GEN_CMDNUMID",				NETAPI_GEN_CMDNUMID },

	//
	// Callback constants
	//
	{ "NETAPIMSG_CONNECTION_CANCELED",	NETAPIMSG_CONNECTION_CANCELED },
	{ "NETAPIMSG_CONNECTION_FAILED",		NETAPIMSG_CONNECTION_FAILED },
	{ "NETAPIMSG_INVITE_FAILED",			NETAPIMSG_INVITE_FAILED },
	{ "NETAPIMSG_STARTUP_FAILED",		NETAPIMSG_STARTUP_FAILED },
	{ "NETAPIMSG_SERVER_TIMEOUT",		NETAPIMSG_SERVER_TIMEOUT },
	{ "NETAPIMSG_REQUEST_FAILED",		NETAPIMSG_REQUEST_FAILED },
	{ "NETAPIMSG_LOGIN_FAILED",			NETAPIMSG_LOGIN_FAILED },
	{ "NETAPIMSG_SERVER_ERROR",			NETAPIMSG_SERVER_ERROR },
	{ "NETAPIMSG_UNKNOWN",				NETAPIMSG_UNKNOWN },
	{ "NETAPIMSG_LOGIN_SUCCESS",			NETAPIMSG_LOGIN_SUCCESS },
	{ "NETAPIMSG_REQUEST_SUCCESS",		NETAPIMSG_REQUEST_SUCCESS },
	{ "NETAPIMSG_STARTUP_SUCCESS",		NETAPIMSG_STARTUP_SUCCESS },
	{ "NETAPIMSG_INVITE_SUCCESS",		NETAPIMSG_INVITE_SUCCESS },
	{ 0, 0 }
};

static CKLBTaskFactory<CKLBNetAPI> factory("HTTP_API", CLS_KLBNETAPI, cmd);

enum {
	ARG_CALLBACK = 5,
	ARG_REQUIRE = ARG_CALLBACK,
};

int map_netapi_success(int request_type)
{
	switch (request_type)
	{
	case NETAPI_STARTUP:
		return NETAPIMSG_STARTUP_SUCCESS;
	case NETAPI_LOGIN:
		return NETAPIMSG_LOGIN_SUCCESS;
	case NETAPI_SEND:
		return NETAPIMSG_REQUEST_SUCCESS;
	default:
		return NETAPIMSG_UNKNOWN;
	}
}

int map_netapi_fail(int request_type)
{
	switch (request_type)
	{
	case NETAPI_STARTUP:
		return NETAPIMSG_STARTUP_FAILED;
	case NETAPI_LOGIN:
		return NETAPIMSG_LOGIN_FAILED;
	case NETAPI_SEND:
		return NETAPIMSG_REQUEST_FAILED;
	default:
		return NETAPIMSG_SERVER_ERROR;
	}
}

char* create_authorize_string(const char* consumerKey, int nonce, const char* token = NULL)
{
	char* out = KLBNEWA(char, 256);

	if (token)
		sprintf(out, "consumerKey=%s&timeStamp=%d&version=1.1&token=%s&nonce=%d", consumerKey, int(time(NULL)), token, nonce);
	else
		sprintf(out, "consumerKey=%s&timeStamp=%d&version=1.1&nonce=%d", consumerKey, int(time(NULL)), nonce);

	return out;
}

const char base[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
/* Base64 编码 */
char *base64_encode(const char* data, int data_len)
{
	//int data_len = strlen(data);   
	int prepare = 0;
	int ret_len;
	int temp = 0;
	char *ret = NULL;
	char *f = NULL;
	int tmp = 0;
	char changed[4];
	int i = 0;
	ret_len = data_len / 3;
	temp = data_len % 3;
	if (temp > 0)
	{
		ret_len += 1;
	}
	ret_len = ret_len * 4 + 1;
	ret = (char *)malloc(ret_len);

	if (ret == NULL)
	{
		printf("No enough memory.\n");
		exit(0);
	}
	memset(ret, 0, ret_len);
	f = ret;
	while (tmp < data_len)
	{
		temp = 0;
		prepare = 0;
		memset(changed, '\0', 4);
		while (temp < 3)
		{
			//printf("tmp = %d\n", tmp);   
			if (tmp >= data_len)
			{
				break;
			}
			prepare = ((prepare << 8) | (data[tmp] & 0xFF));
			tmp++;
			temp++;
		}
		prepare = (prepare << ((3 - temp) * 8));
		//printf("before for : temp = %d, prepare = %d\n", temp, prepare);   
		for (i = 0; i < 4; i++)
		{
			if (temp < i)
			{
				changed[i] = 0x40;
			}
			else
			{
				changed[i] = (prepare >> ((3 - i) * 6)) & 0x3F;
			}
			*f = base[changed[i]];
			//printf("%.2X", changed[i]);   
			f++;
		}
	}
	*f = '\0';

	return ret;

}

/* 转换算子 */
static char find_pos(char ch)
{
	char *ptr = (char*)strrchr(base, ch);//the last position (the only) in base[]   
	return (ptr - base);
}

/* Base64 解码 */
char *base64_decode(const char *data)
{
	int data_len = strlen(data);
	int ret_len = (data_len / 4) * 3;
	int equal_count = 0;
	char *ret = NULL;
	char *f = NULL;
	int tmp = 0;
	int temp = 0;
	int prepare = 0;
	int i = 0;
	if (*(data + data_len - 1) == '=')
	{
		equal_count += 1;
	}
	if (*(data + data_len - 2) == '=')
	{
		equal_count += 1;
	}
	if (*(data + data_len - 3) == '=')
	{//seems impossible   
		equal_count += 1;
	}
	switch (equal_count)
	{
	case 0:
		ret_len += 4;//3 + 1 [1 for NULL]   
		break;
	case 1:
		ret_len += 4;//Ceil((6*3)/8)+1   
		break;
	case 2:
		ret_len += 3;//Ceil((6*2)/8)+1   
		break;
	case 3:
		ret_len += 2;//Ceil((6*1)/8)+1   
		break;
	}
	ret = (char *)malloc(ret_len);
	if (ret == NULL)
	{
		printf("No enough memory.\n");
		exit(0);
	}
	memset(ret, 0, ret_len);
	f = ret;
	while (tmp < (data_len - equal_count))
	{
		temp = 0;
		prepare = 0;
		while (temp < 4)
		{
			if (tmp >= (data_len - equal_count))
			{
				break;
			}
			prepare = (prepare << 6) | (find_pos(data[tmp]));
			temp++;
			tmp++;
		}
		prepare = prepare << ((4 - temp) * 6);
		for (i = 0; i<3; i++)
		{
			if (i == temp)
			{
				break;
			}
			*f = (char)((prepare >> ((2 - i) * 8)) & 0xFF);
			f++;
		}
	}
	*f = '\0';
	return ret;
}

bool
aesEncrypt(const char* plaintext, size_t plaintextLenght, const char* Key, const char* iv, char* ciphertext, int& ciphertextLenght)
{
	int tmp;
	unsigned char* cipherTemp = (unsigned char*)calloc(1024, sizeof(unsigned char));

	EVP_CIPHER_CTX* ctx;
	if (!(ctx = EVP_CIPHER_CTX_new()))
		return false;

	if (false)
	{
		fail:
			EVP_CIPHER_CTX_free(ctx);
			return false;
	}

	// Encrypt the plaintext
	if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, (u8*)Key, (u8*)iv) == 0)
		goto fail;
	if (EVP_EncryptUpdate(ctx, cipherTemp, &ciphertextLenght, (u8*)plaintext, plaintextLenght) == 0)
		goto fail;
	if (EVP_EncryptFinal_ex(ctx, cipherTemp + ciphertextLenght, &tmp) == 0)
		goto fail;

	ciphertextLenght += tmp;
	EVP_CIPHER_CTX_free(ctx); // Free CTX after using

	memcpy(ciphertext, iv, 16);
	memcpy(ciphertext + 16, cipherTemp, ciphertextLenght);
	ciphertextLenght += 16;
	return true;
}

int get_statuscode(CKLBJsonItem* response)
{
	int status_code = 0;

	while (status_code == 0 && response != NULL)
	{
		if (strcmp(response->key(), "status_code") == 0)
		{
			status_code = response->getInt();
			break;
		}

		response = response->next();
	}

	return status_code;
}

CKLBNetAPI::CKLBNetAPI()
	: CKLBLuaTask()
	, m_http(NULL)
	, m_timeout(30000)
	, m_timestart(0)
	, m_canceled(false)
	, m_pRoot(NULL)
	, m_callback(NULL)
	, m_http_header_array(NULL)
	, m_http_header_length(0)
	, m_request_type(-1)
	, m_nonce(1)
	, m_netapi_phase(0)
	, m_downloading(false)
{
	// Create the header array
}

CKLBNetAPI::~CKLBNetAPI()
{
	// Done in Die()
}

u32
CKLBNetAPI::getClassID()
{
	return CLS_KLBNETAPI;
}

void CKLBNetAPI::startUp(int phase, int status_code)
{
	CKLBNetAPIKeyChain& kc = CKLBNetAPIKeyChain::getInstance();

	// Startup phase
	switch (phase)
	{
		case 0:
		{
			// Do additional request. login/startUp
			char request_data[1024];
			const char* form[2];
			char* authorize = create_authorize_string(kc.getConsumerKey(), m_nonce, kc.getToken());

			// Create request_data
			int loginKeyLen, loginPassLen;
			char loginKey[512] = "";
			char loginPasswd[512] = "";

			// Encrypt login key
			for (int i = 0; i < 16; i++) {
				iv[i] = rand();
			}
			if (aesEncrypt(kc.getLoginKey(), strlen(kc.getLoginKey()), sessionKey, iv, loginKey, loginKeyLen) == false) klb_assertAlways("Error in AES encryption");

			// Encrypt login passwd
			for (int i = 0; i < 16; i++) {
				iv[i] = rand();
			}
			if (aesEncrypt(kc.getLoginPw(), strlen(kc.getLoginPw()), sessionKey, iv, loginPasswd, loginPassLen) == false) klb_assertAlways("Error in AES encryption");

			sprintf(request_data, "request_data={\"login_key\": \"%s\",\"login_passwd\": \"%s\"}", base64_encode(loginKey, loginKeyLen), base64_encode(loginPasswd, loginPassLen));
			form[0] = request_data;
			form[1] = NULL;

			// Reuse m_http
			NetworkManager::releaseConnection(m_http);
			m_http = NetworkManager::createConnection();
			m_http->reuse();
			m_http->setForm(form);
			set_header(m_http, authorize);

			// Send
			char URL[MAX_PATH];

			sprintf(URL, "%s/login/startUp", kc.getURL());
			m_http->httpPOST(URL, false);

			// Set values
			m_netapi_phase++;
			m_timestart = 0;			// Reset

			KLBDELETEA(authorize);
			return;
		}
		case 1:
		{
			lua_callback(NETAPIMSG_STARTUP_SUCCESS, status_code, m_pRoot);
			NetworkManager::releaseConnection(m_http);
			kc.setToken(NULL);

			m_http = NULL;
			m_request_type = (-1);
			m_netapi_phase = 0;
			m_nonce = 1;

			return;
		}
		case 2:
		{
			// Startup OK.
			lua_callback(NETAPIMSG_STARTUP_SUCCESS, status_code, m_pRoot);
			NetworkManager::releaseConnection(m_http);
			kc.setToken(NULL);

			m_http = NULL;
			m_request_type = (-1);
			m_netapi_phase = 0;
			m_nonce = 1;

			return;
		}
	}
}

void CKLBNetAPI::login(int phase, int status_code)
{
	CKLBNetAPIKeyChain& kc = CKLBNetAPIKeyChain::getInstance();
	switch (phase)
	{
		case 0:
		{
			char request_data[512];
			const char* form[2];
			char* authorize = create_authorize_string(kc.getConsumerKey(), m_nonce, kc.setToken(m_pRoot->child()->child()->getString()));

			int loginKeyLen, loginPassLen;
			char loginKey[512] = "";
			char loginPasswd[512] = "";

			// Encrypt login key
			for (int i = 0; i < 16; i++) {
				iv[i] = rand();
			}
			if (aesEncrypt(kc.getLoginKey(), strlen(kc.getLoginKey()), sessionKey, iv, loginKey, loginKeyLen) == false) klb_assertAlways("Error in AES encryption");

			// Encrypt login passwd
			for (int i = 0; i < 16; i++) {
				iv[i] = rand();
			}
			if (aesEncrypt(kc.getLoginPw(), strlen(kc.getLoginPw()), sessionKey, iv, loginPasswd, loginPassLen) == false) klb_assertAlways("Error in AES encryption");

			sprintf(request_data, "request_data={\"login_key\": \"%s\",\"login_passwd\": \"%s\"}", base64_encode(loginKey, loginKeyLen), base64_encode(loginPasswd, loginPassLen));
			form[0] = request_data;
			form[1] = NULL;

			// create new HTTP
			NetworkManager::releaseConnection(m_http);
			m_http = NetworkManager::createConnection();
			m_http->reuse();
			m_http->setForm(form);
			set_header(m_http, authorize);

			// send
			char url[MAX_PATH];
			sprintf(url, "%s/login/login", kc.getURL());
			m_http->httpPOST(url, false);

			// Set values
			m_netapi_phase++;
			m_timestart = 0;			// Reset

			KLBDELETEA(authorize);
			return;
		}
		case 1:
		{
			// Login OK
			char user_id[16];

			// Find status code

			if ((status_code = get_statuscode(m_pRoot->child())) == 200)
			{
				kc.setToken(m_pRoot->child()->child()->getString());	// Authorize token

				sprintf(user_id, "%d", m_pRoot->child()->child()->next()->getInt());
				kc.setUserID(user_id);	// User ID

				NetworkManager::releaseConnection(m_http);	// Release it first before calling lua callback.
				m_http = NULL;
				m_request_type = (-1);
				m_netapi_phase = 0;

				lua_callback(NETAPIMSG_LOGIN_SUCCESS, status_code, m_pRoot, 1);
			}
			else
			{
				NetworkManager::releaseConnection(m_http);	// Release it first before calling lua callback.
				m_http = NULL;
				m_request_type = (-1);
				m_netapi_phase = 0;

				lua_callback(NETAPIMSG_LOGIN_FAILED, 200, m_pRoot, 1);
			}

			return;
		}
	}
}

void CKLBNetAPI::request_authkey(int timeout)
{
	CKLBNetAPIKeyChain& kc = CKLBNetAPIKeyChain::getInstance();
	CKLBHTTPInterface* http = NetworkManager::createConnection();
	http->reuse();

	// Authorize string
	{
		const char* auth = create_authorize_string(kc.getConsumerKey(), m_nonce);
		set_header(http, auth);

		KLBDELETEA(auth);
	}

	// Send
	char url[MAX_PATH];
	sprintf(url, "%s/login/authkey", kc.getURL());

	m_http = http;
	m_timeout = timeout;
	m_timestart = 0;
	char request_data[1024];
	const char* form[2];

	// Generate iv
	for (int i = 0; i < 16; i++) {
		iv[i] = aesKeyClient[i + 16];
	}

	// Generate auth data
	const char dev_data[] = "{\"Rating\":\"0\",\"Detail\" : \"This is a Android device\"}";
	char auth_data[2048] = "";
	sprintf(auth_data, "{ \"1\":\"%s\",\"2\": \"%s\", \"3\": \"%s\" }", kc.getLoginKey(), kc.getLoginPw(), base64_encode(dev_data, strlen(dev_data)));

	// Encrypt dummy token
	unsigned char dummy_token[2048] = "";
	RSA_public_encrypt(32, (unsigned char *)aesKeyClient, dummy_token, RSA_Public_Key, RSA_PKCS1_PADDING);

	// Encrypt auth_data
	char auth_data_enc[2048];
	int auth_data_enc_l;
	if (aesEncrypt(auth_data, strlen(auth_data), aesKeyClient, iv, auth_data_enc, auth_data_enc_l) == false) klb_assertAlways("Error in AES encryption");

	sprintf(request_data, "request_data={\"dummy_token\":\"%s\",\"auth_data\":\"%s\"}", base64_encode((char*)dummy_token, 128), base64_encode(auth_data_enc, auth_data_enc_l));
	form[0] = request_data;
	form[1] = NULL;
	m_http->setForm(form);
	authkey = true;
	http->httpPOST(url, false);
}

void
CKLBNetAPI::execute(u32 deltaT)
{
	if (!m_http) {
		return; // Do nothing if no active connection
	}

	m_timestart += deltaT;

	// Check cancel first
	if (m_canceled) {
		lua_callback(NETAPIMSG_CONNECTION_CANCELED, -1, NULL, m_nonce);

		NetworkManager::releaseConnection(m_http);
		m_http = NULL;
		// Reset flag
		m_canceled = false;
		return;
	}

	// Received data second
	if (m_http->httpRECV() || (m_http->getHttpState() != -1)) {
		// Get Data
		u8* body = m_http->getRecvResource();
		u32 bodyLen = body ? m_http->getSize() : 0;
		// Get Status Code
		CKLBNetAPIKeyChain& kc = CKLBNetAPIKeyChain::getInstance();
		int state = m_http->getHttpState();
		bool invalid = ((state >= 400) && (state <= 599)) || (state == 204);
		int msg = invalid == false ? NETAPIMSG_REQUEST_SUCCESS : NETAPIMSG_SERVER_ERROR;

		if (m_http->isMaintenance())
		{
			NetworkManager::releaseConnection(m_http);
			m_http = NULL;

			CKLBLuaEnv::getInstance().intoMaintenance();
			return;
		}

		//
		// Support only JSon for callback
		// 
		freeJSonResult();

		if (bodyLen > 0) m_pRoot = getJsonTree((const char*)body, bodyLen);

		/* Upps, server sends invalid JSON */
		if (m_pRoot == NULL)
		{
			NetworkManager::releaseConnection(m_http);
			m_http = NULL;
			lua_callback(NETAPIMSG_SERVER_ERROR, state, NULL, m_nonce);

			return;
		}

		// If bundle version is very outdated, server return error 720 on login/authkey
		if (get_statuscode(m_pRoot->child()) != 200 && authkey) {
			NetworkManager::releaseConnection(m_http);
			m_http = NULL;
			lua_callback(NETAPIMSG_SERVER_ERROR, state, NULL, m_nonce);
			return;
		}

		if (invalid == false)
		{
			m_nonce++;	// Increase nonce if request success.

			puts("!====Response Data====!");
			fwrite(body, 1, bodyLen, stdout);
			puts("\n*====Response Data====*");

			if (authkey) {
				kc.setToken(m_pRoot->child()->child()->getString());
				char dummy_token[64] = "";
				char dummy_token_[64] = "";
				sprintf(dummy_token, "%s", m_pRoot->child()->child()->next()->getString());
				memcpy(dummy_token_, base64_decode(dummy_token), 32);
				for (int i = 0; i < 32; i++) {
					sessionKey[i] = aesKeyClient[i] ^ dummy_token_[i];
				}
				authkey = false;
			}
			DEBUG_PRINT("Current EndPoint: %s", kc.getEndPoint());
			const char* endPoint = kc.getEndPoint();

			if (m_request_type == NETAPI_STARTUP)
				return startUp(m_netapi_phase, state);
			else if (m_request_type == NETAPI_LOGIN)
				return login(m_netapi_phase, state);


			// Check if we're outdated
			if (m_downloading == false)
			{
				const char* server_ver[1];
				if (m_http->hasHeader("Server-Version", server_ver))
				{
					if (strncmp(*server_ver, kc.getClient(), strlen(kc.getClient())))
					{
						// We need to download some data.
						NetworkManager::releaseConnection(m_http);
						m_http = NULL;
						m_request_type = (-1);
						m_downloading = true;
						CKLBScriptEnv::getInstance().call_netAPI_versionUp(m_verup_callback, this, kc.getClient(), *server_ver);

						return;
					}
				}
			}
			fail_times = 0;

			NetworkManager::releaseConnection(m_http);
			m_http = NULL;
			m_request_type = (-1);
			lua_callback(msg, state, m_pRoot, m_nonce - 1);

			return;
		}

		NetworkManager::releaseConnection(m_http);
		m_http = NULL;
		m_request_type = (-1);

		lua_callback(msg, state, m_pRoot, m_nonce);

		return;
	}

	if ((m_http->m_threadStop == 1) && (m_http->getHttpState() == -1)) {
		if (fail_times < 5)
		{
			fail_times++;
			goto netapi_timeout;
		}
		lua_callback(map_netapi_fail(m_request_type), -1, NULL, m_nonce);
		NetworkManager::releaseConnection(m_http);
		m_http = NULL;
	}

	// Time out third (after check that valid has arrived)
	if (m_timestart >= m_timeout) {
	netapi_timeout:
		lua_callback(NETAPIMSG_SERVER_TIMEOUT, -1, NULL, m_nonce);
		NetworkManager::releaseConnection(m_http);
		m_http = NULL;
		return;
	}
}

void
CKLBNetAPI::die()
{
	if (m_http) {
		NetworkManager::releaseConnection(m_http);
	}
	KLBDELETEA(m_callback);
	freeHeader();
	freeJSonResult();
}

void
CKLBNetAPI::freeJSonResult() {
	KLBDELETE(m_pRoot);
}

void
CKLBNetAPI::freeHeader() {
	if (m_http_header_array) {
		for (u32 n = 0; n < m_http_header_length; n++) {
			KLBDELETEA(m_http_header_array[n]);
		}
		KLBDELETEA(m_http_header_array);
		m_http_header_array = NULL;
	}
}

CKLBNetAPI*
CKLBNetAPI::create(CKLBTask* pParentTask,
	const char * callback)
{
	CKLBNetAPI* pTask = KLBNEW(CKLBNetAPI);
	if (!pTask) { return NULL; }

	if (!pTask->init(pParentTask, callback)) {
		KLBDELETE(pTask);
		return NULL;
	}
	return pTask;
}

bool
CKLBNetAPI::init(CKLBTask* pTask,
	const char * callback)
{
	m_callback = (callback) ? CKLBUtility::copyString(callback) : NULL;

	// 一通り初期化値が作れたのでタスクを登録
	bool res = regist(pTask, P_INPUT);
	return res;
}

extern char* server_url_force;

bool
CKLBNetAPI::initScript(CLuaState& lua)
{
	int argc = lua.numArgs();
	lua.print_stack();

	if (argc < 7) { return false; }

	CKLBNetAPIKeyChain& kc = CKLBNetAPIKeyChain::getInstance();

	if (server_url_force)
		kc.setURL(server_url_force);
	else
		kc.setURL(lua.getString(1));

	kc.setConsumernKey(lua.getString(2));
	kc.setClient(lua.getString(3));
	kc.setAppID(lua.getString(4));
	kc.setRegion(lua.getString(7));

	if (lua.isString(8))
		m_verup_callback = CKLBUtility::copyString(lua.getString(8));

	fail_times = 0;

	return init(NULL, lua.getString(5));
}

CKLBJsonItem *
CKLBNetAPI::getJsonTree(const char * json_string, u32 dataLen)
{
	CKLBJsonItem * pRoot = CKLBJsonItem::ReadJsonData((const char *)json_string, dataLen);

	return pRoot;
}

// authorize_string ends with NULL
void CKLBNetAPI::set_header(CKLBHTTPInterface* http, const char* authorize_string)
{
	// Basic vars
	CPFInterface& pfif = CPFInterface::getInstance();
	CKLBNetAPIKeyChain& kc = CKLBNetAPIKeyChain::getInstance();
	const char* headers[13];
	const char* os_info = pfif.platform().getPlatform();

	// For values above
	char* alldata = new char[1280];
	char* os_data = alldata;
	char* os_version = alldata + 128;
	char* time_zone = alldata + 256;
	char* application_id = time_zone + 128;
	char* authorize = NULL; // Special
	char* bundle_version = time_zone + 256;
	char* client_version = bundle_version + 128;
	char* region = bundle_version + 256;
	char* user_id = region + 128;

	// Process authorize string
	authorize = new char[1024];

	sprintf(authorize, "Authorize: %s", authorize_string);
	sprintf(application_id, "Application-ID: %s", kc.getAppID());
	sprintf(bundle_version, "Bundle-Version: %s", pfif.platform().getBundleVersion());
	sprintf(client_version, "Client-Version: %s", kc.getClient());
	sprintf(region, "Region: %s", kc.getRegion());

	// User-ID
	{
		const char* uid = kc.getUserID();

		if (uid == NULL)
			user_id = NULL;
		else
			sprintf(user_id, "User-ID: %s", uid);
	}

	// Set header
	headers[0] = "API-Model: straightforward";
	headers[1] = application_id;
	headers[2] = authorize;
	headers[3] = bundle_version;
	headers[4] = client_version;
	headers[5] = "Debug: 1";
	headers[6] = "OS: Android";
	headers[7] = "OS-Version: Nexus 5 google hammerhead 4.4.4";
	headers[8] = "Platform-Type: 2";
	headers[9] = region;
	headers[10] = user_id;
	headers[11] = NULL;

	http->setHeader(headers);

	delete[] authorize;
	delete[] alldata;
}

int
CKLBNetAPI::commandScript(CLuaState& lua)
{
	int argc = lua.numArgs();

	if (argc < 2) {
		lua.retBoolean(false);
		return 1;
	}
	lua.print_stack();
	int cmd = m_request_type = lua.getInt(2);
	int ret = 1;

	switch (cmd)
	{
	default:
	{
		lua.retBoolean(false);
	}
	break;
	case NETAPI_STARTUP:
	{
		//
		// 3. login_key
		// 4. login_passwd
		// 5. invite
		// 6. timeout
		//
		if (argc < 4) lua.retBoolean(false);
		else {
			const char* user_id = lua.getString(3);
			const char* password = lua.getString(4);
			int timeout = 30000;

			if (lua.isNum(6))
				timeout = lua.getInt(6);

			// Save credentials
			CKLBNetAPIKeyChain& kc = CKLBNetAPIKeyChain::getInstance();
			kc.setLoginKey(user_id);
			kc.setLoginPwd(password);

			// Create HTTP
			m_timeout = timeout;
			m_timestart = 0;
			request_authkey(timeout);

			lua.retBoolean(true);
		}
	}
	break;
	case NETAPI_LOGIN:
	{
		//
		// 3. login_key
		// 4. login_passwd
		// 5. timeout
		//
		CKLBNetAPIKeyChain& kc = CKLBNetAPIKeyChain::getInstance();
		const char* auth;

		kc.setLoginKey(lua.getString(3));
		kc.setLoginPwd(lua.getString(4));

		auth = create_authorize_string(kc.getConsumerKey(), m_nonce);

		m_timeout = lua.getInt(5);
		m_timestart = 0;

		request_authkey(m_timeout);

		lua.retInt(m_nonce);
	}
	break;
	case NETAPI_CANCEL:
	case NETAPI_CANCEL_ALL:
	{
		if (m_http != NULL) {
			m_canceled = true;
		}
		lua.retBoolean(m_http != NULL);
	}
	break;
	case NETAPI_SEND:
	{
		//
		// 3. Request data table
		// 4. End point URL. Defaults to "/api" if is nil
		// 5. Timeout
		// 6. Skip version check?
		// 7. Absolute URL
		//
		CKLBNetAPIKeyChain& kc = CKLBNetAPIKeyChain::getInstance();
		if (argc < 3 || argc > 9) {
			lua.retBoolean(false);
			DEBUG_PRINT("No enough argc or argc exceed 9!");
		}
		else {
			DEBUG_PRINT("Sending API...");
			char api[MAX_PATH];
			const char* end_point = "/api";

			if (lua.isString(4)) {
				end_point = lua.getString(4);
				kc.setEndPoint(end_point);
			}


			if (lua.isBool(7) && lua.getBool(7))
				memcpy(api, end_point, strlen(end_point) + 1);
			else
				sprintf(api, "%s%s", kc.getURL(), end_point);

			// lua arg 9 can have key, that we will use for signing our message
			if (lua.isString(9) && lua.getString(9)) {
				memcpy(sessionKey, lua.getString(9), 32);
				sp = true;
			}
			// if arg 9 is nil and sp is true, we need to restore our key from backup
			else if (sp == true) {
				memcpy(sessionKey, sessionKey_, 32);
				sp = false;
			}
			// create backup of session key
			else {
				memcpy(sessionKey_, sessionKey, 32);
			}

			// Header list
			const char** headers = NULL;
			const char** values = NULL;
			freeHeader();

			// POST JSon payload
			u32 send_json_size = 0;
			const char* send_json = NULL;

			m_http = NetworkManager::createConnection();

			if (m_http) {

				lua.retValue(3);
				send_json = CKLBUtility::lua2json(lua, send_json_size);
				lua.pop(1);

				if (send_json) {
					char* json;
					const char * items[2];
					const char* req = "request_data=";
					const char* authorize = create_authorize_string(kc.getConsumerKey(), m_nonce, kc.getToken());
					int send_json_length = strlen(send_json);
					int req_length = strlen(req);

					if (lua.isBool(6) == false || lua.getBool(6) == false)
						m_downloading = false;

					json = KLBNEWA(char, send_json_length + req_length + 1);
					strcpy(json, req);
					strcat(json, send_json);
					items[0] = json;
					items[1] = NULL;

					set_header(m_http, authorize);
					m_http->setForm(items);
					m_http->httpPOST(api, false);

					KLBDELETEA(json);
					KLBDELETEA(authorize);
				}
				else {
					m_http->httpGET(api, false);
				}

				m_timeout = lua.getInt(5);
				m_timestart = 0;

				lua.retInt(m_nonce);
			}
			else {
				// Connection creation failed.
				lua.retBoolean(false);
			}
		}
	}
	break;
	}
	return 1;
}

bool
CKLBNetAPI::lua_callback(int msg, int status, CKLBJsonItem * pRoot, int uniq)
{
	return CKLBScriptEnv::getInstance().call_netAPI_callback(m_callback, this, uniq, msg, status, pRoot);
}