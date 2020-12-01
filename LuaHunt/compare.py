#!/bin/python

import sys
import json

def Compare(std_file_name, test_result):
    stdOpCodes = {}
    testResult = {}

    with open(std_file_name,'r') as fpStdOpCodesFile:
        stdOpCodes = json.load(fpStdOpCodesFile)

    with open(test_result,'r') as fpTestResultFile:
        testResult = json.load(fpTestResultFile)

    diff_list = []

    for curOpName in stdOpCodes:
        if curOpName not in testResult:
            diff_list.append(curOpName)
        elif stdOpCodes[curOpName] != testResult[curOpName]:
            diff_list.append(curOpName)
    
    return diff_list

def main():
    if len(sys.argv) != 3:
        print 'Usage: ' + sys.argv[0] + ' StandardOpCodeFile TestResult'
        exit()

    print 'Comparing ' + sys.argv[1] + ' with ' + sys.argv[2] + '...'
    compare_result = Compare(sys.argv[1],sys.argv[2])
    if len(compare_result) != 0:
        print 'Different OpCodes:'
        print compare_result
    else:
        print 'Same result.'

if __name__== "__main__":
    main()