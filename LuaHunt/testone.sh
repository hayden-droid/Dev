#!/bin/sh

gadget_gen=../bin/GadgetGenerator
interpreter=godlua_interpreter.bin

$gadget_gen -g -god bcfiles -kno opcodes.json $1

./execute.sh bcfiles
