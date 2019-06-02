#! python3
# -*- coding: utf-8 -*-
import os, sys

def findSomeInTxt(filePath:str,regex:str='abc'):
    import re,chardet
    size = min(32, os.path.getsize(filePath))
    raw = open(filePath, mode='rb')
    result = chardet.detect(raw.read(size))
    encoding = result['encoding']
    raw.close()

    fileObj = open(filePath, encoding=encoding)
    txtRegex = re.compile(regex)
    print('finding.... in '+filePath)
    # while True:
    #     i = fileObj.readline()
    #     if len(i) > 0:
    #         print(i, end='')
    #     else:
    #         break

    i = 1
    for lineStr in fileObj:
        mo = txtRegex.search(lineStr)
        if mo != None :
            lineStr = lineStr.replace('\n','')
            print('line--%d:%s, %s\n' %(i, filePath, lineStr), end="")
        i += 1
    fileObj.close()


def main():
    print('Start....')
    fileList = os.listdir(sys.argv[1])
    for i in fileList:
        if i.lower().rfind('.txt') != -1:
            if len(sys.argv) == 2:
                findSomeInTxt(sys.argv[1]+i)
            elif len(sys.argv) == 3:
                findSomeInTxt(sys.argv[1]+i, sys.argv[2])

main()