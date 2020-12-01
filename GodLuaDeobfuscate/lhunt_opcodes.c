#include "lhunt_opcodes.h"
#include "lopcodes.h"

#include <string.h>

LUAI_DDEC const char* const opnames_ori[] = {
  "MOVE",
  "LOADK",
  "LOADKX",
  "LOADBOOL",
  "LOADNIL",
  "GETUPVAL",
  "GETTABUP",
  "GETTABLE",
  "SETTABUP",
  "SETUPVAL",
  "SETTABLE",
  "NEWTABLE",
  "SELF",
  "ADD",
  "SUB",
  "MUL",
  "MOD",
  "POW",
  "DIV",
  "IDIV",
  "BAND",
  "BOR",
  "BXOR",
  "SHL",
  "SHR",
  "UNM",
  "BNOT",
  "NOT",
  "LEN",
  "CONCAT",
  "JMP",
  "EQ",
  "LT",
  "LE",
  "TEST",
  "TESTSET",
  "CALL",
  "TAILCALL",
  "RETURN",
  "FORLOOP",
  "FORPREP",
  "TFORCALL",
  "TFORLOOP",
  "SETLIST",
  "CLOSURE",
  "VARARG",
  "EXTRAARG",
  NULL
};

OpCode GetOriOpCode(int opcode)
{
    for (int i = 0; i < NUM_OPCODES; ++i)
    {
        if (!strcmp(opnames_ori[i], luaP_opnames[opcode]))
            return (OpCode)i;
    }

    // UNDEFINED
    return (OpCode)-1;
}

void lhunt_setopcodes(int sizecode, Instruction* code)
{
	for (int i = 0; i < sizecode; ++i)
	{
	    OpCode opCode = GET_OPCODE(code[i]);
            OpCode oriOpCode = GetOriOpCode(opCode);

            SET_OPCODE(code[i], oriOpCode);
	}
}
