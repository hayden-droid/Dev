#ifndef H_LHUNT_DEOBFUSCATOR

#include "lua.h"
#include "lobject.h"

void lhunt_deobfuscate(lua_State* L, const Proto* f, lua_Writer w, void* data);

#define H_LHUNT_DEOBFUSCATOR
#endif