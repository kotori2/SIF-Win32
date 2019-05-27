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
#include "CKLBUIPolyline2.h"

enum {
	UI_POLYLINE_SET_POINTCOUNT,
	UI_POLYLINE_ADDPOINT,
	UI_POLYLINE_SET_COLOR,
	UI_POLYLINE_BUILD
};

static IFactory::DEFCMD cmd[] = {
	{"UI_POLYLINE_SET_POINTCOUNT",		UI_POLYLINE_SET_POINTCOUNT  },
	{"UI_POLYLINE_ADDPOINT",			UI_POLYLINE_ADDPOINT		},
	{"UI_POLYLINE_SET_COLOR",			UI_POLYLINE_SET_COLOR       },
	{"UI_POLYLINE_BUILD",				UI_POLYLINE_BUILD			},
	{0, 0 }
};

static CKLBTaskFactory<CKLBUIPolyline2> factory("UI_Polyline2", CLS_KLBUIPOLYLINE2, cmd);

// Allowed Property Keys
CKLBLuaPropTask::PROP_V2 CKLBUIPolyline2::ms_propItems[] = {
	UI_BASE_PROP,
	{	"order",			R_UINTEGER,	NULL,					(getBoolT)&CKLBUIPolyline2::getOrder,	0	},
};

// 引数のインデックス定義
enum {
	ARG_PARENT = 1,
	ARG_ORDER,
	
	ARG_NUMS    = ARG_ORDER,
	ARG_REQUIRE = ARG_ORDER	// 最低限必要なパラメータ数
};

CKLBUIPolyline2::CKLBUIPolyline2()
: CKLBUITask	()
, m_pPolyline2	(NULL) 
{
	setNotAlwaysActive();
	m_newScriptModel = true;
}

CKLBUIPolyline2::~CKLBUIPolyline2() 
{
}

u32
CKLBUIPolyline2::getClassID()
{
	return CLS_KLBUIPOLYLINE2;
}

CKLBUIPolyline2* 
CKLBUIPolyline2::create(CKLBUITask* pParent, CKLBNode* pNode, u32 order) 
{
	CKLBUIPolyline2* pTask = KLBNEW(CKLBUIPolyline2);
    if (!pTask) { return NULL; }
	if (!pTask->init(pParent, pNode, order)) {
		KLBDELETE(pTask);
		return NULL;
	}
	return pTask;
}

bool CKLBUIPolyline2::init(CKLBUITask* pParent, CKLBNode* pNode, u32 order) {
    if(!setupNode()) { return false; }
	bool bResult = initCore(order);
	bResult = registUI(pParent, bResult);
	if(pNode) {
		pParent->getNode()->removeNode(getNode());
		pNode->addNode(getNode());
	}
	return bResult;
}

bool 
CKLBUIPolyline2::initCore(u32 order)
{
	if(!setupPropertyList((const char**)ms_propItems,SizeOfArray(ms_propItems))) {
		return false;
	}

	m_maxpointcount = 21474836;
	m_order         = order;

	// 必要とされるオブジェクトを生成する
	CKLBRenderingManager& pRdrMgr = CKLBRenderingManager::getInstance();

	m_pPolyline2 = pRdrMgr.allocateCommandPolyline(m_maxpointcount, order);
	if(!m_pPolyline2) {
		return false;
	}

	// 二つのDynSpriteを自分のNodeに登録
	getNode()->setRender(m_pPolyline2);
	getNode()->setRenderOnDestroy(true);

	getNode()->markUpMatrix();
	
	return true;
}

bool
CKLBUIPolyline2::initUI(CLuaState& lua)
{
	int argc = lua.numArgs();
    if(argc > ARG_NUMS || argc < ARG_REQUIRE) { return false; }

	u32 order = lua.getInt(ARG_ORDER);
	//u32 maxpoint = lua.getInt(ARG_MAXPOINTCNT);
	return initCore(order);
}

int
CKLBUIPolyline2::commandUI(CLuaState& lua, int argc, int cmd)
{
	int ret = 0;
	switch(cmd)
	{
	case UI_POLYLINE_SET_POINTCOUNT:
		{
			bool result = false;
			if(argc == 3) {
				int ptsCount = lua.getInt(3);
				setPointCount(ptsCount);
				result = true;
			}
			lua.retBoolean(result);
			ret = 1;
		}
		break;
	case UI_POLYLINE_ADDPOINT:
		{
			bool result = false;
			if(argc == 5) {
				int idx = lua.getInt(3);
				float x = lua.getFloat(4);
				float y = lua.getFloat(5);
				setPoint(idx,x,y);
				result = true;
			}
			lua.retBoolean(result);
			ret = 1;
		}
		break;
	case UI_POLYLINE_SET_COLOR:
		{
			bool result = false;
			if(argc == 4) {
				u32 alpha = lua.getInt(3);
				u32 rgb = lua.getInt(4);

				u32 color = (alpha << 24) | (rgb & 0xffffff);

				setColor(color);
				result = true;
			}
			lua.retBoolean(result);
			ret = 1;

		}
		break;
	case UI_POLYLINE_BUILD:
		{
			DEBUG_PRINT("UI_POLYLINE_BUILD called, do nothing.");
			lua.retBoolean(true);
		}
		break;
	}
	return ret;
}

void
CKLBUIPolyline2::execute(u32 /*deltaT*/)
{
	// Should never be executed.
	klb_assertAlways("Task execution is not necessary");
}

void
CKLBUIPolyline2::dieUI()
{
}
