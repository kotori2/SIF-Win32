// Implements Micro Download system

#include <vector>
#include <direct.h>
#include <io.h>

#include "CPFInterface.h"
#include "CKLBScriptEnv.h"
#include "CKLBUtility.h"
#include "MultithreadedNetwork.h"

#include "DownloadQueue.h"

struct MDLData
{
	const char* url;
	const char* callback;
	const char* filename;
	CKLBHTTPInterface* http;

	MDLData(const char* c, const char* f, const char* u)
	{
		url = CKLBUtility::copyString(u);
		callback = CKLBUtility::copyString(c);
		filename = CKLBUtility::copyString(f);
		http = NetworkManager::createConnection();

		http->httpGET(u, false);
	};

	~MDLData()
	{
		KLBDELETEA(url);
		KLBDELETEA(callback);
		KLBDELETEA(filename);

		NetworkManager::releaseConnection(http);
	};
};

typedef std::vector<MDLData*> MicroDLQueue;
MicroDLQueue queue_list;

void MicroDownload::MainLoop(int )
{
	CKLBScriptEnv& scriptenv = CKLBScriptEnv::getInstance();

	for(MicroDLQueue::iterator i = queue_list.begin(); i != queue_list.end();)
	{
		MDLData* mdl = *i;
		int status_code = mdl->http->getHttpState();

		if(mdl->http->httpRECV() || status_code != (-1))
		{
			// Downloaded, but unsure if it's okay
			if(status_code != 200)
				goto error_mdl;

			IPlatformRequest& ir = CPFInterface::getInstance().platform();
			i = queue_list.erase(i);

			// Write to file. Without encryption so use fopen
			bool _;
			const char* target_file = ir.getFullPath(mdl->filename, &_);
			u8* body = mdl->http->getRecvResource();

			//Create folder recursively
			for (int i = 2; i < strlen(target_file); i++) {
				if (target_file[i] == '/') {
					char* folder = (char*)malloc(sizeof(char) * i + 2);
					strncpy(folder, target_file, i + 1);
					folder[i + 1] = 0;
					if (access(folder, 0)) {
						DEBUG_PRINT("Create folder %s", folder);
						if (_mkdir(folder) != 0) {
							DEBUG_PRINT("Failed to create folder %s", folder);
							break;
						}
					}
					
					KLBDELETEA(folder);
				}
			}

			if(body == NULL)
				goto error_mdl2;
			
			FILE* f = fopen(target_file, "wb");
			
			if(f == NULL)
				goto error_mdl2;

			size_t bodylen = body ? mdl->http->getSize() : 0;

			fwrite(body, 1, bodylen, f);
			fclose(f);
			
			// callback
			scriptenv.call_Mdl(mdl->callback, mdl->filename, mdl->url);

			delete mdl;
		}
		else if(mdl->http->m_threadStop == 1 && mdl->http->getHttpState() == (-1))
		{
			// Failed.
			error_mdl:
			i = queue_list.erase(i);

			error_mdl2:
			DEBUG_PRINT("MDL Error 2 occured.");
			scriptenv.call_Mdl(mdl->callback, NULL, mdl->url);

			delete mdl;
		}
		else
			++i;
	}
}

void MicroDownload::Queue(const char* callback, const char* filename, const char* url)
{
	MDLData* a = new MDLData(callback, filename, url);

	DEBUG_PRINT("Micro Download: %s ; %s ; %s", callback, filename, url);

	queue_list.push_back(a);
}

void MicroDownload::DeleteAll()
{
	CKLBScriptEnv& scriptenv = CKLBScriptEnv::getInstance();

	while(queue_list.empty() == false)
	{
		MDLData* mdl = queue_list.back();

		queue_list.pop_back();
		scriptenv.call_Mdl(mdl->callback, NULL, mdl->url);
		
		delete mdl;
	}
}
