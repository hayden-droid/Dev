#ifndef lhunt_hook_h
#define lhunt_hook_h

#include "lua.h"
#include "lobject.h"

#ifdef __cplusplus
extern "C" {
#endif
	void lhunt_dump(lua_State* luaState, Proto* function, lua_Writer writer, char* fileName);
#ifdef __cplusplus
}
#endif 


#endif