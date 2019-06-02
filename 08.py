import os, re

# fileObj = open(r'.\233', 'a+')
# fileObjN = open(r'.\233', 'r+')
# fileObj.seek(0, 0)

# data = '' 
# for lineStr in fileObj:
#     print('%s %s'%(lineStr, type(lineStr)))
#     if '556' in lineStr:
#         lineStr = lineStr.replace('556', 'iii')
#     data += lineStr
# print(data)
# fileObjN.write(data)
# fileObj.close()
# fileObjN.close()

import shelve

shelFile = shelve.open('mydata')
cats = ['Zophie', 'Pooka', 'Simon']
shelFile['cats'] = cats
shelFile.close()