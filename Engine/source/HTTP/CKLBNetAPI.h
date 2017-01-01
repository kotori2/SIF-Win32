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
#ifndef CKLBNetAPI_h
#define CKLBNetAPI_h

//#include <vector>

#include "CKLBLuaTask.h"
#include "CKLBHTTPInterface.h"
#include "CKLBJsonItem.h"
#include "CKLBUtility.h"
#include "MultithreadedNetwork.h"

// regionを指定しなかった場合のデフォルトregion値(ISO 3166-1)
#define DEFAULT_REGION "840"    // 北米アメリカ合衆国

//// 指定しそうなデフォルト値をコメントアウトして書いておく。
// #define DEFAULT_REGION "392"    // 日本


enum {
	// メッセージ値定義
	NETAPIMSG_CONNECTION_CANCELED	= -999,	// セッションはキャンセルされた
	NETAPIMSG_CONNECTION_FAILED		= -500,	// 接続に失敗した
	NETAPIMSG_INVITE_FAILED			= -200,
	NETAPIMSG_STARTUP_FAILED		= -100,
	NETAPIMSG_SERVER_TIMEOUT		= -4,	// サーバとの通信がタイムアウトした
	NETAPIMSG_REQUEST_FAILED		= -3,
	NETAPIMSG_LOGIN_FAILED			= -2,
	NETAPIMSG_SERVER_ERROR			= -1,
	NETAPIMSG_UNKNOWN				= 0,
	NETAPIMSG_LOGIN_SUCCESS			= 2,
	NETAPIMSG_REQUEST_SUCCESS		= 3,	// リクエスト成功ステータス
	NETAPIMSG_STARTUP_SUCCESS		= 100,
	NETAPIMSG_INVITE_SUCCESS		= 200,
};

// Native側からAPIタスクにコマンドを発行するためのsingleton.
// 
class CKLBNetAPI;

/*!
* \class CKLBNetAPI
* \brief Net API class.
* 
* CKLBNetAPI is responsible Network communications.
*/
class CKLBNetAPI : public CKLBLuaTask
{
	friend class CKLBTaskFactory<CKLBNetAPI>;
	friend bool KLBNetAPI_test_init(CKLBNetAPI* a,CKLBTask* b,const char* url,const char* name,const char* ver,const char* uv1,const char* uv2,unsigned int uv3,const char* uv4);
private:
	CKLBNetAPI();
	virtual ~CKLBNetAPI();

	bool init(	CKLBTask* pTask, 
				const char * callback);
public:
	virtual u32 getClassID();
	static CKLBNetAPI* create(	CKLBTask* pParentTask, 
								const char * callback);
	void set_header(CKLBHTTPInterface* http, const char* authorize_string);
	void request_authkey(int timeout);
	void startUp(int phase, int status_code);
	void login(int phase, int status_code);
	bool cancel(unsigned int uniq);
	void cancelAll();
	void execute(u32 deltaT);
	void die();

	bool initScript(CLuaState& lua);
	int commandScript(CLuaState& lua);
//private:
	//std::vector<CKLBHTTPInterface*> m_http_list;	// Multithreaded network.
	CKLBHTTPInterface*		m_http;		// そのセッションで使用されている接続
	u32						m_timeout;	// タイムアウト時間
	u32						m_timestart;
	u32						m_http_header_length;
	bool					m_canceled;	// セッションがキャンセルされると true になる
	CKLBJsonItem*			m_pRoot;
	int						m_request_type;	// For request type
	int						m_nonce;		// Request counter
	int						m_netapi_phase;
	bool					m_downloading;

	// スクリプトコールバック用
	const char			*	m_callback;	// Lua callback function
	const char			*	m_verup_callback;	// Lua callback function for update

	// HTTP通信で追加するヘッダの配列
	const char			**	m_http_header_array;

//private:
	void freeHeader();
	void freeJSonResult();

	bool lua_callback(int msg, int status, CKLBJsonItem * pRoot, int uniq = 0);

	CKLBJsonItem * getJsonTree(const char * json_string, u32 dataLen);

//public:
	bool cancel		();
};

#endif // CKLBNetAPI_h
