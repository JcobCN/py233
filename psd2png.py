#! /usr/bin/python3
import os,sys
from psd_tools import PSDImage
import PIL

def psd2png(dirPath):
    print('dirPath=%s'% (dirPath))
    flistList = os.listdir(dirPath)
    for psdFile in flistList:
        print(psdFile)
        if '.psd'.lower() in psdFile.lower():
            print(dirPath+"/"+psdFile)
            psd =PSDImage.load(dirPath+"/"+psdFile)
            print(psd.layers)
            mergedImage = psd.as_PIL()
            mergedImage.save("%s/%s.jpg" % (dirPath, psdFile.lower().replace(".psd", "") ) )
    

if __name__ == '__main__':
    psd2png(sys.argv[1])