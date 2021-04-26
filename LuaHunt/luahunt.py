#!/bin/python

import os
import sys
import subprocess
import time
import json

verbose = False

lua_src_dir = '../LuaGadgetTemplate'
bc_files_dir = 'bcfiles'

execute_script = './execute.sh'

known_opcodes_file = 'opcodes.json'

lua_src_list = ['return.lua','call.lua','settabup.lua','loadk.lua','closure.lua','tailcall.lua','add.lua',\
'sub.lua','mul.lua','mod.lua','pow.lua','idiv.lua','div.lua','band.lua','bor.lua','bxor.lua','shl.lua',\
'shr.lua','unm.lua','bnot.lua','len.lua','concat.lua','move.lua','loadbool.lua','not.lua','loadnil.lua',\
'getupval.lua','setupval.lua','jmp.lua','eq.lua','testset.lua','test.lua','vararg.lua','newtable.lua',\
'loadkx.lua','setlist.lua','settable.lua','self.lua','tforcall.lua','forloop.lua']

two_code_list = ['call.lua','setlist.lua','forloop.lua']
two_code_opnames = {'call.lua':['GETTABUP','CALL'],'setlist.lua':['SETLIST','GETTABLE'],'forloop.lua':['FORPREP','FORLOOP']}

compare_result = {'return.lua':'', 'settabup.lua':'1234567', 'loadk.lua':'LuaHunt', 'add.lua':'69134','sub.lua':'79004',\
'mul.lua':'701060205','mod.lua':'4916868','pow.lua':'1801152661463.0','div.lua':'1.6618747104978','idiv.lua':'-461',\
'band.lua':'4113','bor.lua':'65021','bxor.lua':'60908','shl.lua':'790080','shr.lua':'192','unm.lua':'-12345',\
'bnot.lua':'-12346','not.lua':'false','len.lua':'128','concat.lua':'LuaHunt','move.lua':'LuaHunt','forloop.lua':'125',\
'loadbool.lua':'true','loadnil.lua':'nil','closure.lua':'LuaHunt','getupval.lua':'LuaHunt','setlist.lua':'LuaHunt',\
'self.lua':'LuaHunt','tailcall.lua':'LuaHunt','jmp.lua':'LuaHuntJmp','testset.lua':'true','test.lua':'456789',\
'vararg.lua':'LuaHuntVarArg','setupval.lua':'LuaHuntSetUpVal','settable.lua':'LuaHuntSetTable',\
'tforcall.lua':'Lua Hunt T FORCALL','loadkx.lua':'131072'}

known_opcodes = {}

def ProcessResult(result, fileName):
    resultByLine = result.split('\n')
    for curLine in resultByLine:
        if len(curLine) == 0: #skip empty line
            continue
        splitedResult = curLine.split('<=>') #split result by format OpCode<=>Result
        if len(splitedResult) == 1: #append an empty result if the result is an empty string
            splitedResult.append('')
        curOpCode = splitedResult[0]
        curResult = splitedResult[1]
        if fileName == 'call.lua':
            if 'function:' in curResult:
                return curOpCode
        elif fileName == 'newtable.lua':
            if 'table:' in curResult:
                return curOpCode
        elif fileName == 'tforcall.lua':
            if 'Lua Hunt' in curResult:
                return curOpCode
        else:
            if curResult == compare_result[fileName]:
                return curOpCode
    return -1

def SaveOpCodeFile():
    dumpStr = json.dumps(known_opcodes) # dump result
    with open(known_opcodes_file, 'w') as fpKnownOpCodes:
        fpKnownOpCodes.write(dumpStr) # write result into file

def SaveOneCodeResult(opCode, fileName):
    opName = fileName.split('.')[0].upper()
    opCodeInt = int(opCode, base=16)
    #print 'OpCode of ' + opName + ' is ' + str(opCodeInt)
    known_opcodes[opName] = opCodeInt
    SaveOpCodeFile()

def SaveTwoCodeResult(opCode, fileName):
    opName0 = two_code_opnames[fileName][0]
    opName1 = two_code_opnames[fileName][1]
    opCodeInt0 = int(opCode[:2], base=16)
    opCodeInt1 = int(opCode[2:], base=16)
    #print 'OpCode of ' + opName0 + ' is ' + str(opCodeInt0)
    #print 'OpCode of ' + opName1 + ' is ' + str(opCodeInt1)
    known_opcodes[opName0] = opCodeInt0
    known_opcodes[opName1] = opCodeInt1
    SaveOpCodeFile()

def TestGadgets(interpreter):
    return subprocess.check_output([execute_script, bc_files_dir, interpreter]) #test gadgets

def GenGadgetFiles(gadget_gen, src_name):
    return subprocess.check_output([gadget_gen, '-l', '-g', '-god', bc_files_dir, '-kno', known_opcodes_file, lua_src_dir + '/' + src_name])

def Test(fileName, gadget_gen, interpreter):
    if verbose == True: print 'Generating gadget for ' + fileName
    genRes = GenGadgetFiles(gadget_gen, fileName)
    if verbose == True:
        if fileName != 'loadkx.lua':
            print genRes
    if verbose == True: print 'Testing ' + fileName
    strResult = TestGadgets(interpreter)
    if verbose == True: print strResult
    opCodeResult = ProcessResult(strResult, fileName) #process result
    return opCodeResult

def GetResult(strResult, strComparison):
    resultList = []
    resultByLine = strResult.split('\n')
    for curLine in resultByLine:
        if len(curLine) == 0: #skip empty line
            continue
        splitedResult = curLine.split('<=>') #split result by format OpCode<=>Result
        if len(splitedResult) == 1: #append an empty result if the result is an empty string
            splitedResult.append('')
        curOpCode = splitedResult[0]
        curResult = splitedResult[1]
        if curResult == strComparison:
            resultList.append(curOpCode)
    return resultList

def TestEq(gadget_gen, interpreter):
    if verbose == True: print 'Generating gadget for eq.lua'
    gen_res = GenGadgetFiles(gadget_gen, 'eq.lua')
    if verbose == True: print gen_res
    if verbose == True: print 'Testing eq.lua'
    strResult = TestGadgets(interpreter)
    if verbose == True: print strResult
    resultEqTest = GetResult(strResult, 'LuaHuntEq')
    if len(resultEqTest) != 2:
        print 'Test OP_EQ failed! resultEqTest:'
        print resultEqTest
        return False
    resultEq = resultEqTest[0]
    resultLe = resultEqTest[1]
    resultLt = -1
    #print resultEqTest
    SaveOneCodeResult(resultEq, 'eq.lua') # save temp result of eq
    # test le
    if verbose == True:print 'Generating gadget for le.lua'
    gen_res = GenGadgetFiles(gadget_gen, 'le.lua')
    if verbose == True: print gen_res
    if verbose == True: print 'Testing le.lua'
    strResult = TestGadgets(interpreter)
    if verbose == True: print strResult
    resultLeResult = GetResult(strResult, 'LuaHuntLe')
    #print resultLeResult
    if len(resultLeResult) == 2:
        if resultLe == resultLeResult[0]:
            resultLt = resultLeResult[1]
        else:
            resultLt = resultLeResult[0]
    elif len(resultLeResult) == 1: # switch result of eq and le if le test failed
        resultEq = resultEqTest[1]
        resultLe = resultEqTest[0]
        resultLt = resultLeResult[0]
    else:
        print 'Test OP_LE failed! resultLeResult:'
        print resultLeResult
        return False
    SaveOneCodeResult(resultEq, 'eq.lua') # save final result of eq and le
    SaveOneCodeResult(resultLe, 'le.lua')
    SaveOneCodeResult(resultLt, 'lt.lua')

def TestTailCall(gadget_gen, interpreter):
    if verbose == True: print 'Generating gadget for tailcall.lua'
    gen_res = GenGadgetFiles(gadget_gen, 'tailcall.lua')
    if verbose == True: print gen_res
    if verbose == True: print 'Testing tailcall.lua'
    strResult = TestGadgets(interpreter)
    if verbose == True: print strResult
    resultTailCall = GetResult(strResult, 'LuaHunt')
    if len(resultTailCall) == 0:
        known_opcodes['TAILCALL'] = known_opcodes['CALL']
        del known_opcodes['CALL']
        if verbose == True: print 'Generating gadget for call_second.lua'
        gen_res = GenGadgetFiles(gadget_gen, 'call_second.lua')
        if verbose == True: print gen_res
        if verbose == True: print 'Testing call_second.lua'
        strResult = TestGadgets(interpreter)
        if verbose == True: print strResult    
        opCodeResult = ProcessResult(strResult, 'call.lua')
        if opCodeResult != -1:
            SaveOneCodeResult(opCodeResult,'call.lua')
        else:
            print 'CALL testing failed!'
            return False
    else:
        SaveOneCodeResult(resultTailCall[0],'tailcall.lua')
    
def TestTForCall(gadget_gen, interpreter):
    if verbose == True: print 'Generating gadget for tforcall.lua'
    gen_res = GenGadgetFiles(gadget_gen, 'tforcall.lua')
    if verbose == True: print gen_res
    if verbose == True: print 'Testing tforcall.lua'
    strResult = TestGadgets(interpreter)
    if verbose == True: print strResult
    opCodeTForCall = ProcessResult(strResult,'tforcall.lua')
    if opCodeTForCall == -1:
        known_opcodes['TFORLOOP'] = known_opcodes['JMP']
        del known_opcodes['JMP']
        SaveOpCodeFile()
        opCodeJmp = Test('jmp.lua',gadget_gen,interpreter)
        if opCodeJmp != -1:
            SaveOneCodeResult(opCodeJmp,'jmp.lua')
            opCodeTForCall = Test('tforcall.lua',gadget_gen,interpreter)
            if opCodeTForCall != -1:
                SaveOneCodeResult(opCodeTForCall, 'tforcall.lua')
            else:
                print('Test TFORCALL failed!')
                return False
        else:
            print('Test TFORCALL failed!')
            return False
    else:
        SaveOneCodeResult(opCodeTForCall,'tforcall.lua')

def TestFile(fileName, gadget_gen, interpreter):
    if fileName == 'eq.lua':
        return TestEq(gadget_gen, interpreter)
    elif fileName == 'tailcall.lua':  
        return TestTailCall(gadget_gen, interpreter)
    elif fileName == 'tforcall.lua':
        return TestTForCall(gadget_gen, interpreter)

    opCodeResult = Test(fileName,gadget_gen, interpreter)

    if opCodeResult == -1:
        print 'Test failed!'
        return False

    if fileName in two_code_list: # save result
        SaveTwoCodeResult(opCodeResult, fileName)
    else:
        SaveOneCodeResult(opCodeResult, fileName)

def TestInterpreter(gadget_gen, interpreter):
    with open(known_opcodes_file, 'w') as fpKnownOpCodes:
        fpKnownOpCodes.write('{ }')
    
    known_opcodes.clear()

    print 'Start testing for ' + interpreter + ':'

    start = time.time()

    for curSrc in lua_src_list:
        if verbose == True: print '---------------------------- ' + curSrc + ' ----------------------------'

        if verbose == False: print 'Testing ' + curSrc + '...'

        if TestFile(curSrc, gadget_gen, interpreter) == False:
            print 'Testing ' + interpreter + ' with ' + curSrc + ' failed!'
            return -1

        print 'Done.'
        if verbose == True: print '------------------------------------------------------------------'

    if 'TFORLOOP' not in known_opcodes:
        opCodeTForLoop = Test('jmp.lua',gadget_gen,interpreter)
        if opCodeTForLoop == -1:
            print 'Test TFORLOOP failed!'
            return -1
        SaveOneCodeResult(opCodeTForLoop,'tforloop.lua')
    
    knownOpCodeList = []
    for curKey in known_opcodes:
        knownOpCodeList.append(known_opcodes[curKey])
    for cur in range(0,47):
        if cur not in knownOpCodeList:
            SaveOneCodeResult(str(hex(cur)),'extraarg.lua')

    stop = time.time()

    totalTime = stop - start

    print 'LuaHunt testing finished! Total time consuming: ' + str(totalTime)

    print 'OpCode sequence of ' + interpreter +': '
    print known_opcodes

    return totalTime

def main():
    if len(sys.argv) != 3 and len(sys.argv) != 4:
        print 'Usage: ' + sys.argv[0] + ' GadgetGenerator Interpreter [-v]'
        exit()

    if len(sys.argv) == 4:
        if sys.argv[3] == '-v':
            global verbose
            verbose = True

    TestInterpreter(sys.argv[1], sys.argv[2])

if __name__== "__main__":
    main()
