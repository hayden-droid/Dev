#!/bin/python

import os
from os import path
import compare

gen_file_dir = '../bin/interpreters'
result_dir = 'test_result'
log_file = result_dir + '/log.txt'

gen_file_list = os.listdir(gen_file_dir)
for cur_gen_file_name in gen_file_list:
    if cur_gen_file_name.split('.')[1] == 'json':
        cur_gen_name = cur_gen_file_name.split('.')[0]
        result_file_name = result_dir + '/' + cur_gen_name + '_result.json'
        if path.exists(result_file_name):
            print 'Comparing ' +cur_gen_file_name + ' with ' + result_file_name + '...'
            compare_result = compare.Compare(gen_file_dir + '/' + cur_gen_file_name, result_file_name)
            if len(compare_result) != 0:
                print 'Different OpCodes:'
                print compare_result
            else:
                print 'Same result.'
        else:
            print cur_gen_name + ' missing!!!'

time_list = []
with open(log_file,'r') as fpLog:
    time_list = fpLog.readlines()

total_time = 0.0
for cur_time in time_list:
    total_time = total_time + float(cur_time)

print 'Testing count: ' + str(len(time_list))
print 'Total time: ' + str(total_time)
print 'Avg testing time: ' + str(total_time / len(time_list))