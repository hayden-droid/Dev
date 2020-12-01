#include "lhunt_handlers.h"

#include <iostream>
#include <iomanip>
#include <fstream>

#include <math.h>
#include <ftw.h>

#include <map>

#include "nlohmann/json.hpp"

#include "lopcodes.h"
#include "lundump.h"
#include "luac.h"

using namespace std;
using json = nlohmann::json;

#define FILE_NAME_BUF_SIZE 1024
#define OP_COUNT_BUF_SIZE 20
#define OPCODE_MAX pow(2,SIZE_OP)

#define OneOpcodeGenFileName(x) sprintf(outFileName, "%s/%02x", gadgetOutDir, x)
#define TwoOpcodeGenFileName(x,y) sprintf(outFileName, "%s/%02x%02x", gadgetOutDir, x, y)

#define DumpToFile() { FILE* D = fopen(outFileName, "wb");\
	lua_lock(luaState);\
	luaU_dump(luaState, function, writer, D, 1);\ 
	lua_unlock(L); \
	fclose(D);	\
}

int genGadgets = 0;

char* outFileName;
map<OpCode, OpCode> knownOpCodes;

OpCode GetOpCode(string opName)
{
	for (int i = 0; i < NUM_OPCODES; ++i)
	{
		if (opName == luaP_opnames[i])
			return (OpCode)i;
	}

	return (OpCode)-1;
}

string GetOpName(OpCode opCode)
{
	return luaP_opnames[opCode];
}

void lhunt_handlers_init()
{
	outFileName = (char *)malloc(FILE_NAME_BUF_SIZE);

	json jKnownOpCodes;

	ifstream ifs(knownOpcodesFile);

	if (!ifs.good())
	{
		perror(knownOpcodesFile);
		exit(1);
	}

	ifs >> jKnownOpCodes;
	ifs.close();

	cout << setw(4) << jKnownOpCodes << endl;

	// initialize known opcodes map
	for (json::iterator itr = jKnownOpCodes.begin(); itr != jKnownOpCodes.end(); ++itr)
	{
		knownOpCodes.insert(pair<OpCode, OpCode>(GetOpCode(itr.key()), itr.value()));
	}
}

void lhunt_handlers_free()
{
	free(outFileName);
}

static int unlink_cb(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf)
{
	int rv = remove(fpath);

	if (rv)
		perror(fpath);

	return rv;
}

#define FTW_DEPTH 8
#define FTW_PHYS 1

static int rmrf(const char* path)
{
	return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

static void InitializeOutputDir()
{
	rmrf(gadgetOutDir);

	mkdir(gadgetOutDir, 0700);
}

static bool IsOpcodeKnown(OpCode opcode)
{
	for (map<OpCode, OpCode>::iterator itr = knownOpCodes.begin(); itr != knownOpCodes.end(); ++itr)
	{
		if (itr->second == opcode)
			return true;
	}

	return false;
}

void DumpFile(HANDLER_PARAMS)
{
	for (int i = 0; i < function->sizecode; ++i)
	{
		OpCode curOpCode = GET_OPCODE(function->code[i]);

		map<OpCode,OpCode>::iterator knownOpCode = knownOpCodes.find(curOpCode);
		if (knownOpCode != knownOpCodes.end())
		{
			cout << "Set opcode " << luaP_opnames[GET_OPCODE(function->code[i])] << " to " << knownOpCode->second << endl;
			SET_OPCODE(function->code[i], knownOpCode->second);
		}
	}

	// dump to file directly

	FILE* D = fopen(output, "wb");

	lua_lock(luaState);
	luaU_dump(luaState, function, writer, D, 1);
	lua_unlock(L);

	fclose(D);
}

void SetKnownOpCodes(Proto* function)
{
	for (int i = 0; i < function->sizecode; ++i)
	{
		OpCode curOpCode = GET_OPCODE(function->code[i]);

		map<OpCode, OpCode>::iterator knownOpCode = knownOpCodes.find(curOpCode);
		if (knownOpCode != knownOpCodes.end())
		{
			cout << "Set opcode " << luaP_opnames[GET_OPCODE(function->code[i])] << " to " << knownOpCode->second << endl;
			SET_OPCODE(function->code[i], knownOpCode->second);
		}
	}

	for (int i = 0; i < function->sizep; ++i)
	{
		SetKnownOpCodes(function->p[i]);
	}
}

static void OneOpcodeGenFile(OpCode opcode, HANDLER_PARAMS)
{
	int opCount = 0;
	int opPos[OP_COUNT_BUF_SIZE];

	// find positions of opcode
	for (int insIndex = 0; insIndex < function->sizecode; ++insIndex)
	{
		if (GET_OPCODE(function->code[insIndex]) == opcode)
		{
			opPos[opCount++] = insIndex;
		}
	}

	// set known opcodes
	SetKnownOpCodes(function);

	// iterate opcode
	for (int i = 0; i < OPCODE_MAX; ++i)
	{
		// skip knwon opcodes
		if (IsOpcodeKnown((OpCode)i))
			continue;

		// set opcode
		for (int insIndex = 0; insIndex < opCount; ++insIndex)
		{
			SET_OPCODE(function->code[opPos[insIndex]], i);
		}

		// dump file
		OneOpcodeGenFileName(i);

		DumpToFile();
	}
}

static void OneOpcodeGenFile(OpCode opcode, HANDLER_PARAMS, Proto* target)
{
	int opCount = 0;
	int opPos[OP_COUNT_BUF_SIZE];

	// find positions of opcode
	for (int insIndex = 0; insIndex < target->sizecode; ++insIndex)
	{
		if (GET_OPCODE(target->code[insIndex]) == opcode)
		{
			opPos[opCount++] = insIndex;
		}
	}

	// set known opcodes
	SetKnownOpCodes(function);

	// iterate opcode
	for (int i = 0; i < OPCODE_MAX; ++i)
	{
		// skip knwon opcodes
		if (IsOpcodeKnown((OpCode)i))
			continue;

		// set opcode
		for (int insIndex = 0; insIndex < opCount; ++insIndex)
		{
			SET_OPCODE(target->code[opPos[insIndex]], i);
		}

		// dump file
		OneOpcodeGenFileName(i);

		DumpToFile();
	}
}

static void TwoOpcodeGenFile(OpCode first, OpCode second, HANDLER_PARAMS)
{
	int firstOpCount = 0, secondOpCount = 0;
	int firstOpPos[OP_COUNT_BUF_SIZE], secondOpPos[OP_COUNT_BUF_SIZE];

	// find first opcode positions
	for (int insIndex = 0; insIndex < function->sizecode; ++insIndex)
	{
		if (GET_OPCODE(function->code[insIndex]) == first)
		{
			firstOpPos[firstOpCount++] = insIndex;
		}
	}

	// find second opcode positions
	for (int insIndex = 0; insIndex < function->sizecode; ++insIndex)
	{
		if (GET_OPCODE(function->code[insIndex]) == second)
		{
			secondOpPos[secondOpCount++] = insIndex;
		}
	}

	// set known opcodes
	SetKnownOpCodes(function);

	// iterate first opcode
	for (int i = 0; i < OPCODE_MAX; ++i)
	{
		// skip knwon opcodes
		if (IsOpcodeKnown((OpCode)i))
			continue;

		// set first opcode
		for (int insIndex = 0; insIndex < firstOpCount; ++insIndex)
		{
			SET_OPCODE(function->code[firstOpPos[insIndex]], i);
		}

		// iterate second opcode
		for (int j = 0; j < OPCODE_MAX; ++j)
		{
			// skip knwon opcodes
			if (IsOpcodeKnown((OpCode)j))
				continue;

			//set second opcode
			for (int insIndex = 0; insIndex < secondOpCount; ++insIndex)
			{
				SET_OPCODE(function->code[secondOpPos[insIndex]], j);
			}

			// dump file
			TwoOpcodeGenFileName(i, j);

			DumpToFile();
		}
	}
}

#define OneOpcodeTest(x) InitializeOutputDir(); OneOpcodeGenFile(x, PASSING_HANDLER_PARAMS);
#define TwoOpcodeTest(x,y) InitializeOutputDir();  TwoOpcodeGenFile(x, y, PASSING_HANDLER_PARAMS);

void lhunt_gen_return(lua_State* luaState, Proto* function, lua_Writer writer)
{
	OneOpcodeTest(OP_RETURN)
}

/*	call.lua:
	1	[1]	GETTABUP 	0 0 -1	; _ENV "print"
	2	[1]	GETTABUP 	1 0 -1	; _ENV "print"
	3	[1]	CALL     	0 2 1
	4	[1]	RETURN   	0 1
*/
void lhunt_gen_call(HANDLER_PARAMS)
{
	TwoOpcodeTest(OP_GETTABUP, OP_CALL)
}

/*	call.lua:
	1	[1]	GETTABUP 	0 0 -1	; _ENV "print"
	2	[1]	GETTABUP 	1 0 -1	; _ENV "print"
	3	[1]	CALL     	0 2 1
	4	[1]	RETURN   	0 1
*/
void lhunt_gen_call_second(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_CALL)
}

/*		settabup.lua
		1       [1]     SETTABUP        0 -1 -2 ; _ENV "a" 1234567
		2       [3]     GETTABUP        0 0 -3  ; _ENV "print"
		3       [3]     GETTABUP        1 0 -1  ; _ENV "a"
		4       [3]     CALL            0 2 1
		5       [3]     RETURN          0 1
*/
void lhunt_gen_settabup(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_SETTABUP)
}

/*	loadk.lua:
	1	[1]	GETTABUP 	0 0 -1	; _ENV "print"
	2	[1]	LOADK    	1 -2	; "LuaGadget"
	3	[1]	CALL     	0 2 1
	4	[1]	RETURN   	0 1
*/
void lhunt_gen_loadk(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_LOADK)
}

/*		add.lua:
		1       [1]     LOADK           0 -1    ; 12345
		2       [2]     LOADK           1 -2    ; 56789
		3       [4]     GETTABUP        2 0 -3  ; _ENV "print"
		4       [4]     ADD             3 0 1
		5       [4]     CALL            2 2 1
		6       [4]     RETURN          0 1
*/
void lhunt_gen_add(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_ADD)
}

/*
		1       [1]     LOADK           0 -1    ; 12345
		2       [2]     LOADK           1 -2    ; 56789
		3       [4]     GETTABUP        2 0 -3  ; _ENV "print"
		4       [4]     SUB             3 1 0
		5       [4]     CALL            2 2 1
		6       [4]     RETURN          0 1
*/
void lhunt_gen_sub(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_SUB)
}

/*
		1       [1]     LOADK           0 -1    ; 12345
		2       [2]     LOADK           1 -2    ; 56789
		3       [4]     GETTABUP        2 0 -3  ; _ENV "print"
		4       [4]     MUL             3 0 1
		5       [4]     CALL            2 2 1
		6       [4]     RETURN          0 1
*/
void lhunt_gen_mul(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_MUL)
}

/*
		1       [1]     LOADK           0 -1    ; 12345567
		2       [2]     LOADK           1 -2    ; 7428699
		3       [4]     GETTABUP        2 0 -3  ; _ENV "print"
		4       [4]     MOD             3 0 1
		5       [4]     CALL            2 2 1
		6       [4]     RETURN          0 1
*/
void lhunt_gen_mod(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_MOD)
}

/*
		1       [1]     LOADK           0 -1    ; 23
		2       [2]     LOADK           1 -2    ; 9
		3       [4]     GETTABUP        2 0 -3  ; _ENV "print"
		4       [4]     POW             3 0 1
		5       [4]     CALL            2 2 1
		6       [4]     RETURN          0 1
*/
void lhunt_gen_pow(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_POW)
}

/*
		1       [1]     LOADK           0 -1    ; 12345567
		2       [2]     LOADK           1 -2    ; 7428699
		3       [4]     GETTABUP        2 0 -3  ; _ENV "print"
		4       [4]     DIV             3 0 1
		5       [4]     CALL            2 2 1
		6       [4]     RETURN          0 1
*/
void lhunt_gen_div(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_DIV)
}

/*
		1       [1]     LOADK           0 -1    ; -123457
		2       [2]     LOADK           1 -2    ; 268
		3       [4]     GETTABUP        2 0 -3  ; _ENV "print"
		4       [4]     IDIV            3 0 1
		5       [4]     CALL            2 2 1
		6       [4]     RETURN          0 1
*/
void lhunt_gen_idiv(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_IDIV)
}

/*
		1       [1]     LOADK           0 -1    ; 12345
		2       [2]     LOADK           1 -2    ; 56789
		3       [4]     GETTABUP        2 0 -3  ; _ENV "print"
		4       [4]     BAND            3 0 1
		5       [4]     CALL            2 2 1
		6       [4]     RETURN          0 1
*/
void lhunt_gen_band(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_BAND)
}

/*
		1       [1]     LOADK           0 -1    ; 12345
		2       [2]     LOADK           1 -2    ; 56789
		3       [4]     GETTABUP        2 0 -3  ; _ENV "print"
		4       [4]     BOR             3 0 1
		5       [4]     CALL            2 2 1
		6       [4]     RETURN          0 1
*/
void lhunt_gen_bor(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_BOR)
}

/*
		1       [1]     LOADK           0 -1    ; 12345
		2       [2]     LOADK           1 -2    ; 56789
		3       [4]     GETTABUP        2 0 -3  ; _ENV "print"
		4       [4]     BXOR            3 0 1
		5       [4]     CALL            2 2 1
		6       [4]     RETURN          0 1
*/
void lhunt_gen_bxor(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_BXOR)
}

/*
	1	[1]	LOADK    	0 -1	; 12345
	2	[3]	GETTABUP 	1 0 -2	; _ENV "print"
	3	[3]	SHL      	2 0 -3	; - 6
	4	[3]	CALL     	1 2 1
	5	[3]	RETURN   	0 1

*/
void lhunt_gen_shl(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_SHL)
}

/*
	1	[1]	LOADK    	0 -1	; 12345
	2	[3]	GETTABUP 	1 0 -2	; _ENV "print"
	3	[3]	SHR      	2 0 -3	; - 6
	4	[3]	CALL     	1 2 1
	5	[3]	RETURN   	0 1

*/
void lhunt_gen_shr(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_SHR)
}

/*
	1	[1]	LOADK    	0 -1	; 12345
	2	[3]	GETTABUP 	1 0 -2	; _ENV "print"
	3	[3]	UNM      	2 0
	4	[3]	CALL     	1 2 1
	5	[3]	RETURN   	0 1

*/
void lhunt_gen_unm(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_UNM)
}

/*
	1	[1]	LOADK    	0 -1	; 12345
	2	[3]	GETTABUP 	1 0 -2	; _ENV "print"
	3	[3]	BNOT     	2 0
	4	[3]	CALL     	1 2 1
	5	[3]	RETURN   	0 1

*/
void lhunt_gen_bnot(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_BNOT)
}

/*
	1	[1]	LOADK    	0 -1	; 12345
	2	[3]	GETTABUP 	1 0 -2	; _ENV "print"
	3	[3]	NOT      	2 0
	4	[3]	CALL     	1 2 1
	5	[3]	RETURN   	0 1
*/
void lhunt_gen_not(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_NOT)
}

/*
	1	[1]	LOADK    	0 -1	; 
	2	[4]	GETTABUP 	1 0 -2	; _ENV "print"
	3	[4]	LEN      	2 0
	4	[4]	CALL     	1 2 1
	5	[4]	RETURN   	0 1

*/
void lhunt_gen_len(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_LEN)
}

/*
	1	[2]	GETTABUP 	0 0 -1	; _ENV "print"
	2	[2]	LOADK    	1 -2	; "Lua"
	3	[2]	LOADK    	2 -3	; "Hunt"
	4	[2]	CONCAT   	1 1 2
	5	[2]	CALL     	0 2 1
	6	[2]	RETURN   	0 1
*/
void lhunt_gen_concat(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_CONCAT)
}

/*
	1	[1]	LOADK    	0 -1	; "LuaHunt"
	2	[3]	GETTABUP 	1 0 -2	; _ENV "print"
	3	[3]	MOVE     	2 0
	4	[3]	CALL     	1 2 1
	5	[3]	RETURN   	0 1

*/
void lhunt_gen_move(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_MOVE)
}

/*
		1       [1]     LOADK           0 -1    ; 100
		2       [3]     LOADK           1 -2    ; 1
		3       [3]     LOADK           2 -3    ; 9
		4       [3]     LOADK           3 -4    ; 2
		5       [3]     FORPREP         1 1     ; to 7
		6       [4]     ADD             0 0 4
		7       [3]     FORLOOP         1 -2    ; to 6
		8       [7]     GETTABUP        1 0 -5  ; _ENV "print"
		9       [7]     MOVE            2 0
		10      [7]     CALL            1 2 1
		11      [7]     RETURN          0 1
*/
void lhunt_gen_forloop(HANDLER_PARAMS)
{
	TwoOpcodeTest(OP_FORPREP,OP_FORLOOP)
}

/*
		1       [1]     GETTABUP        0 0 -1  ; _ENV "print"
		2       [1]     LOADBOOL        1 0 0
		3       [1]     CALL            0 2 1
		4       [1]     RETURN          0 1
*/
void lhunt_gen_loadbool(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_LOADBOOL)
}

/*
		1       [1]     GETTABUP        0 0 -1  ; _ENV "print"
		2       [1]     LOADNIL         1 0
		3       [1]     CALL            0 2 1
		4       [1]     RETURN          0 1
*/
void lhunt_gen_loadnil(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_LOADNIL)
}

/*
main <.\closure.lua:0,0> (5 instructions at 00000000009a89f0)
0+ params, 2 slots, 1 upvalue, 0 locals, 1 constant, 1 function
		1       [4]     CLOSURE         0 0     ; 00000000009a8b60
		2       [2]     SETTABUP        0 -1 0  ; _ENV "foo"
		3       [6]     GETTABUP        0 0 -1  ; _ENV "foo"
		4       [6]     CALL            0 1 1
		5       [6]     RETURN          0 1

function <.\closure.lua:2,4> (4 instructions at 00000000009a8b60)
0 params, 2 slots, 1 upvalue, 0 locals, 2 constants, 0 functions
		1       [3]     GETTABUP        0 0 -1  ; _ENV "print"
		2       [3]     LOADK           1 -2    ; "LuaHunt"
		3       [3]     CALL            0 2 1
		4       [4]     RETURN          0 1
*/
void lhunt_gen_closure(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_CLOSURE)
}

/*
main <.\getupval.lua:0,0> (6 instructions at 00000000001e89f0)
0+ params, 2 slots, 1 upvalue, 1 local, 2 constants, 1 function
		1       [1]     LOADK           0 -1    ; "LuaHunt"
		2       [5]     CLOSURE         1 0     ; 00000000001e8b60
		3       [3]     SETTABUP        0 -2 1  ; _ENV "foo"
		4       [7]     GETTABUP        1 0 -2  ; _ENV "foo"
		5       [7]     CALL            1 1 1
		6       [7]     RETURN          0 1

function <.\getupval.lua:3,5> (4 instructions at 00000000001e8b60)
0 params, 2 slots, 2 upvalues, 0 locals, 1 constant, 0 functions
		1       [4]     GETTABUP        0 0 -1  ; _ENV "print"
		2       [4]     GETUPVAL        1 1     ; a
		3       [4]     CALL            0 2 1
		4       [5]     RETURN          0 1
*/

void lhunt_gen_getupval(HANDLER_PARAMS)
{
	InitializeOutputDir();

	OneOpcodeGenFile(OP_GETUPVAL, PASSING_HANDLER_PARAMS, function->p[0]);
}

/*
		1       [1]     NEWTABLE        0 0 0
		2       [1]     SETTABUP        0 -1 0  ; _ENV "newtab"
		3       [3]     GETTABUP        0 0 -2  ; _ENV "print"
		4       [3]     GETTABUP        1 0 -1  ; _ENV "newtab"
		5       [3]     CALL            0 2 1
		6       [3]     RETURN          0 1
*/
void lhunt_gen_newtable(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_NEWTABLE)
}

/*
		1       [1]     NEWTABLE        0 1 0
		2       [1]     LOADK           1 -2    ; "LuaHunt"
		3       [1]     SETLIST         0 1 1   ; 1
		4       [1]     SETTABUP        0 -1 0  ; _ENV "newtab"
		5       [3]     GETTABUP        0 0 -3  ; _ENV "print"
		6       [3]     GETTABUP        1 0 -1  ; _ENV "newtab"
		7       [3]     GETTABLE        1 1 -4  ; 1
		8       [3]     CALL            0 2 1
		9       [3]     RETURN          0 1
*/
void lhunt_gen_setlist(HANDLER_PARAMS)
{
	TwoOpcodeTest(OP_SETLIST,OP_GETTABLE)
}

/*
main <.\self.lua:0,0> (10 instructions at 0000000000a989f0)
0+ params, 3 slots, 1 upvalue, 0 locals, 3 constants, 1 function
		1       [1]     NEWTABLE        0 0 0
		2       [1]     SETTABUP        0 -1 0  ; _ENV "foo"
		3       [3]     GETTABUP        0 0 -1  ; _ENV "foo"
		4       [5]     CLOSURE         1 0     ; 0000000000a98c10
		5       [3]     SETTABLE        0 -2 1  ; "bar" -
		6       [7]     GETTABUP        0 0 -1  ; _ENV "foo"
		7       [7]     SELF            0 0 -2  ; "bar"
		8       [7]     LOADK           2 -3    ; "LuaHunt"
		9       [7]     CALL            0 3 1
		10      [7]     RETURN          0 1

function <.\self.lua:3,5> (4 instructions at 0000000000a98c10)
2 params, 4 slots, 1 upvalue, 2 locals, 1 constant, 0 functions
		1       [4]     GETTABUP        2 0 -1  ; _ENV "print"
		2       [4]     MOVE            3 1
		3       [4]     CALL            2 2 1
		4       [5]     RETURN          0 1
*/
void lhunt_gen_self(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_SELF)
}

/*
main <.\tailcall.lua:0,0> (9 instructions at 0000000000c589f0)
0+ params, 2 slots, 1 upvalue, 0 locals, 3 constants, 2 functions
		1       [3]     CLOSURE         0 0     ; 0000000000c58b60
		2       [1]     SETTABUP        0 -1 0  ; _ENV "foo1"
		3       [7]     CLOSURE         0 1     ; 0000000000c58d80
		4       [5]     SETTABUP        0 -2 0  ; _ENV "foo2"
		5       [9]     GETTABUP        0 0 -3  ; _ENV "print"
		6       [9]     GETTABUP        1 0 -2  ; _ENV "foo2"
		7       [9]     CALL            1 1 0
		8       [9]     CALL            0 0 1
		9       [9]     RETURN          0 1

function <.\tailcall.lua:1,3> (3 instructions at 0000000000c58b60)
0 params, 2 slots, 0 upvalues, 0 locals, 1 constant, 0 functions
		1       [2]     LOADK           0 -1    ; "LuaHunt"
		2       [2]     RETURN          0 2
		3       [3]     RETURN          0 1

function <.\tailcall.lua:5,7> (4 instructions at 0000000000c58d80)
0 params, 2 slots, 1 upvalue, 0 locals, 1 constant, 0 functions
		1       [6]     GETTABUP        0 0 -1  ; _ENV "foo1"
		2       [6]     TAILCALL        0 1 0
		3       [6]     RETURN          0 0
		4       [7]     RETURN          0 1
*/
void lhunt_gen_tailcall(HANDLER_PARAMS)
{
	InitializeOutputDir();

	OneOpcodeGenFile(OP_TAILCALL, PASSING_HANDLER_PARAMS, function->p[1]);
}

// replace OP_ADD by OP_JMP
void SetJump(Proto* function)
{
	for (int codeIndex = 0; codeIndex < function->sizecode; ++codeIndex)
	{
		Instruction& curInst = function->code[codeIndex];

		if (GET_OPCODE(curInst) == OP_ADD)
		{
			SET_OPCODE(curInst, OP_JMP); // replace ADD by JMP

			SETARG_A(curInst, 0);
			SETARG_sBx(curInst, 3);
		}
	}
}

/*
		1       [1]     LOADK           0 -1    ; 123
		2       [3]     GETTABUP        1 0 -2  ; _ENV "print"
		3       [3]     LOADK           2 -3    ; "FLAG1"
		4       [3]     CALL            1 2 1
		5       [4]     ADD             0 0 -4  ; - 1
		6       [5]     GETTABUP        1 0 -2  ; _ENV "print"
		7       [5]     LOADK           2 -5    ; "FLAG2"
		8       [5]     CALL            1 2 1
		9       [6]     ADD             0 0 -4  ; - 1
		10      [7]     GETTABUP        1 0 -2  ; _ENV "print"
		11      [7]     LOADK           2 -6    ; "FLAG3"
		12      [7]     CALL            1 2 1
		13      [8]     ADD             0 0 -4  ; - 1
		14      [9]     GETTABUP        1 0 -2  ; _ENV "print"
		15      [9]     LOADK           2 -7    ; "FLAG4"
		16      [9]     CALL            1 2 1
		17      [10]    ADD             0 0 -4  ; - 1
		18      [11]    GETTABUP        1 0 -2  ; _ENV "print"
		19      [11]    LOADK           2 -8    ; "LuaHuntJmp"
		20      [11]    CALL            1 2 1
		21      [12]    ADD             0 0 -4  ; - 1
		22      [13]    GETTABUP        1 0 -2  ; _ENV "print"
		23      [13]    LOADK           2 -9    ; "FLAG5"
		24      [13]    CALL            1 2 1
		25      [14]    ADD             0 0 -4  ; - 1
		26      [15]    GETTABUP        1 0 -2  ; _ENV "print"
		27      [15]    LOADK           2 -10   ; "FLAG6"
		28      [15]    CALL            1 2 1
		29      [17]    GETTABUP        1 0 -2  ; _ENV "print"
		30      [17]    LOADK           2 -11   ; "a"
		31      [17]    CALL            1 2 1
		32      [17]    RETURN          0 1
*/
void lhunt_gen_jmp(HANDLER_PARAMS)
{
	SetJump(function);

	OneOpcodeTest(OP_JMP);
}

/*
	1	[1]	LOADK    	0 -1	; 123
	2	[3]	EQ       	0 0 -1	; - 123
	3	[3]	JMP      	0 27	; to 31
	4	[4]	ADD      	0 0 -2	; - 1
	5	[5]	GETTABUP 	1 0 -3	; _ENV "print"
	6	[5]	LOADK    	2 -4	; "FLAG1"
	7	[5]	CALL     	1 2 1
	8	[6]	ADD      	0 0 -2	; - 1
	9	[7]	GETTABUP 	1 0 -3	; _ENV "print"
	10	[7]	LOADK    	2 -5	; "FLAG2"
	11	[7]	CALL     	1 2 1
	12	[8]	ADD      	0 0 -2	; - 1
	13	[9]	GETTABUP 	1 0 -3	; _ENV "print"
	14	[9]	LOADK    	2 -6	; "FLAG3"
	15	[9]	CALL     	1 2 1
	16	[10]	ADD      	0 0 -2	; - 1
	17	[11]	GETTABUP 	1 0 -3	; _ENV "print"
	18	[11]	LOADK    	2 -7	; "FLAG4"
	19	[11]	CALL     	1 2 1
	20	[13]	GETTABUP 	1 0 -3	; _ENV "print"
	21	[13]	LOADK    	2 -8	; "LuaHuntLe"
	22	[13]	CALL     	1 2 1
	23	[14]	ADD      	0 0 -2	; - 1
	24	[15]	GETTABUP 	1 0 -3	; _ENV "print"
	25	[15]	LOADK    	2 -9	; "FLAG5"
	26	[15]	CALL     	1 2 1
	27	[16]	ADD      	0 0 -2	; - 1
	28	[17]	GETTABUP 	1 0 -3	; _ENV "print"
	29	[17]	LOADK    	2 -10	; "FLAG6"
	30	[17]	CALL     	1 2 1
	31	[18]	RETURN   	0 1

*/
void lhunt_gen_eq(HANDLER_PARAMS)
{
	SetJump(function);

	OneOpcodeTest(OP_EQ)
}

/*
		1       [1]     LOADK           0 -1    ; 123
		2       [3]     LE              0 0 -2  ; - 127
		3       [3]     JMP             0 27    ; to 31
		4       [4]     ADD             0 0 -3  ; - 1
		5       [5]     GETTABUP        1 0 -4  ; _ENV "print"
		6       [5]     LOADK           2 -5    ; "FLAG1"
		7       [5]     CALL            1 2 1
		8       [6]     ADD             0 0 -3  ; - 1
		9       [7]     GETTABUP        1 0 -4  ; _ENV "print"
		10      [7]     LOADK           2 -6    ; "FLAG2"
		11      [7]     CALL            1 2 1
		12      [8]     ADD             0 0 -3  ; - 1
		13      [9]     GETTABUP        1 0 -4  ; _ENV "print"
		14      [9]     LOADK           2 -7    ; "FLAG3"
		15      [9]     CALL            1 2 1
		16      [10]    ADD             0 0 -3  ; - 1
		17      [11]    GETTABUP        1 0 -4  ; _ENV "print"
		18      [11]    LOADK           2 -8    ; "FLAG4"
		19      [11]    CALL            1 2 1
		20      [13]    GETTABUP        1 0 -4  ; _ENV "print"
		21      [13]    LOADK           2 -9    ; "LuaHuntEq"
		22      [13]    CALL            1 2 1
		23      [14]    ADD             0 0 -3  ; - 1
		24      [15]    GETTABUP        1 0 -4  ; _ENV "print"
		25      [15]    LOADK           2 -10   ; "FLAG5"
		26      [15]    CALL            1 2 1
		27      [16]    ADD             0 0 -3  ; - 1
		28      [17]    GETTABUP        1 0 -4  ; _ENV "print"
		29      [17]    LOADK           2 -11   ; "FLAG6"
		30      [17]    CALL            1 2 1
		31      [18]    RETURN          0 1
*/
void lhunt_gen_le(HANDLER_PARAMS)
{
	SetJump(function);

	OneOpcodeTest(OP_LE)
}

void lhunt_gen_testset(HANDLER_PARAMS)
{
	for (int codeIndex = 0; codeIndex < function->sizecode; ++codeIndex)
	{
		Instruction& curInst = function->code[codeIndex];

		if (GET_OPCODE(curInst) == OP_TESTSET)
		{
			SETARG_C(curInst, 1);
		}
	}

	OneOpcodeTest(OP_TESTSET)
}

void lhunt_gen_test(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_TEST)
}

/*
main <.\vararg.lua:0,0> (7 instructions at 00000000001b89f0)
0+ params, 3 slots, 1 upvalue, 0 locals, 3 constants, 1 function
		1       [4]     CLOSURE         0 0     ; 00000000001b8b60
		2       [1]     SETTABUP        0 -1 0  ; _ENV "foo"
		3       [6]     GETTABUP        0 0 -1  ; _ENV "foo"
		4       [6]     LOADK           1 -2    ; "LuaHunt"
		5       [6]     LOADK           2 -3    ; "VarArg"
		6       [6]     CALL            0 3 1
		7       [6]     RETURN          0 1

function <.\vararg.lua:1,4> (9 instructions at 00000000001b8b60)
0+ params, 3 slots, 1 upvalue, 0 locals, 3 constants, 0 functions
		1       [2]     VARARG          0 3
		2       [2]     SETTABUP        0 -2 1  ; _ENV "b"
		3       [2]     SETTABUP        0 -1 0  ; _ENV "a"
		4       [3]     GETTABUP        0 0 -3  ; _ENV "print"
		5       [3]     GETTABUP        1 0 -1  ; _ENV "a"
		6       [3]     GETTABUP        2 0 -2  ; _ENV "b"
		7       [3]     CONCAT          1 1 2
		8       [3]     CALL            0 2 1
		9       [4]     RETURN          0 1
*/
void lhunt_gen_vararg(HANDLER_PARAMS)
{
	InitializeOutputDir();

	OneOpcodeGenFile(OP_VARARG, PASSING_HANDLER_PARAMS, function->p[0]);
}

/*
main <.\setupval.lua:0,0> (9 instructions at 00000000001c89f0)
0+ params, 3 slots, 1 upvalue, 1 local, 3 constants, 1 function
		1       [1]     LOADK           0 -1    ; 10086
		2       [5]     CLOSURE         1 0     ; 00000000001c8b60
		3       [3]     SETTABUP        0 -2 1  ; _ENV "foo"
		4       [7]     GETTABUP        1 0 -2  ; _ENV "foo"
		5       [7]     CALL            1 1 1
		6       [8]     GETTABUP        1 0 -3  ; _ENV "print"
		7       [8]     MOVE            2 0
		8       [8]     CALL            1 2 1
		9       [8]     RETURN          0 1

function <.\setupval.lua:3,5> (3 instructions at 00000000001c8b60)
0 params, 2 slots, 1 upvalue, 0 locals, 1 constant, 0 functions
		1       [4]     LOADK           0 -1    ; "LuaHuntSetUpVal"
		2       [4]     SETUPVAL        0 0     ; a
		3       [5]     RETURN          0 1
*/
void lhunt_gen_setupval(HANDLER_PARAMS)
{
	InitializeOutputDir();

	OneOpcodeGenFile(OP_SETUPVAL, PASSING_HANDLER_PARAMS, function->p[0]);
}

/*
main <.\settable.lua:0,0> (11 instructions at 00000000009e89f0)
0+ params, 2 slots, 1 upvalue, 0 locals, 5 constants, 0 functions
		1       [1]     NEWTABLE        0 1 0
		2       [1]     LOADK           1 -2    ; "Garbage"
		3       [1]     SETLIST         0 1 1   ; 1
		4       [1]     SETTABUP        0 -1 0  ; _ENV "newtab"
		5       [3]     GETTABUP        0 0 -1  ; _ENV "newtab"
		6       [3]     SETTABLE        0 -3 -4 ; 2 "LuaHuntSetTable"
		7       [5]     GETTABUP        0 0 -5  ; _ENV "print"
		8       [5]     GETTABUP        1 0 -1  ; _ENV "newtab"
		9       [5]     GETTABLE        1 1 -3  ; 2
		10      [5]     CALL            0 2 1
		11      [5]     RETURN          0 1
*/
void lhunt_gen_settable(HANDLER_PARAMS)
{
	OneOpcodeTest(OP_SETTABLE)
}

/*
main <.\tforloop.lua:0,0> (17 instructions at 00000000001789f0)
0+ params, 7 slots, 1 upvalue, 5 locals, 7 constants, 0 functions
		1       [1]     NEWTABLE        0 4 0
		2       [1]     LOADK           1 -2    ; "Lua"
		3       [1]     LOADK           2 -3    ; "Hunt"
		4       [1]     LOADK           3 -4    ; "T"
		5       [1]     LOADK           4 -5    ; "FORLOOP"
		6       [1]     SETLIST         0 4 1   ; 1
		7       [1]     SETTABUP        0 -1 0  ; _ENV "t"
		8       [3]     GETTABUP        0 0 -6  ; _ENV "pairs"
		9       [3]     GETTABUP        1 0 -1  ; _ENV "t"
		10      [3]     CALL            0 2 4
		11      [3]     JMP             0 3     ; to 15
		12      [4]     GETTABUP        5 0 -7  ; _ENV "print"
		13      [4]     MOVE            6 4
		14      [4]     CALL            5 2 1
		15      [3]     TFORCALL        0 2
		16      [3]     TFORLOOP        2 -5    ; to 12
		17      [5]     RETURN          0 1
*/
void lhunt_gen_tforcall(HANDLER_PARAMS)
{
	for (int codeIndex = 0; codeIndex < function->sizecode; ++codeIndex)
	{
		Instruction& curInst = function->code[codeIndex];

		if (GET_OPCODE(curInst) == OP_TFORLOOP)
		{
			SET_OPCODE(curInst, OP_BAND); // any opcode will work
		}
	}

	OneOpcodeTest(OP_TFORCALL)
}

/*
	392961	[131073]	LOADKX   	0
	392962	[131073]	EXTRAARG 	-262145	; "a131072"
	392963	[131073]	LOADKX   	1
	392964	[131073]	EXTRAARG 	-262146	; 131072
	392965	[131073]	SETTABUP 	0 0 1	; _ENV
	392966	[131075]	LOADKX   	0
	392967	[131075]	EXTRAARG 	-262147	; "print"
	392968	[131075]	GETTABUP 	0 0 0	; _ENV
	392969	[131075]	LOADKX   	1
	392970	[131075]	EXTRAARG 	-262145	; "a131072"
	392971	[131075]	GETTABUP 	1 0 1	; _ENV
	392972	[131075]	CALL     	0 2 1
	392973	[131075]	RETURN   	0 1
*/

void lhunt_gen_loadkx(HANDLER_PARAMS)
{
	function->sizecode = 13;

	Instruction* oldCode = function->code;
	function->code = (Instruction *)malloc(sizeof(Instruction) * 13);

	for (int i = 0; i < 13; ++i)
	{
		int oldIndex = 392960;

		function->code[i] = oldCode[oldIndex + i];
	}

	for (int i = 0; i < function->sizecode; ++i)
	{
		Instruction& curInst = function->code[i];

		if (GET_OPCODE(curInst) == OP_EXTRAARG)
		{
			SET_OPCODE(curInst, OP_BAND);
		}
	}

	OneOpcodeTest(OP_LOADKX)
}


// bind handler with the file name
const map<string, LuaHuntHandler> HandlerBinding = {
	{"return.lua",lhunt_gen_return}, {"call.lua",lhunt_gen_call}, {"settabup.lua",lhunt_gen_settabup},{"loadk.lua",lhunt_gen_loadk},
	{"add.lua",lhunt_gen_add},{"sub.lua",lhunt_gen_sub},{"mul.lua",lhunt_gen_mul}, {"mod.lua",lhunt_gen_mod},{"pow.lua",lhunt_gen_pow},
	{"div.lua",lhunt_gen_div},{"idiv.lua",lhunt_gen_idiv},{"band.lua",lhunt_gen_band},{"bor.lua",lhunt_gen_bor}, {"bxor.lua",lhunt_gen_bxor},
	{"bxor.lua",lhunt_gen_bxor},{"shl.lua",lhunt_gen_shl},{"shr.lua",lhunt_gen_shr},{"unm.lua",lhunt_gen_unm},{"bnot.lua",lhunt_gen_bnot},
	{"not.lua",lhunt_gen_not},{"len.lua",lhunt_gen_len},{"concat.lua",lhunt_gen_concat},{"move.lua",lhunt_gen_move},{"forloop.lua",lhunt_gen_forloop},
	{"loadbool.lua",lhunt_gen_loadbool},{"loadnil.lua",lhunt_gen_loadnil},{"closure.lua",lhunt_gen_closure},{"getupval.lua",lhunt_gen_getupval},
	{"newtable.lua",lhunt_gen_newtable},{"setlist.lua",lhunt_gen_setlist},{"self.lua",lhunt_gen_self},{"tailcall.lua",lhunt_gen_tailcall},
	{"jmp.lua",lhunt_gen_jmp},{"eq.lua",lhunt_gen_eq},{"le.lua",lhunt_gen_le},{"test.lua",lhunt_gen_test},{"testset.lua",lhunt_gen_testset},
	{"vararg.lua",lhunt_gen_vararg},{"setupval.lua",lhunt_gen_setupval},{"settable.lua",lhunt_gen_settable},{"tforcall.lua",lhunt_gen_tforcall},
	{"loadkx.lua",lhunt_gen_loadkx},{"call_second.lua",lhunt_gen_call_second}
};
