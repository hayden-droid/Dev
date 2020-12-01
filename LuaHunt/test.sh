#!/bin/sh

python_interp=NULL

if test -f "/bin/python2"; then
    python_interp=python2
else 
    if test -f "/bin/python3"; then
        echo 'LuaHunt not compatible with python3!'
        exit
    fi
fi

chmod o+x execute.sh
chmod o+x testone.sh
chmod o+x mk.sh
chmod o+x godlua_interpreter.bin

$python_interp test_multiple.py
