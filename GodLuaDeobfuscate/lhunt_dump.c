#include "lhunt_deobfuscator.h"

#include <stdio.h>

#include "lua.h"
#include "lobject.h"
#include "lstate.h"

#include "lhunt_opcodes.h"

#define LUA_VERSION_MAJOR_ORI	"5"
#define LUA_VERSION_MINOR_ORI	"3"

#define LUA_SIGNATURE_ORI	"\x1bLua"
#define MYINT(s)	(s[0]-'0')
#define LUAC_VERSION	(MYINT(LUA_VERSION_MAJOR_ORI)*16+MYINT(LUA_VERSION_MINOR_ORI))
#define LUAC_FORMAT	0	/* this is the official format */
#define LUAC_DATA	"\x19\x93\r\n\x1a\n"
#define LUAC_INT	0x5678
#define LUAC_NUM	cast_num(370.5)



typedef struct {
    lua_State* L;
    lua_Writer writer;
    void* data;
    int strip;
    int status;
} DumpState;

void DumpBlock(const void* b, size_t size, DumpState* D);
void DumpByte(int y, DumpState* D);
void DumpNumber(lua_Number x, DumpState* D);
void DumpInteger(lua_Integer x, DumpState* D);

#define DumpVector(v,n,D)	DumpBlock(v,(n)*sizeof((v)[0]),D)

#define DumpLiteral(s,D)	DumpBlock(s, sizeof(s) - sizeof(char), D)

void DumpHeader(DumpState* D)
{
    DumpLiteral(LUA_SIGNATURE_ORI, D);
    DumpByte(LUAC_VERSION, D);
    DumpByte(LUAC_FORMAT, D);
    DumpLiteral(LUAC_DATA, D);
    DumpByte(sizeof(int), D);
    DumpByte(sizeof(size_t), D);
    DumpByte(sizeof(Instruction), D);
    DumpByte(sizeof(lua_Integer), D);
    DumpByte(sizeof(lua_Number), D);
    DumpInteger(LUAC_INT, D);
    DumpNumber(LUAC_NUM, D);
}


void DumpBlock(const void* b, size_t size, DumpState* D) {
    if (D->status == 0 && size > 0) {
        lua_unlock(D->L);
        D->status = (*D->writer)(D->L, b, size, D->data);
        lua_lock(D->L);
    }
}


#define DumpVar(x,D)		DumpVector(&x,1,D)


void DumpByte(int y, DumpState* D) {
    lu_byte x = (lu_byte)y;
    DumpVar(x, D);
}


void DumpInt(int x, DumpState* D) {
    DumpVar(x, D);
}


void DumpNumber(lua_Number x, DumpState* D) {
    DumpVar(x, D);
}


void DumpInteger(lua_Integer x, DumpState* D) {
    DumpVar(x, D);
}


void DumpString(const TString* s, DumpState* D) {
    if (s == NULL)
        DumpByte(0, D);
    else {
        size_t size = tsslen(s) + 1;  /* include trailing '\0' */
        const char* str = getstr(s);
        if (size < 0xFF)
            DumpByte(cast_int(size), D);
        else {
            DumpByte(0xFF, D);
            DumpVar(size, D);
        }
        DumpVector(str, size - 1, D);  /* no need to save '\0' */
    }
}


void DumpCode(const Proto* f, DumpState* D) {
    DumpInt(f->sizecode, D);
    lhunt_setopcodes(f->sizecode, f->code); // set original opcodes
    DumpVector(f->code, f->sizecode, D);
}


void DumpFunction(const Proto* f, TString* psource, DumpState* D);

void DumpConstants(const Proto* f, DumpState* D) {
    int i;
    int n = f->sizek;
    DumpInt(n, D);
    for (i = 0; i < n; i++) {
        const TValue* o = &f->k[i];
        DumpByte(ttype(o), D);
        switch (ttype(o)) {
        case LUA_TNIL:
            break;
        case LUA_TBOOLEAN:
            DumpByte(bvalue(o), D);
            break;
        case LUA_TNUMFLT:
            DumpNumber(fltvalue(o), D);
            break;
        case LUA_TNUMINT:
            DumpInteger(ivalue(o), D);
            break;
        case LUA_TSHRSTR:
        case LUA_TLNGSTR:
            DumpString(tsvalue(o), D);
            break;
        default:
            lua_assert(0);
        }
    }
}


void DumpProtos(const Proto* f, DumpState* D) {
    int i;
    int n = f->sizep;
    DumpInt(n, D);
    for (i = 0; i < n; i++)
        DumpFunction(f->p[i], f->source, D);
}


void DumpUpvalues(const Proto* f, DumpState* D) {
    int i, n = f->sizeupvalues;
    DumpInt(n, D);
    for (i = 0; i < n; i++) {
        DumpByte(f->upvalues[i].instack, D);
        DumpByte(f->upvalues[i].idx, D);
    }
}


void DumpDebug(const Proto* f, DumpState* D) {
    int i, n;
    n = (D->strip) ? 0 : f->sizelineinfo;
    DumpInt(n, D);
    DumpVector(f->lineinfo, n, D);
    n = (D->strip) ? 0 : f->sizelocvars;
    DumpInt(n, D);
    for (i = 0; i < n; i++) {
        DumpString(f->locvars[i].varname, D);
        DumpInt(f->locvars[i].startpc, D);
        DumpInt(f->locvars[i].endpc, D);
    }
    n = (D->strip) ? 0 : f->sizeupvalues;
    DumpInt(n, D);
    for (i = 0; i < n; i++)
        DumpString(f->upvalues[i].name, D);
}


void DumpFunction(const Proto* f, TString* psource, DumpState* D) {
    if (D->strip || f->source == psource)
        DumpString(NULL, D);  /* no debug info or same source as its parent */
    else
        DumpString(f->source, D);
    DumpInt(f->linedefined, D);
    DumpInt(f->lastlinedefined, D);
    DumpByte(f->numparams, D);
    DumpByte(f->is_vararg, D);
    DumpByte(f->maxstacksize, D);
    DumpCode(f, D);
    DumpConstants(f, D);
    DumpUpvalues(f, D);
    DumpProtos(f, D);
    DumpDebug(f, D);
}

void DumpFunction_Ori(const Proto* f, TString* psource, DumpState* D)
{
    if (D->strip || f->source == psource)
        DumpString(NULL, D);  /* no debug info or same source as its parent */
    else
        DumpString(f->source, D);
    DumpInt(f->linedefined, D);
    DumpInt(f->lastlinedefined, D);
    DumpByte(f->numparams, D);
    DumpByte(f->is_vararg, D);
    DumpByte(f->maxstacksize, D);
    DumpCode(f, D);
    DumpConstants(f, D);
    DumpUpvalues(f, D);
    DumpProtos(f, D);
    DumpDebug(f, D);
}

void lhunt_deobfuscate(lua_State* L, const Proto* f, lua_Writer w, void* data)
{
    puts("LuaHunt deobfuscating...");

    DumpState D;
    D.L = L;
    D.writer = w;
    D.data = data;
    D.strip = 0;
    D.status = 0;
    DumpHeader(&D);
    DumpByte(f->sizeupvalues, &D);
    DumpFunction(f, NULL, &D);

    puts("Done.");
}
