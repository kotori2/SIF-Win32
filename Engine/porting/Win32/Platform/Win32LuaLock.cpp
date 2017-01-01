// Win32 lua lock
#include <map>
#include <exception>
#include <stdexcept>

#include "lua.h"
#include "CPFInterface.h"
#include "assert_klb.h"

static std::map<lua_State*, void*> LockList;

extern "C" void LockLuaState(lua_State* L)
{
	IPlatformRequest& platform = CPFInterface::getInstance().platform();
	void* lock = LockList[L];

	if(lock == NULL)
	{
		lock = platform.allocMutex();
		klb_assert(lock, "Failed to alloc mutex for lua state %p", L);
		LockList[L] = lock;
	}

	platform.mutexLock(lock);
}

extern "C" void UnlockLuaState(lua_State* L)
{
	void* lock = LockList[L];

	klb_assert(lock, "Mutex for lua state %p does not exist", L);

	CPFInterface::getInstance().platform().mutexUnlock(lock);
}
