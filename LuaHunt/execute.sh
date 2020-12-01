#!/bin/sh

loader=../bin/loader
interpreter=$2

for file in `ls $1/`
do
    result=`timeout 5 $loader $interpreter $1/$file`
    echo $file"<=>"$result
done
