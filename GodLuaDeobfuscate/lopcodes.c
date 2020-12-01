/*
** $Id: lopcodes.c,v 1.55.1.1 2017/04/19 17:20:42 roberto Exp $
** Opcodes for Lua virtual machine
** See Copyright Notice in lua.h
*/

#define lopcodes_c
#define LUA_CORE

#include "lprefix.h"


#include <stddef.h>

#include "lopcodes.h"


/* ORDER OP */

LUAI_DDEF const char *const luaP_opnames[NUM_OPCODES+1] = {
"GETTABUP", "GETTABLE", "MOVE", "LOADK", "LOADKX", "LOADBOOL", "LOADNIL", "GETUPVAL", "SETTABUP", "SETUPVAL", "SETTABLE", "NEWTABLE", "SELF", "CONCAT", "ADD", "SUB", "MUL", "MOD", "POW", "DIV", "IDIV", "BAND", "BOR", "BXOR", "SHL", "SHR", "UNM", "BNOT", "NOT", "LEN", "TEST", "TESTSET", "JMP", "EQ", "LT", "LE", "CALL", "TAILCALL", "RETURN", "FORLOOP", "FORPREP", "TFORCALL", "TFORLOOP", "SETLIST", "CLOSURE", "VARARG", "EXTRAARG",
  NULL
};


#define opmode(t,a,b,c,m) (((t)<<7) | ((a)<<6) | ((b)<<4) | ((c)<<2) | (m))

LUAI_DDEF const lu_byte luaP_opmodes[NUM_OPCODES] = {
opmode(0, 1, OpArgU, OpArgK, iABC),
opmode(0, 1, OpArgR, OpArgK, iABC),
opmode(0, 1, OpArgR, OpArgN, iABC),
opmode(0, 1, OpArgK, OpArgN, iABx),
opmode(0, 1, OpArgN, OpArgN, iABx),
opmode(0, 1, OpArgU, OpArgU, iABC),
opmode(0, 1, OpArgU, OpArgN, iABC),
opmode(0, 1, OpArgU, OpArgN, iABC),
opmode(0, 0, OpArgK, OpArgK, iABC),
opmode(0, 0, OpArgU, OpArgN, iABC),
opmode(0, 0, OpArgK, OpArgK, iABC),
opmode(0, 1, OpArgU, OpArgU, iABC),
opmode(0, 1, OpArgR, OpArgK, iABC),
opmode(0, 1, OpArgR, OpArgR, iABC),
opmode(0, 1, OpArgK, OpArgK, iABC),
opmode(0, 1, OpArgK, OpArgK, iABC),
opmode(0, 1, OpArgK, OpArgK, iABC),
opmode(0, 1, OpArgK, OpArgK, iABC),
opmode(0, 1, OpArgK, OpArgK, iABC),
opmode(0, 1, OpArgK, OpArgK, iABC),
opmode(0, 1, OpArgK, OpArgK, iABC),
opmode(0, 1, OpArgK, OpArgK, iABC),
opmode(0, 1, OpArgK, OpArgK, iABC),
opmode(0, 1, OpArgK, OpArgK, iABC),
opmode(0, 1, OpArgK, OpArgK, iABC),
opmode(0, 1, OpArgK, OpArgK, iABC),
opmode(0, 1, OpArgR, OpArgN, iABC),
opmode(0, 1, OpArgR, OpArgN, iABC),
opmode(0, 1, OpArgR, OpArgN, iABC),
opmode(0, 1, OpArgR, OpArgN, iABC),
opmode(1, 0, OpArgN, OpArgU, iABC),
opmode(1, 1, OpArgR, OpArgU, iABC),
opmode(0, 0, OpArgR, OpArgN, iAsBx),
opmode(1, 0, OpArgK, OpArgK, iABC),
opmode(1, 0, OpArgK, OpArgK, iABC),
opmode(1, 0, OpArgK, OpArgK, iABC),
opmode(0, 1, OpArgU, OpArgU, iABC),
opmode(0, 1, OpArgU, OpArgU, iABC),
opmode(0, 0, OpArgU, OpArgN, iABC),
opmode(0, 1, OpArgR, OpArgN, iAsBx),
opmode(0, 1, OpArgR, OpArgN, iAsBx),
opmode(0, 0, OpArgN, OpArgU, iABC),
opmode(0, 1, OpArgR, OpArgN, iAsBx),
opmode(0, 0, OpArgU, OpArgU, iABC),
opmode(0, 1, OpArgU, OpArgN, iABx),
opmode(0, 1, OpArgU, OpArgN, iABC),
opmode(0, 0, OpArgU, OpArgU, iAx)
};

