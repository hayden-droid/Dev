all:
	# create binary directory
	mkdir -p bin

	# make luahunt utils
	cd ./LuaHunt && make

	# make gadget generator for godlua
	cd ./GadgetGenerator && make
	
	# build godlua deobfuscator
	cd ./GodLuaDeobfuscate && make
	
	# generate luagadget template for loadkx
	python LuaGadgetTemplate/gen_loadkx.py > LuaGadgetTemplate/loadkx.lua
	
	#copy files
	cp ./LuaHunt/loader ./bin/loader
	cp ./GadgetGenerator/GadgetGenerator ./bin/GadgetGenerator
	cp ./LuaHunt/godlua_interpreter.bin ./bin/godlua_interpreter.bin
	cp ./GodLuaDeobfuscate/GodLuaDeobfuscate ./bin/GodLuaDeobfuscate
	
	echo 'Build finished!'

clean:
	cd ./GadgetGenerator && make clean
	cd ./LuaHunt && make clean
	cd ./GodLuaDeobfuscate && make clean

	rm -rf bin
	rm -rf LuaHunt/bcfiles
	rm -f LuaGadgetTemplate/loadkx.lua
