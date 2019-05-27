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
#include "CKLBUIPolygon.h"

enum {
	UI_POLYGON_NEWPATH,
	UI_POLYGON_PUSHPATH,
	UI_POLYGON_NEWHOLE,
	UI_POLYGON_ENDHOLE,
	UI_POLYGON_BUILD,
	UI_POLYGON_ADDPOINT,
	UI_POLYGON_SETTEXTURE
};

static IFactory::DEFCMD cmd[] = {
	{	"UI_POLYGON_NEWPATH",			UI_POLYGON_NEWPATH },
	{	"UI_POLYGON_PUSHPATH",			UI_POLYGON_PUSHPATH },
	{	"UI_POLYGON_NEWHOLE",			UI_POLYGON_NEWHOLE },
	{	"UI_POLYGON_ENDHOLE",			UI_POLYGON_ENDHOLE },
	{	"UI_POLYGON_BUILD",				UI_POLYGON_BUILD },
	{	"UI_POLYGON_ADDPOINT",			UI_POLYGON_ADDPOINT },
	{	"UI_POLYGON_SETTEXTURE",		UI_POLYGON_SETTEXTURE },
	{	0, 0 }
};

static CKLBTaskFactory<CKLBUIPolygon> factory("UI_Polygon", CLS_KLBUIPOLYGON, cmd);

// Allowed Property Keys
CKLBLuaPropTask::PROP_V2 CKLBUIPolygon::ms_propItems[] = {
	UI_BASE_PROP,
	{	"order",			R_UINTEGER,	NULL,									(getBoolT)&CKLBUIPolygon::getOrder,	0	},
	{	"maxpointcount",	UINTEGER,	(setBoolT)&CKLBUIPolygon::setMaxPoint,	(getBoolT)&CKLBUIPolygon::getMaxPoint,	0	}
};

// 引数のインデックス定義
enum {
	ARG_PARENT = 1,
	ARG_ORDER,

	ARG_MAXPOINTCNT,
	
	ARG_NUMS    = ARG_MAXPOINTCNT,
	ARG_REQUIRE = ARG_MAXPOINTCNT	// 最低限必要なパラメータ数
};

CKLBUIPolygon::CKLBUIPolygon()
: CKLBUITask	()
, m_pPolygon	(NULL) 
{
	setNotAlwaysActive();
	m_newScriptModel = true;
}

CKLBUIPolygon::~CKLBUIPolygon() 
{
}

u32
CKLBUIPolygon::getClassID()
{
	return CLS_KLBUIPOLYGON;
}

CKLBUIPolygon* 
CKLBUIPolygon::create(CKLBUITask* pParent, CKLBNode* pNode, u32 order, u32 maxPoint) 
{
	CKLBUIPolygon* pTask = KLBNEW(CKLBUIPolygon);
    if (!pTask) { return NULL; }
	if (!pTask->init(pParent, pNode, order, maxPoint)) {
		KLBDELETE(pTask);
		return NULL;
	}
	return pTask;
}

bool CKLBUIPolygon::init(CKLBUITask* pParent, CKLBNode* pNode, u32 order, u32 maxPoint) {
    /*if(!setupNode()) { return false; }
	bool bResult = initCore(order, maxPoint);
	bResult = registUI(pParent, bResult);
	if(pNode) {
		pParent->getNode()->removeNode(getNode());
		pNode->addNode(getNode());
	}
	return bResult;*/
	return true;
}

bool 
CKLBUIPolygon::initCore(u32 order, u32 maxPoint)
{
	/*if(!setupPropertyList((const char**)ms_propItems,SizeOfArray(ms_propItems))) {
		return false;
	}

	m_maxpointcount = maxPoint;
	m_order         = order;

	// 必要とされるオブジェクトを生成する
	CKLBRenderingManager& pRdrMgr = CKLBRenderingManager::getInstance();

	m_pPolygon = pRdrMgr.allocateCommandPolyline(m_maxpointcount, order);
	if(!m_pPolygon) {
		return false;
	}

	// 二つのDynSpriteを自分のNodeに登録
	getNode()->setRender(m_pPolygon);
	getNode()->setRenderOnDestroy(true);

	getNode()->markUpMatrix();*/
	
	return true;
}

bool
CKLBUIPolygon::initUI(CLuaState& lua)
{
	lua.print_stack();
	/*int argc = lua.numArgs();
    if(argc > ARG_NUMS || argc < ARG_REQUIRE) { return false; }

	u32 order = lua.getInt(ARG_ORDER);
	u32 maxpoint = lua.getInt(ARG_MAXPOINTCNT);*/
	return true;//initCore(order,maxpoint);
}

int
CKLBUIPolygon::commandUI(CLuaState& lua, int argc, int cmd)
{
	int ret = 0;
	DEBUG_PRINT("UI_Polygon called: %d", cmd);
	lua.retString("123");
	lua.retBoolean(true);
	return 1;
	/*
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
	case UI_POLYLINE_SET_POINT:
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
	}
	return ret;*/
}

void
CKLBUIPolygon::execute(u32 /*deltaT*/)
{
	// Should never be executed.
	klb_assertAlways("Task execution is not necessary");
}

void
CKLBUIPolygon::dieUI()
{
}
