#include "lhunt_hook.h"

#include <iostream>
#include <map>
#include <string.h>

#include "luac.h"
#include "lopcodes.h"

#include "lhunt_handlers.h"

using namespace std;

#define OPCODE_FILE "known_opcodes.json"

#define CompareFileName(a,b) !strcmp(a,b)

#define IsFileName(x) CompareFileName(fileName, x)


string GetFileNameWithoutPath(string fileName)
{
	size_t pos = 0;
	string delimiter = "/";

	while ((pos = fileName.find(delimiter)) != std::string::npos)
		fileName.erase(0, pos + delimiter.length());

	return fileName;
}

void lhunt_dump(lua_State* luaState, Proto* function, lua_Writer writer, char* fileName)
{
	lhunt_handlers_init();

	cout << "Dispatching..." << endl;

	auto itr = HandlerBinding.find(GetFileNameWithoutPath(fileName));

	cout << "Generating..." << endl;

	if (!genGadgets)
	{
		DumpFile(PASSING_HANDLER_PARAMS);
	}
	else
	{
		if (itr != HandlerBinding.end())
			itr->second(PASSING_HANDLER_PARAMS);
		else
		{
			cout << "Call handler failed!!!" << endl;
			return;
		}
	}

	lhunt_handlers_free();
}
