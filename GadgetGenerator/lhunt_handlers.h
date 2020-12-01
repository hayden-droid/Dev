#ifndef lhunt_handlers_h
#define lhunt_handlers_h


#include "lua.h"
#include "lobject.h"

#define HANDLER_PARAMS lua_State* luaState, Proto* function, lua_Writer writer
#define PASSING_HANDLER_PARAMS luaState,function,writer

extern int genGadgets;
extern const char* output;

#ifdef __cplusplus
#include <map>
#include <string>
typedef void(*LuaHuntHandler)(HANDLER_PARAMS);
extern const std::map<std::string, LuaHuntHandler> HandlerBinding;

void DumpFile(HANDLER_PARAMS);
#endif

#ifdef __cplusplus
extern "C"
{
#endif
	void lhunt_handlers_init();
	void lhunt_handlers_free();

#ifdef __cplusplus
}
#endif

#endif