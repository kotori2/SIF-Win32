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
#include "CKLBLuaLibENG.h"
#include "CKLBTask.h"
#include "CKLBLuaEnv.h"
#include "CKLBUtility.h"

static CKLBLuaLibENG libdef(0);

s64  CKLBLuaLibENG::ms_timers[SCRIPT_TIMER_COUNT];

CKLBLuaLibENG::CKLBLuaLibENG(DEFCONST * arrConstDef) : ILuaFuncLib(arrConstDef) {}
CKLBLuaLibENG::~CKLBLuaLibENG() {}

int NotificationManage(lua_State * L);

// Lua関数の追加
void
CKLBLuaLibENG::addLibrary()
{
    addFunction("ENG_getAppVersion",    CKLBLuaLibENG::luaGetAppVersion );
	addFunction("ENG_isRelease",		CKLBLuaLibENG::luaIsRelease		);
	addFunction("ENG_getPlatform",		CKLBLuaLibENG::luaGetPlatform	);
	addFunction("ENG_getNanoTime",		CKLBLuaLibENG::luaGetNanoTime	);
	addFunction("ENG_startNanoTime",	CKLBLuaLibENG::luaStartNanoTime	);
	addFunction("ENG_endNanoTime",		CKLBLuaLibENG::luaEndNanoTime	);
	addFunction("ENG_getFrameID",		CKLBLuaLibENG::luaGetFrameID	);
    addFunction("ENG_getElapsedTime",   CKLBLuaLibENG::luaGetElapsedTime);
    addFunction("ENG_forbidSleep",      CKLBLuaLibENG::luaForbidSleep   );
	addFunction("NotificationManager",  NotificationManage);
}

CKLBLuaLibENG::CKLBLuaLibENG() {

}

int CKLBLuaLibENG::luaGetAppVersion(lua_State * L) {
    IPlatformRequest& pForm = CPFInterface::getInstance().platform();
    lua_newtable(L);
    lua_pushstring(L,pForm.getBundleVersion());
    return 1;
}

int
CKLBLuaLibENG::luaStartNanoTime(lua_State * L)
{
	CLuaState lua(L);
	int argc = lua.numArgs();
	int idx = 0;
	if(argc >= 1) {
		idx = lua.getInt(1);
		klb_assert(idx < SCRIPT_TIMER_COUNT, "Timer index %i is out of range 0..%i",idx, (SCRIPT_TIMER_COUNT-1));
	}

	// Read time last.
	ms_timers[idx] = CPFInterface::getInstance().platform().nanotime();

	// Return nothing.
	return 0;
}

int
CKLBLuaLibENG::luaEndNanoTime(lua_State * L)
{
	// Read time first
	s64 endTime = CPFInterface::getInstance().platform().nanotime();

	CLuaState lua(L);
	int argc = lua.numArgs();
	int idx = 0;
	if(argc >= 1) {
		idx = lua.getInt(1);
		klb_assert(idx < SCRIPT_TIMER_COUNT, "Timer index %i is out of range 0..%i",idx, (SCRIPT_TIMER_COUNT-1));
	}
	s64 deltaTime = endTime - ms_timers[idx];
	ms_timers[idx]	  = endTime;	// ready for next call to end nanotime.

	s32 nano	= (s32)(deltaTime % 1000000LL);
	s32 milli	= (s32)(deltaTime / 1000000LL);

	lua.retInt(milli);
	lua.retInt(nano);

	return 2;
}

int
CKLBLuaLibENG::luaIsRelease(lua_State * L)
{
	CLuaState lua(L);

	int argc = lua.numArgs();
	if(argc != 0) {
		lua.retBoolean(false);
		return 1;
	}

	lua.retBoolean(isRelease());
	return 1;
}

int
CKLBLuaLibENG::luaGetPlatform(lua_State * L)
{
	CLuaState lua(L);

	int argc = lua.numArgs();
	if(argc != 0) {
		lua.retBoolean(false);
		return 1;
	}
	IPlatformRequest& pfif = CPFInterface::getInstance().platform();
	const char * platform = pfif.getPlatform();

	lua.retString(platform);
	return 1;
}

int
CKLBLuaLibENG::luaGetNanoTime(lua_State * L)
{
	CLuaState lua(L);

	s64 nanotime = CPFInterface::getInstance().platform().nanotime();

	s32 nano  = (s32)(nanotime % 1000000LL);
	s32 milli = (s32)(nanotime / 1000000LL);

	lua.retInt(milli);
	lua.retInt(nano);

	return 2;
}

int
CKLBLuaLibENG::luaGetFrameID(lua_State * L)
{
	CLuaState lua(L);

	if(lua.numArgs() > 0) {
		lua.retNil();
		return 1;
	}
	CKLBTaskMgr& mgr = CKLBTaskMgr::getInstance();
	u32 frameID = mgr.getFrameID();

	lua.retInt(frameID);

	return 1;
}

int CKLBLuaLibENG::luaGetElapsedTime(lua_State * L)
{
    CLuaState lua(L);
    
	s64 sec = CPFInterface::getInstance().platform().getElapsedTime();
    
	lua.retInt((s32)sec);
    return 1;
}

int CKLBLuaLibENG::luaForbidSleep(lua_State * L)
{
    CLuaState lua(L);
	int argc = lua.numArgs();
	bool is_forbidden = lua.getBool(1);
	CPFInterface::getInstance().platform().forbidSleep(is_forbidden);
	return 0;
}

extern bool SIF_Win32_IS_RELEASE;

bool CKLBLuaLibENG::isRelease()
{
#ifndef IS_RELEASE
#define IS_RELEASE false
#endif
/*
	#if defined(DEBUG_MEMORY) || defined(DEBUG_PERFORMANCE) || defined(DEBUG_LUAEDIT) || defined(DEBUG_RT_CHECK) || defined(DEBUG_MENU)
		IPlatformRequest& pfif = CPFInterface::getInstance().platform();
		pfif.logging("===== Engine Compile Flags =====");
		#if defined(DEBUG_MEMORY) 
		pfif.logging("DEBUG_MEMORY");
		#endif
		#if defined(DEBUG_PERFORMANCE)
		pfif.logging("DEBUG_PERFORMANCE");
		#endif
		#if defined(DEBUG_LUAEDIT)
		pfif.logging("DEBUG_LUAEDIT");
		#endif
		#if defined(DEBUG_RT_CHECK)
		pfif.logging("DEBUG_RT_CHECK");
		#endif
		#if defined(DEBUG_MENU)
		pfif.logging("DEBUG_MENU");
		#endif
		pfif.logging("================================");
		// return false;
	#else
		return true;
	#endif*/
		return SIF_Win32_IS_RELEASE;
}

const char* CKLBLuaLibENG::getPlatform()
{
	IPlatformRequest& pfif = CPFInterface::getInstance().platform();
	return pfif.getPlatform();
}

void CKLBLuaLibENG::getNanoTime(s32* milli, s32* nano)
{
	s64 nanotime = CPFInterface::getInstance().platform().nanotime();

	*nano  = nanotime % 1000000LL;
	*milli = (s32)(nanotime / 1000000LL);
}

void CKLBLuaLibENG::endNanoTime(int idx, s32* milli, s32* nano)
{
	s64 endTime = CPFInterface::getInstance().platform().nanotime();

	klb_assert(idx < SCRIPT_TIMER_COUNT, "Timer index %i is out of range 0..%i",idx, (SCRIPT_TIMER_COUNT-1));

	s64 deltaTime = endTime - ms_timers[idx];
	ms_timers[idx]	  = endTime;

	*nano	= (s32)(deltaTime % 1000000LL);
	*milli	= (s32)(deltaTime / 1000000LL);
}

void CKLBLuaLibENG::startNanoTime(int idx) 
{
	ms_timers[idx] = CPFInterface::getInstance().platform().nanotime();
}

NotificationManager::NotificationManager():CKLBLuaTask() {
	
}

bool NotificationManager::initScript(CLuaState& lua) {
	DEBUG_PRINT("initScript Called");
	lua.print_stack();
	int argc = lua.numArgs();

	const char * callbackonNotificationOfPermissionGranted = (argc >= 1) ? lua.getString(1) : NULL;
	const char * callbackonNotificationOfPermissionFailed = (argc >= 2) ? lua.getString(2) : NULL;
	const char * callbackonNotificationOfResume = (argc >= 3) ? lua.getString(3) : NULL;

	return regist(NULL, P_NORMAL);
}

int NotificationManager::commandScript(CLuaState& lua) {
	DEBUG_PRINT("CommandScript Called");
	lua.print_stack();
	return 0;
}

void NotificationManager::execute(u32 deltaT) {
	DEBUG_PRINT("execute Called, step %d", m_eStep);
	switch (m_eStep)
	{
		case S_REQUIRE_PERMISSION:		exec_require_permission(deltaT); break;
	}
}

void NotificationManager::die() {
	DEBUG_PRINT("die Called");
}

void NotificationManager::exec_require_permission(u32 deltaT) {
	DEBUG_PRINT("exec_require_permission Called");
}

int NotificationManage(lua_State * L) {
	CLuaState lua(L);
	DEBUG_PRINT("Notification Manage Called");
	lua.print_stack();
	NotificationManager* NMP = KLBNEW(NotificationManager);
	NMP->initScript(lua);
	lua.retPointer(NMP);
	return 1;
}