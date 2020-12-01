#/!bin/python

import os
from os import path
import subprocess
import luahunt

gadget_gen = '../bin/luac_gen'
result_json = 'opcodes.json'

resultDir = 'test_result_ddl'
interpreter_dir = '../bin/interpreters_ddl'

log_file = resultDir + '/log.txt'

subprocess.check_output(['rm','-rf',resultDir])
subprocess.check_output(['mkdir',resultDir])

fileNameList = os.listdir(interpreter_dir)
interpreter_list = []

for curFileName in fileNameList:
    if curFileName.split('.')[1] == 'interp':
        interpreter_list.append(curFileName)

with open(log_file, 'w') as fpLog:
    for curInterp in interpreter_list:
        suffix = '_result.json'
        curTime = luahunt.TestInterpreter(gadget_gen, interpreter_dir + '/' + curInterp)
        if curTime == -1:
            suffix = '_failed.json'
        subprocess.check_output(['rm','-rf','bcfiles'])
        os.rename(result_json,resultDir+'/'+curInterp.split('.')[0]+suffix)
        if curTime != -1:
            fpLog.write(str(curTime) + '\n')

