CC = g++
CFLAGS = -lm -O3

OBJ_FILES = lapi.o lauxlib.o lbaselib.o lbitlib.o lcode.o lcorolib.o lctype.o ldblib.o ldebug.o \
	ldo.o ldump.o lfunc.o lgc.o linit.o liolib.o llex.o lmathlib.o lmem.o loadlib.o lobject.o \
	lopcodes.o loslib.o lparser.o lstate.o lstring.o lstrlib.o ltable.o ltablib.o ltm.o luac.o \
	lundump.o lutf8lib.o lvm.o lzio.o lhunt_opcodes.o lhunt_dump.o

all: $(OBJ_FILES)
	$(CC) $(CFLAGS) -o GodLuaDeobfuscate $(OBJ_FILES)

clean:
	rm -f *.o
	rm -f GodLuaDeobfuscate
